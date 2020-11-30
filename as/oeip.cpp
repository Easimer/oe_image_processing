#include <optional>

#include <opencv2/highgui.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/imgproc.hpp>

#include "oeip.h"
#include "image_inpainting.h"

static constexpr int total_edge_buffers = 4;

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

static void two_major_bins(cv::UMat const &H, int *idx0, int *idx1) {
	cv::Mat buf;
	H.copyTo(buf);
	two_major_bins(buf, idx0, idx1);
}

// Asszertaljuk, hogy P implikalja Q-t
#define assert_implies(p, q) assert(!(p) || (q))

class OEIP : public IOEIP {
public:
	OEIP(cv::VideoCapture &&video, std::optional<cv::VideoWriter> &&output) :
		_video(std::move(video)),
		_output(std::move(output)),
		_cb_output(nullptr),
		_cb_progress(nullptr),
		_progress_callback_mask(0x00000000)
	{
		assert(_video.isOpened());
		assert_implies(output.has_value(), output->isOpened());

		auto width = (int)_video.get(cv::CAP_PROP_FRAME_WIDTH);
		auto height = (int)_video.get(cv::CAP_PROP_FRAME_HEIGHT);
		auto fps = round(_video.get(cv::CAP_PROP_FPS));

		_mask_subtitle_bottom = cv::UMat::zeros(height, width, CV_8U);
		_mask_subtitle_bottom(cv::Rect(width * 0.2f, height * 0.8f, width * 0.6f, height * 0.2f)) = 255;

		for (int i = 0; i < total_edge_buffers; i++) {
			_edge_buffers[i] = cv::UMat::zeros(height, width, CV_8U);
		}

		_total_frames = (int)_video.get(cv::CAP_PROP_FRAME_COUNT);
	}

protected:
	void register_stage_output_callback_impl(oeip_cb_output fun) override {
		_cb_output = fun;
	}

	void register_progress_callback_impl(oeip_cb_progress fun) override {
		_cb_progress = fun;
		struct oeip_progress_info inf = { 0, _total_frames };
		_cb_progress(&inf);
	}

