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
		
		_video.read(buf);

		if (buf.empty()) {
			return false;
		}

		cv::Mat buf_gray;

		cv::cvtColor(buf, buf_gray, cv::COLOR_BGR2GRAY);

		if (_cb_output != nullptr) {
			emit_output(OEIP_STAGE_INPUT, OEIP_COLSPACE_1D_GRAY, buf_gray);
		}

		return true;
	}

	template<typename Mat>
	void emit_output(oeip_stage stage, oeip_buffer_color_space cs, Mat const& mat) {
		auto ptr = mat.ptr<unsigned char>();
		auto width = mat.cols;
		auto height = mat.rows;
		auto stride = mat.step[0] * mat.elemSize();
		_cb_output(stage, cs, ptr, height * stride, width, height, stride);
	}

private:
	cv::VideoCapture _video;
	oeip_cb_output _cb_output;
	oeip_cb_benchmark _cb_benchmark;
};

std::unique_ptr<IOEIP> make_oeip(char const* pathToVideo) {
	return std::make_unique<OEIP>(pathToVideo);
}