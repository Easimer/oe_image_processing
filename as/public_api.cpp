#include <memory>

#include "public_api.h"
#include "oeip.h"
#include <cassert>

struct oeip_handle_ {
	std::unique_ptr<IOEIP> oeip;
};

PAPI oeip_handle oeip_open_video(char const* path) {
	if (path == nullptr) {
		return nullptr;
	}

	auto handle = new oeip_handle_;
	handle->oeip = make_oeip(path);

	if (handle->oeip == nullptr) {
		delete handle;
		return nullptr;
	}

	return handle;
}

PAPI void oeip_close_video(oeip_handle handle) {
	if (handle == nullptr) {
		return;
	}

	assert(handle->oeip != nullptr);

	delete handle;
}

PAPI bool oeip_step(oeip_handle handle) {
	if (handle == nullptr) {
		return false;
	}

	assert(handle->oeip != nullptr);

	return handle->oeip->step();
}

PAPI bool oeip_register_stage_output_callback(oeip_handle handle, oeip_cb_output fun) {
	if (handle == nullptr) {
		return false;
	}

	assert(handle->oeip != nullptr);

	if (handle->oeip == nullptr) {
		return false;
	}

	handle->oeip->register_stage_output_callback(fun);
	return true;
}

PAPI bool oeip_register_stage_benchmark_callback(oeip_handle handle, oeip_cb_benchmark fun) {
	if (handle == nullptr) {
		return false;
	}

	assert(handle->oeip != nullptr);

	if (handle->oeip == nullptr) {
		return false;
	}

	handle->oeip->register_stage_benchmark_callback(fun);
	return true;
}