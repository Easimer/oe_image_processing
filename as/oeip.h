#pragma once

#include <memory>
#include "public_api.h"

class IOEIP {
public:
	virtual ~IOEIP() = default;

	void register_stage_output_callback(oeip_cb_output fun) {
		register_stage_output_callback_impl(fun);
	}

	void register_progress_callback(oeip_cb_progress fun, unsigned mask) {
		register_progress_callback_impl(fun);
		set_progress_callback_mask(mask);
	}

	bool step() {
		return step_impl();
	}

	bool process() {
		return process_impl();
	}
	
protected:
	virtual void register_stage_output_callback_impl(oeip_cb_output fun) = 0;
	virtual void register_progress_callback_impl(oeip_cb_progress fun) = 0;
	virtual void set_progress_callback_mask(unsigned mask) = 0;

	virtual bool step_impl() = 0;
	virtual bool process_impl() = 0;
};

std::unique_ptr<IOEIP> make_oeip(char const *pathToInput);
std::unique_ptr<IOEIP> make_oeip(char const *pathToInput, char const *pathToOutput);