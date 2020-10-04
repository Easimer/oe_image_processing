#include "oeip.h"
#include <opencv2/highgui.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/imgproc.hpp>

static constexpr int total_edge_buffers = 16;

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

		for (int i = 0; i < total_edge_buffers; i++) {
			cv::Mat tmp;
			_edge_buffers[i] = cv::UMat(height, width, CV_16S);
			//_video.read(tmp);
			//_edge_buffers[i] = get_edge_buffer(tmp);
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
			accumulate(_edge_buffers[i], acc);
		}

		cv::UMat avg, avg_bin;

		acc.convertTo(avg, CV_8U, 1 / (float)total_edge_buffers);
		threshold(avg, avg_bin, 252, 255, cv::THRESH_BINARY);

		subtract(_edge_buffers[_edge_buffers_cursor], avg_bin, _edge_buffers[_edge_buffers_cursor], cv::noArray(), CV_16S);		

		emit_output(OEIP_STAGE_ACCUMULATED_EDGE_BUFFER, OEIP_COLSPACE_R8, avg_bin);

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

	cv::UMat _edge_buffers[total_edge_buffers];
	int _edge_buffers_cursor = 0;

	cv::Mat _output_buffers[oeip_stage::OEIP_STAGE_OUTPUT + 1];
	oeip_buffer_color_space _output_buffer_formats[oeip_stage::OEIP_STAGE_OUTPUT + 1];
};

std::unique_ptr<IOEIP> make_oeip(char const* pathToVideo) {
	return std::make_unique<OEIP>(pathToVideo);
}