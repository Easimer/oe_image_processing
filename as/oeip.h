#pragma once

#include <memory>
#include "public_api.h"

class IOEIP {
public:
	virtual ~IOEIP() = default;

	void register_stage_output_callback(oeip_cb_output fun) {
		register_stage_output_callback_impl(fun);
	}

	void register_stage_benchmark_callback(oeip_cb_benchmark fun) {
		register_stage_benchmark_callback_impl(fun);
	}

	bool step() {
		return step_impl();
	}
	
protected:
	virtual void register_stage_output_callback_impl(oeip_cb_output fun) = 0;
	virtual void register_stage_benchmark_callback_impl(oeip_cb_benchmark fun) = 0;

	virtual bool step_impl() = 0;
};

std::unique_ptr<IOEIP> make_oeip(char const* pathToVideo);