	// Felirat-maszk eloallitasa
	// _avg_subtitle_mask-ba teszi a maszkot
	bool make_subtitle_mask(cv::Mat const& frame) {
		if (frame.empty()) {
			return false;
		}

		if (!_init) {
			auto edges = detect_edges(frame);
			for (int i = 0; i < total_edge_buffers; i++) {
				_edge_buffers[i] = edges;
			}
			_init = true;
		}


		emit_output(OEIP_STAGE_INPUT, OEIP_COLSPACE_RGB888_RGB, frame);

		auto edge_cur = get_edge_buffer(frame);
		edge_cur.convertTo(edge_cur, CV_8U);

		_edge_buffers_cursor = (_edge_buffers_cursor + 1) % total_edge_buffers;
		_edge_buffers[_edge_buffers_cursor] = std::move(edge_cur);

		auto edge_avg = average_edge_buffers();
		edge_avg.convertTo(edge_avg, CV_8U);
		cv::UMat edge_cur_bin;
		threshold(edge_avg, edge_cur_bin, 127, 255, CV_8U);

		emit_output(OEIP_STAGE_ACCUMULATED_EDGE_BUFFER, OEIP_COLSPACE_R8, edge_cur_bin);

		// megjeloljuk azokat a pixeleket, amelyek a YCbCr kep Cb es Cr csatornajanak hisztogramjaiban
		// benne vannak a ket major bin-ben
		cv::UMat buf_ycrcb;
		cvtColor(frame, buf_ycrcb, cv::COLOR_RGB2YCrCb);
		// cv::UMat buf_ycrcb_channels[3];
		std::vector<cv::UMat> buf_ycrcb_channels;
		split(buf_ycrcb, buf_ycrcb_channels);

		cv::UMat buf_cr;
		cv::UMat buf_cb;

		// TODO: leftover kod? minek csinalunk masolatot?
		buf_ycrcb_channels[1].copyTo(buf_cr);
		buf_ycrcb_channels[2].copyTo(buf_cb);

		cv::UMat mask;

		bitwise_and(_mask_subtitle_bottom, edge_cur_bin, mask);

		bitwise_and(buf_ycrcb_channels[1], mask, buf_cr);
		bitwise_and(buf_ycrcb_channels[2], mask, buf_cb);

		cv::Mat buf_cr_loc, buf_cb_loc;

		buf_cr.copyTo(buf_cr_loc);
		buf_cb.copyTo(buf_cb_loc);

		int histSize = 256;
		float range[] = { 0, 256 };
		const float* histRange = { range };

		cv::UMat cr_hist, cb_hist;

		// Letrehozzuk a kepkocka hisztogramjat a Cr es Cb csatornakban
		calcHist(&buf_cr_loc, 1, 0, mask, cr_hist, 1, &histSize, &histRange, true, false);
		calcHist(&buf_cb_loc, 1, 0, mask, cb_hist, 1, &histSize, &histRange, true, false);

		cv::UMat cr_hist_float, cb_hist_float;
		cv::normalize(cr_hist, cr_hist_float, 1, 0, cv::NORM_L2);
		cv::normalize(cb_hist, cb_hist_float, 1, 0, cv::NORM_L2);
		emit_output(OEIP_STAGE_HISTOGRAM_CR, OEIP_COLSPACE_HISTOGRAM, cr_hist_float);
		emit_output(OEIP_STAGE_HISTOGRAM_CB, OEIP_COLSPACE_HISTOGRAM, cb_hist_float);

		// Megkeressuk a ket foszint mindegyik csatornaban (kiveve luma)
		int cr0, cr1;
		int cb0, cb1;
		two_major_bins(cr_hist, &cr0, &cr1);
		two_major_bins(cb_hist, &cb0, &cb1);

		// Csatornankent megjeloljuk azon pixeleket, amelyek a ket foszint veszik fel
		// thresh_CH0 az elso, thresh_CH1 a masodik foszinhez tartozo maszk
		// thresh_CH a fenti ket maszk unioja

		cv::UMat thresh_cr0, thresh_cr1;
		inRange(buf_ycrcb_channels[1], cv::Scalar(cr0), cv::Scalar(cr0), thresh_cr0);
		inRange(buf_ycrcb_channels[1], cv::Scalar(cr1), cv::Scalar(cr1), thresh_cr1);
		cv::UMat thresh_cr;
		max(thresh_cr0, thresh_cr1, thresh_cr);

		cv::UMat thresh_cb0, thresh_cb1;
		inRange(buf_ycrcb_channels[2], cv::Scalar(cb0), cv::Scalar(cb0), thresh_cb0);
		inRange(buf_ycrcb_channels[2], cv::Scalar(cb1), cv::Scalar(cb1), thresh_cb1);
		cv::UMat thresh_cb;
		max(thresh_cb0, thresh_cb1, thresh_cb);

		// Vesszuk a ket thresh_CH maszk uniojat
		cv::UMat thresh(thresh_cr.rows, thresh_cr.cols, CV_32F);
		max(thresh_cr, thresh_cb, thresh);

		cv::UMat subtitle_mask;
		cv::UMat thresh_blur3, avg_bin_blur3;

		bitwise_and(edge_cur_bin, _mask_subtitle_bottom, edge_cur_bin);
		pyrDown(edge_cur_bin, edge_cur_bin, cv::Size(edge_cur_bin.cols / 2, edge_cur_bin.rows / 2));
		pyrDown(thresh, thresh, cv::Size(thresh.cols / 2, thresh.rows / 2));

		// A threshold maszkot es az edge buffert homalyositjuk, majd binarizaljuk
		boxFilter(thresh, thresh_blur3, thresh.type(), { 3, 3 });
		boxFilter(edge_cur_bin, avg_bin_blur3, edge_cur_bin.type(), { 3, 3 });

		threshold(avg_bin_blur3, avg_bin_blur3, 0, 255, _threshold_type);
		threshold(thresh_blur3, thresh_blur3, 0, 255, _threshold_type);

		// Vesszuk a threshold maszk es az edge buffer uniojat
		bitwise_and(avg_bin_blur3, thresh_blur3, subtitle_mask);

		if (_avg_subtitle_mask.empty()) {
			// Ha meg nincs felirat-maszk (legelso kepkocka), akkor bemasoljuk a jelenlegit
			// Az atlagszamitast nem rontja el, hiszen (x + x) / 2 == x
			subtitle_mask.copyTo(_avg_subtitle_mask);
		}

		// Vesszuk a regi es az uj felirat-maszk atlagat
		average_u8(_avg_subtitle_mask, subtitle_mask, _avg_subtitle_mask);

		return true;
	}

	bool step_impl() override {
		bool ret = true;
		cv::Mat buf;
		bool has_output_callback = _cb_output != nullptr;

		_video.read(buf);

		ret &= make_subtitle_mask(buf);

		emit_output(OEIP_STAGE_SUBTITLE_MASK, OEIP_COLSPACE_R8, _avg_subtitle_mask);

		cv::UMat buf_u, mask_u;
		buf.copyTo(buf_u);
		resize(_avg_subtitle_mask, mask_u, buf.size());
		oeip_inpaint_cvmat(_inpaint_res, buf_u, mask_u);

		emit_output(OEIP_STAGE_OUTPUT, OEIP_COLSPACE_RGB888_RGB, _inpaint_res);

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

		return ret;
	}

	void enable_otsu_binarization_impl() override {
		_threshold_type |= cv::THRESH_OTSU;
	}

