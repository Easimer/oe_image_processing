#include "oeip.h"
#include <opencv2/highgui.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/imgproc.hpp>

// TODO: This should be configurable by the user
static constexpr int total_edge_buffers = 4;

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
			_edge_buffers[i] = cv::UMat(height, width, CV_16S);
		}
	}

protected:
	void register_stage_output_callback_impl(oeip_cb_output fun) override {
		_cb_output = fun;
	}

	void register_stage_benchmark_callback_impl(oeip_cb_benchmark fun) override {
		_cb_benchmark = fun;
	}

	bool step_impl() {
		cv::Mat buf;
		cv::UMat edge_cur;
		bool has_output_callback = _cb_output != nullptr;
		
		_video.read(buf);

		if (buf.empty()) {
			return false;
		}

		emit_output(OEIP_STAGE_INPUT, OEIP_COLSPACE_RGB888_RGB, buf);

		// TODO: ha ezt a sort kitoroljuk, akkor nem fog mukodni az algo
		// Nem kene a Canny()-nek kiuriteni az edge_cur matrixot?
		edge_cur = get_edge_buffer(buf);
		Canny(buf, edge_cur, 0, 0);

		emit_output(OEIP_STAGE_CURRENT_EDGE_BUFFER, OEIP_COLSPACE_R8, edge_cur);

		auto& e1 = _edge_buffers[_edge_buffers_cursor];

		addWeighted(e1, 1, edge_cur, 1, 0, edge_cur, CV_16S);

		_edge_buffers_cursor = (_edge_buffers_cursor + 1) % total_edge_buffers;
		_edge_buffers[_edge_buffers_cursor] = std::move(edge_cur);

		cv::UMat acc(buf.size(), CV_32F, cv::Scalar(0));
		for (int i = 0; i < total_edge_buffers; i++) {
			cv::Mat eb;
			_edge_buffers[i].copyTo(eb);
			auto w = get_edge_buffer_weight(_edge_buffers_cursor, total_edge_buffers, i);
			accumulate(w * eb, acc);
		}

		cv::UMat avg, avg_bin;

		auto avg_div = total_edge_buffers * (total_edge_buffers + 1) / 2.0f;

		acc.convertTo(avg, CV_8U, 1 / avg_div);
		threshold(avg, avg_bin, 252, 255, cv::THRESH_BINARY);

		subtract(_edge_buffers[_edge_buffers_cursor], avg_bin, _edge_buffers[_edge_buffers_cursor], cv::noArray(), CV_16S);		

		emit_output(OEIP_STAGE_ACCUMULATED_EDGE_BUFFER, OEIP_COLSPACE_R8, avg_bin);

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

		cv::bitwise_and(buf_ycrcb_channels[1], _mask_subtitle_bottom, buf_cr);
		cv::bitwise_and(buf_ycrcb_channels[2], _mask_subtitle_bottom, buf_cb);

		int histSize = 256;
		float range[] = { 0, 256 };
		const float* histRange = { range };

		cv::Mat cr_hist, cb_hist;

		// TODO: talan az acc. edge buffert kene maszkkent hasznalni amikor
		// a ket foszint keressuk, nem pedig a textbox maszkot.

		cv::calcHist(&buf_cr, 1, 0, _mask_subtitle_bottom, cr_hist, 1, &histSize, &histRange, true, false);
		cv::calcHist(&buf_cb, 1, 0, _mask_subtitle_bottom, cb_hist, 1, &histSize, &histRange, true, false);

		int cr0, cr1;
		int cb0, cb1;
		two_major_bins(cr_hist, &cr0, &cr1);
		two_major_bins(cb_hist, &cb0, &cb1);

		cv::Mat thresh_cr0, thresh_cr1;
		cv::inRange(buf_cr, cv::Scalar(cr0), cv::Scalar(cr0), thresh_cr0);
		cv::inRange(buf_cr, cv::Scalar(cr1), cv::Scalar(cr1), thresh_cr1);
		cv::Mat thresh_cr = cv::max(thresh_cr0, thresh_cr1);

		cv::Mat thresh_cb0, thresh_cb1;
		cv::inRange(buf_cb, cv::Scalar(cb0), cv::Scalar(cb0), thresh_cb0);
		cv::inRange(buf_cb, cv::Scalar(cb1), cv::Scalar(cb1), thresh_cb1);
		cv::Mat thresh_cb = cv::max(thresh_cb0, thresh_cb1);

		cv::Mat thresh(thresh_cr.rows, thresh_cr.cols, CV_32F);
		thresh = cv::max(thresh_cr, thresh_cb);

		cv::Mat subtitle_mask;
		cv::Mat thresh_blur3, avg_bin_blur3;

		cv::GaussianBlur(thresh, thresh_blur3, { 3, 3 }, 0.0f);
		cv::GaussianBlur(avg_bin, avg_bin_blur3, { 3, 3 }, 0.0f);

		cv::bitwise_and(avg_bin_blur3, thresh_blur3, subtitle_mask);

		emit_output(OEIP_STAGE_SUBTITLE_MASK, OEIP_COLSPACE_R8, subtitle_mask);

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

	cv::UMat get_edge_buffer(cv::Mat const& buf) {
		cv::UMat src, blurred, blurred_gray;

		buf.copyTo(src);
		GaussianBlur(src, blurred, cv::Size(3, 3), 0, 0, cv::BORDER_DEFAULT);
		cvtColor(blurred, blurred_gray, cv::COLOR_BGR2GRAY);

		cv::UMat grad_x, grad_y, grad;
		cv::UMat abs_grad_x, abs_grad_y;
		Sobel(blurred_gray, grad_x, CV_16S, 1, 0);
		Sobel(blurred_gray, grad_y, CV_16S, 0, 1);
		convertScaleAbs(grad_x, abs_grad_x);
		convertScaleAbs(grad_y, abs_grad_y);
		addWeighted(abs_grad_x, 0.5, abs_grad_y, 0.5, 0, grad);

		return grad;
	}

	template<typename InputArray>
	void emit_output(oeip_stage stage, oeip_buffer_color_space cs, InputArray mat) {
		mat.copyTo(_output_buffers[stage]);
		_output_buffer_formats[stage] = cs;
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
};

std::unique_ptr<IOEIP> make_oeip(char const* pathToVideo) {
	return std::make_unique<OEIP>(pathToVideo);
}