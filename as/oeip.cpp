#include "oeip.h"
#include <opencv2/highgui.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/imgproc.hpp>

// TODO: This should be configurable by the user
static constexpr int total_edge_buffers = 4;

#define UMat Mat

struct Edge_Buffers {
	Edge_Buffers(int n, cv::Size const& size) {
		for (int i = 0; i < n; i++) {
			_buffers.emplace_back(cv::UMat(size, CV_16U));
		}
	}

	std::vector<cv::UMat> _buffers;
};

static int get_edge_buffer_weight(int c, int n, int i) {
	// i = c -> n
	// else (i + n - c) `mod` n
	return (i != c) ? ((i + n - c) % n) : (n);
}

static void two_major_bins(cv::Mat const& H, int* idx0, int* idx1) {
	int N = H.cols * H.rows;

	auto max0 = H.at<float>(0);
	auto max1 = H.at<float>(0);
	int max0_idx = 0;
	int max1_idx = 0;

	for (int i = 1; i < N; i++) {
		auto v = H.at<float>(i);

		if (v > max0) {
			max0 = v;
			max0_idx = i;
		}
	}

	for (int i = 1; i < N; i++) {
		auto v = H.at<float>(i);

		if (max1 < v && v < max0) {
			max1 = v;
			max1_idx = i;
		}
	}

	*idx0 = max0_idx;
	*idx1 = max1_idx;
}

class OEIP : public IOEIP {
public:
	OEIP(char const* pathToVideo) :
		_video(cv::VideoCapture(pathToVideo)),
		_cb_output(nullptr),
		_cb_benchmark(nullptr)
	{
		auto width = (int)_video.get(cv::CAP_PROP_FRAME_WIDTH);
		auto height = (int)_video.get(cv::CAP_PROP_FRAME_HEIGHT);
		auto fps = round(_video.get(cv::CAP_PROP_FPS));

		_mask_subtitle_bottom = cv::UMat::zeros(height, width, CV_8U);
		_mask_subtitle_bottom(cv::Rect(width * 0.2f, height * 0.8f, width * 0.6f, height * 0.2f)) = 255;

		for (int i = 0; i < total_edge_buffers; i++) {
			cv::Mat tmp;
			_edge_buffers[i] = cv::UMat::zeros(height, width, CV_8U);
		}
	}

protected:
	void register_stage_output_callback_impl(oeip_cb_output fun) override {
		_cb_output = fun;
	}

	void register_stage_benchmark_callback_impl(oeip_cb_benchmark fun) override {
		_cb_benchmark = fun;
	}

	bool _init = false;

	void edge_buffers_average(cv::UMat& avg, cv::UMat& avg_bin, cv::Size const& size) {
		// NOTE: maguk az edge bufferek 8U formatumban vannak es 0-255 normalizalt ertekek vannak benne
		// Ezeket akkumulaljuk 16U matrixba
		cv::UMat acc(size, CV_16U, cv::Scalar(0));
		for (int i = 0; i < total_edge_buffers; i++) {
			// Sulyozott atlag; a legujabb kapja a legnagyobb sulyt, a legregebbi a legkisebbet
			cv::UMat eb(size, CV_16U);
			_edge_buffers[i].convertTo(eb, CV_16U);
			auto w = get_edge_buffer_weight(_edge_buffers_cursor, total_edge_buffers, i);
			acc += w * eb;
		}

		auto const sum_of_weights = total_edge_buffers * (total_edge_buffers + 1) / 2;
		cv::UMat avg_tmp = acc / sum_of_weights;

		avg_tmp.convertTo(avg, CV_8U);
		threshold(avg, avg_bin, 252, 255, cv::THRESH_BINARY);
	}

