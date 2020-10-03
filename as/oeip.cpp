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
			auto ptr = buf_gray.ptr<unsigned char>();
			auto width = buf_gray.cols;
			auto height = buf_gray.rows;
			auto stride = buf_gray.step[0] * buf_gray.elemSize();
			_cb_output(OEIP_STAGE_INPUT, OEIP_COLSPACE_1D_GRAY, ptr, height * stride, width, height, stride);
		}

		return true;
	}

private:
	cv::VideoCapture _video;
	oeip_cb_output _cb_output;
	oeip_cb_benchmark _cb_benchmark;
};

std::unique_ptr<IOEIP> make_oeip(char const* pathToVideo) {
	return std::make_unique<OEIP>(pathToVideo);
}