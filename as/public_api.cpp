#include "public_api.h"

struct oeip_handle_ {

};

PAPI oeip_handle oeip_open_video(char const* path) {
	if (path == nullptr) {
		return nullptr;
	}

	return new oeip_handle_;
}

PAPI void oeip_close_video(oeip_handle handle) {
	if (handle == nullptr) {
		return;
	}

	delete handle;
}

PAPI bool oeip_step(oeip_handle handle) {
	if (handle == nullptr) {
		return false;
	}

	return false;
}

PAPI bool oeip_register_stage_output_callback(oeip_handle handle, oeip_cb_output fun) {
	if (handle == nullptr) {
		return false;
	}

	

	return false;
}

PAPI bool oeip_register_stage_benchmark_callback(oeip_handle handle, oeip_cb_benchmark fun) {
	if (handle == nullptr) {
		return false;
	}

	

	return false;
}