	bool step_impl() {
		cv::Mat buf;
		bool has_output_callback = _cb_output != nullptr;
		
		_video.read(buf);

		if (buf.empty()) {
			return false;
		}

		if (!_init) {
			auto edges = detect_edges(buf);
			for (int i = 0; i < total_edge_buffers; i++) {
				_edge_buffers[i] = edges;
			}
			_init = true;
		}


		emit_output(OEIP_STAGE_INPUT, OEIP_COLSPACE_RGB888_RGB, buf);

		auto edge_cur = get_edge_buffer(buf);
		edge_cur.convertTo(edge_cur, CV_8U);

		_edge_buffers_cursor = (_edge_buffers_cursor + 1) % total_edge_buffers;
		_edge_buffers[_edge_buffers_cursor] = std::move(edge_cur);

		auto edge_avg = average_edge_buffers();
		edge_avg.convertTo(edge_avg, CV_8U);
		cv::UMat edge_cur_bin;
		cv::threshold(edge_avg, edge_cur_bin, 127, 255, CV_8U);

		emit_output(OEIP_STAGE_ACCUMULATED_EDGE_BUFFER, OEIP_COLSPACE_R8, edge_cur_bin);


		// megjeloljuk azokat a pixeleket, amelyek a YCbCr kep Cb es Cr csatornajanak hisztogramjaiban
		// benne vannak a ket major bin-ben
		cv::Mat buf_ycrcb;
		cvtColor(buf, buf_ycrcb, cv::COLOR_RGB2YCrCb);
		cv::Mat buf_ycrcb_channels[3];
		cv::split(buf_ycrcb, buf_ycrcb_channels);

		cv::Mat buf_cr;
		cv::Mat buf_cb;

		// TODO: leftover kod? minek csinalunk masolatot?
		buf_ycrcb_channels[1].copyTo(buf_cr);
		buf_ycrcb_channels[2].copyTo(buf_cb);

		cv::Mat mask;

		cv::bitwise_and(_mask_subtitle_bottom, edge_cur_bin, mask);

		cv::bitwise_and(buf_ycrcb_channels[1], mask, buf_cr);
		cv::bitwise_and(buf_ycrcb_channels[2], mask, buf_cb);

		int histSize = 256;
		float range[] = { 0, 256 };
		const float* histRange = { range };

		cv::UMat cr_hist, cb_hist;

		// Letrehozzuk a kepkocka hisztogramjat a Cr es Cb csatornakban
		cv::calcHist(&buf_cr, 1, 0, mask, cr_hist, 1, &histSize, &histRange, true, false);
		cv::calcHist(&buf_cb, 1, 0, mask, cb_hist, 1, &histSize, &histRange, true, false);

		// Megkeressuk a ket foszint mindegyik csatornaban
		int cr0, cr1;
		int cb0, cb1;
		two_major_bins(cr_hist, &cr0, &cr1);
		two_major_bins(cb_hist, &cb0, &cb1);

		cv::UMat thresh_cr0, thresh_cr1;
		cv::inRange(buf_ycrcb_channels[1], cv::Scalar(cr0), cv::Scalar(cr0), thresh_cr0);
		cv::inRange(buf_ycrcb_channels[1], cv::Scalar(cr1), cv::Scalar(cr1), thresh_cr1);
		cv::UMat thresh_cr = cv::max(thresh_cr0, thresh_cr1);

		cv::UMat thresh_cb0, thresh_cb1;
		cv::inRange(buf_ycrcb_channels[2], cv::Scalar(cb0), cv::Scalar(cb0), thresh_cb0);
		cv::inRange(buf_ycrcb_channels[2], cv::Scalar(cb1), cv::Scalar(cb1), thresh_cb1);
		cv::UMat thresh_cb = cv::max(thresh_cb0, thresh_cb1);

		cv::UMat thresh(thresh_cr.rows, thresh_cr.cols, CV_32F);
		thresh = cv::max(thresh_cr, thresh_cb);

		cv::UMat subtitle_mask;
		cv::UMat thresh_blur3, avg_bin_blur3;

		cv::bitwise_and(edge_cur_bin, _mask_subtitle_bottom, edge_cur_bin);
		cv::pyrDown(edge_cur_bin, edge_cur_bin, cv::Size(edge_cur_bin.cols / 2, edge_cur_bin.rows / 2));
		cv::pyrDown(thresh, thresh, cv::Size(thresh.cols / 2, thresh.rows / 2));

		cv::GaussianBlur(thresh, thresh_blur3, { 3, 3 }, 0.0f);
		cv::GaussianBlur(edge_cur_bin, avg_bin_blur3, { 3, 3 }, 0.0f);

		cv::threshold(avg_bin_blur3, avg_bin_blur3, 0, 255, cv::THRESH_BINARY);
		cv::threshold(thresh_blur3, thresh_blur3, 0, 255, cv::THRESH_BINARY);
		cv::bitwise_and(avg_bin_blur3, thresh_blur3, subtitle_mask);

		if (_avg_subtitle_mask.empty()) {
			subtitle_mask.copyTo(_avg_subtitle_mask);
		}

		average_u8(_avg_subtitle_mask, subtitle_mask, _avg_subtitle_mask);

		emit_output(OEIP_STAGE_SUBTITLE_MASK, OEIP_COLSPACE_R8, _avg_subtitle_mask);

		if (has_output_callback) {
			for (int i = 0; i < OEIP_STAGE_OUTPUT + 1; i++) {
				auto& buf = _output_buffers[i];
				if (!buf.empty()) {
					auto ptr = buf.ptr<unsigned char>();
					auto width = buf.cols;
					auto height = buf.rows;
					auto stride = buf.step[0];
					_cb_output((oeip_stage)i, _output_buffer_formats[i], ptr, height * stride, width, height, stride);
				}
			}
		}

		return true;
	}