	bool process_impl() override {
		cv::Mat buf;

		while (_video.read(buf)) {
			if (!make_subtitle_mask(buf)) {
				return false;
			}

            cv::UMat buf_u, mask_u;
            buf.copyTo(buf_u);
            resize(_avg_subtitle_mask, mask_u, buf.size());
            oeip_inpaint_cvmat(_inpaint_res, buf_u, mask_u);

			if (_output.has_value()) {
				_output->write(_inpaint_res);
			}

			if (_cb_progress != nullptr) {
				// Megprobaljuk meghivni a progress callback-et
				auto frameIdx = (int)_video.get(cv::CAP_PROP_POS_FRAMES);
				if ((frameIdx & _progress_callback_mask) == 0) {
					struct oeip_progress_info inf = { frameIdx, _total_frames };
					_cb_progress(&inf);
				}
			}
		}

		return true;
	}

	void set_progress_callback_mask(unsigned mask) override {
		_progress_callback_mask = mask;
	}

	// Veszi ket u8 matrix atlagat
	void average_u8(cv::UMat const& lhs, cv::UMat const& rhs, cv::UMat& out) {
		cv::UMat acc(lhs.size(), CV_64F, cv::Scalar(0));
		accumulate(lhs, acc);
		accumulate(rhs, acc);
		acc.convertTo(out, CV_8U, 1 / 2.0);
	}

	// Eldetektalast vegez el
	// Gauss + Sobel
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

	cv::UMat get_edge_buffer(cv::Mat const& buf) {
		auto edges = detect_edges(buf);
		emit_output(OEIP_STAGE_CURRENT_EDGE_BUFFER, OEIP_COLSPACE_R8, edges);
		return edges;
	}

	// Eltarolja az elonezeti buffert
	template<typename InputArray>
	void emit_output(oeip_stage stage, oeip_buffer_color_space cs, InputArray mat) {
		mat.copyTo(_output_buffers[stage]);
		_output_buffer_formats[stage] = cs;
	}

	// Atlagolja az edge-buffereket.
	cv::UMat average_edge_buffers() {
		// NOTE: maguk az edge bufferek 8U formatumban vannak es 0-255 normalizalt ertekek vannak benne
		// Ezeket akkumulaljuk 16U matrixba
		cv::UMat acc = cv::UMat::zeros(_edge_buffers[0].size(), CV_16U);

		auto temp_idx = (_edge_buffers_cursor + 1) % total_edge_buffers;
		auto const sum_of_weights = total_edge_buffers * (total_edge_buffers + 1) / 2;

		for (int i = 0; i < total_edge_buffers; i++) {
			cv::UMat eb;
            _edge_buffers[i].convertTo(eb, CV_16U);

			auto w = 1;
			scaleAdd(acc, w, eb, acc);
		}

		auto avg_tmp = cv::UMat::zeros(acc.rows, acc.cols, acc.type());
		scaleAdd(acc, 1 / (double)total_edge_buffers, avg_tmp, avg_tmp);
		return avg_tmp;
	}

private:
	bool _init = false;
	cv::VideoCapture _video;
	std::optional<cv::VideoWriter> _output;
	oeip_cb_output _cb_output;
	oeip_cb_progress _cb_progress;

	cv::UMat _mask_subtitle_bottom;
	cv::UMat _edge_buffers[total_edge_buffers];
	int _edge_buffers_cursor = 0;

	cv::Mat _output_buffers[oeip_stage::OEIP_STAGE_OUTPUT + 1];
	oeip_buffer_color_space _output_buffer_formats[oeip_stage::OEIP_STAGE_OUTPUT + 1];

	cv::UMat _avg_subtitle_mask;
	cv::Mat _inpaint_res;

	int _total_frames;
	unsigned _progress_callback_mask;
	int _threshold_type = cv::THRESH_BINARY;
};

std::unique_ptr<IOEIP> make_oeip(char const *pathToInput) {
	return make_oeip(pathToInput, nullptr);
}

std::unique_ptr<IOEIP> make_oeip(char const *pathToInput, char const *pathToOutput) {
	auto video = cv::VideoCapture(pathToInput);
	if (!video.isOpened()) {
		return nullptr;
	}

	std::optional<cv::VideoWriter> writer;

	if (pathToOutput != nullptr) {
		double width = video.get(cv::CAP_PROP_FRAME_WIDTH);
		double height = video.get(cv::CAP_PROP_FRAME_HEIGHT);
		auto size = cv::Size(width, height);
		double fps = video.get(cv::CAP_PROP_FPS);
		bool isColor = video.get(cv::CAP_PROP_MONOCHROME) > 0.0 ? false : true;
		writer = cv::VideoWriter(pathToOutput, cv::VideoWriter::fourcc('X', '2', '6', '4'), fps, size, isColor);

		if (writer.has_value() && !writer->isOpened()) {
			return nullptr;
		}
	}

	return std::make_unique<OEIP>(std::move(video), std::move(writer));
}