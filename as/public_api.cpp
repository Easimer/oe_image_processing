#include <memory>

#include "public_api.h"
#include "oeip.h"
#include <cassert>

struct oeip_handle {
	std::unique_ptr<IOEIP> oeip;
};

PAPI HOEIP oeip_open_video(char const *pathToInput, char const *pathToOutput, int flags) {
	if (pathToInput == nullptr) {
		return nullptr;
	}

	auto handle = new oeip_handle;
	handle->oeip = make_oeip(pathToInput, pathToOutput);

	if (handle->oeip == nullptr) {
		delete handle;
		return nullptr;
	}

	if (flags & OEIP_FLAGS_APPLY_OTSU_BINARIZATION) {
		handle->oeip->enable_otsu_binarization();
	}

	return handle;
}

PAPI void oeip_close_video(HOEIP handle) {
	if (handle == nullptr) {
		return;
	}

	assert(handle->oeip != nullptr);

	delete handle;
}

PAPI bool oeip_step(HOEIP handle) {
	if (handle == nullptr) {
		return false;
	}

	assert(handle->oeip != nullptr);

	if (handle->oeip == nullptr) {
		return false;
	}

	return handle->oeip->step();
}

PAPI bool oeip_process(HOEIP handle) {
	if (handle == nullptr) {
		return false;
	}

	assert(handle->oeip != nullptr);

	if (handle->oeip == nullptr) {
		return false;
	}

	return handle->oeip->process();
}

PAPI bool oeip_register_stage_output_callback(HOEIP handle, oeip_cb_output fun) {
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

PAPI bool oeip_register_progress_callback(HOEIP handle, oeip_cb_progress fun, unsigned mask) {
	if (handle == nullptr) {
		return false;
	}

	assert(handle->oeip != nullptr);

	if (handle->oeip == nullptr) {
		return false;
	}

	handle->oeip->register_progress_callback(fun, mask);
	return true;
}