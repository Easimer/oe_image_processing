#include <memory>

#include "public_api.h"
#include "oeip.h"
#include <cassert>

struct oeip_handle_ {
	std::unique_ptr<IOEIP> oeip;
};

PAPI oeip_handle oeip_open_video(char const *pathToInput, char const *pathToOutput) {
	if (pathToInput == nullptr) {
		return nullptr;
	}

	auto handle = new oeip_handle_;
	handle->oeip = make_oeip(pathToInput, pathToOutput);

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

	if (handle->oeip == nullptr) {
		return false;
	}

	return handle->oeip->step();
}

PAPI bool oeip_process(oeip_handle handle) {
	if (handle == nullptr) {
		return false;
	}

	assert(handle->oeip != nullptr);

	if (handle->oeip == nullptr) {
		return false;
	}

	return handle->oeip->process();
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

PAPI bool oeip_register_progress_callback(oeip_handle handle, oeip_cb_progress fun) {
	if (handle == nullptr) {
		return false;
	}

	assert(handle->oeip != nullptr);

	if (handle->oeip == nullptr) {
		return false;
	}

	handle->oeip->register_progress_callback(fun);
	return true;
}