	void average_u8(cv::Mat const& lhs, cv::Mat const& rhs, cv::Mat& out) {
		cv::Mat acc(lhs.size(), CV_64F, cv::Scalar(0));
		accumulate(lhs, acc);
		accumulate(rhs, acc);
		acc.convertTo(out, CV_8U, 1 / 2.0);
	}

	cv::UMat detect_edges(cv::Mat const& buf) {
		cv::UMat src, blurred, blurred_gray;

		buf.copyTo(src);
		GaussianBlur(src, blurred, cv::Size(3, 3), 0, 0, cv::BORDER_DEFAULT);
		cvtColor(blurred, blurred_gray, cv::COLOR_BGR2GRAY);

		cv::UMat grad_x, grad_y, grad;
		cv::UMat abs_grad_x, abs_grad_y;
		Sobel(blurred_gray, grad_x, CV_16U, 1, 0);
		Sobel(blurred_gray, grad_y, CV_16U, 0, 1);
		convertScaleAbs(grad_x, abs_grad_x);
		convertScaleAbs(grad_y, abs_grad_y);
		addWeighted(abs_grad_x, 0.5, abs_grad_y, 0.5, 0, grad);

		return grad;
	}

	cv::UMat& prev_edge_buffer() {
		return _edge_buffers[_edge_buffers_cursor];
	}

	cv::UMat get_edge_buffer(cv::Mat const& buf) {
		auto edges = detect_edges(buf);
		emit_output(OEIP_STAGE_CURRENT_EDGE_BUFFER, OEIP_COLSPACE_R8, edges);
		return edges;
		/*
		edges.convertTo(edges, CV_16U);
		auto prev = prev_edge_buffer();
		prev.convertTo(prev, CV_16U);

		cv::UMat e_i = edges + prev;
		cv::UMat avg = average_edge_buffers(e_i);
		cv::UMat e_i_2 = e_i - avg;

		return e_i_2;
		*/
	}

	template<typename InputArray>
	void emit_output(oeip_stage stage, oeip_buffer_color_space cs, InputArray mat) {
		mat.copyTo(_output_buffers[stage]);
		_output_buffer_formats[stage] = cs;
	}

	cv::UMat average_edge_buffers() {
		cv::UMat acc = cv::UMat::zeros(_edge_buffers[0].size(), CV_16U);

		auto temp_idx = (_edge_buffers_cursor + 1) % total_edge_buffers;
		auto const sum_of_weights = total_edge_buffers * (total_edge_buffers + 1) / 2;

		for (int i = 0; i < total_edge_buffers; i++) {
			cv::UMat eb;
            _edge_buffers[i].convertTo(eb, CV_16U);

			// auto w = get_edge_buffer_weight(temp_idx, total_edge_buffers, i);
			auto w = 1;
			acc += w * eb;
		}

		return acc / total_edge_buffers;
	}

private:
	cv::VideoCapture _video;
	oeip_cb_output _cb_output;
	oeip_cb_benchmark _cb_benchmark;

	cv::UMat _mask_subtitle_bottom;
	cv::UMat _edge_buffers[total_edge_buffers];
	int _edge_buffers_cursor = 0;

	cv::Mat _output_buffers[oeip_stage::OEIP_STAGE_OUTPUT + 1];
	oeip_buffer_color_space _output_buffer_formats[oeip_stage::OEIP_STAGE_OUTPUT + 1];

	cv::UMat _avg_subtitle_mask;
};

std::unique_ptr<IOEIP> make_oeip(char const* pathToVideo) {
	return std::make_unique<OEIP>(pathToVideo);
}