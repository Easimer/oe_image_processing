#include "oeip.h"
#include <opencv2/videoio.hpp>
#include <opencv2/imgproc.hpp>

class OEIP : public IOEIP {
public:
	OEIP(char const* pathToVideo) :
		_video(cv::VideoCapture(pathToVideo)),
		_cb_output(nullptr),
		_cb_benchmark(nullptr)
	{
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
		cv::UMat edge_cur, edge_buf;
		bool has_output_callback = _cb_output != nullptr;
		
		_video.read(buf);

		if (buf.empty()) {
			return false;
		}

		cv::Mat buf_gray;

		cv::cvtColor(buf, buf_gray, cv::COLOR_BGR2GRAY);

		if (has_output_callback) {
			emit_output(OEIP_STAGE_INPUT, OEIP_COLSPACE_1D_GRAY, buf_gray);
		}

		edge_cur = get_edge_buffer(buf);

		if (has_output_callback) {
			emit_output(OEIP_STAGE_CURRENT_EDGE_BUFFER, OEIP_COLSPACE_1D_GRAY, edge_cur);
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

	void emit_output(oeip_stage stage, oeip_buffer_color_space cs, cv::Mat const& mat) {
		auto ptr = mat.ptr<unsigned char>();
		auto width = mat.cols;
		auto height = mat.rows;
		auto stride = mat.step[0] * mat.elemSize();
		_cb_output(stage, cs, ptr, height * stride, width, height, stride);
	}

	void emit_output(oeip_stage stage, oeip_buffer_color_space cs, cv::UMat const& mat) {
		cv::Mat local;
		mat.copyTo(local);
		emit_output(stage, cs, local);
	}

private:
	cv::VideoCapture _video;
	oeip_cb_output _cb_output;
	oeip_cb_benchmark _cb_benchmark;
};

std::unique_ptr<IOEIP> make_oeip(char const* pathToVideo) {
	return std::make_unique<OEIP>(pathToVideo);
}