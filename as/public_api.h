#pragma once

#define PAPI extern "C"

typedef struct oeip_handle_ *oeip_handle;

enum oeip_stage : int {
    OEIP_STAGE_INPUT = 0,

    OEIP_STAGE_CURRENT_EDGE_BUFFER,
    OEIP_STAGE_ACCUMULATED_EDGE_BUFFER,

    OEIP_STAGE_OUTPUT
};

enum oeip_buffer_color_space : int {
    OEIP_COLSPACE_1D_GRAY = 0,
    OEIP_COLSPACE_3D_RGB,
    OEIP_COLSPACE_3D_YCBCR,

    OEIP_COLSPACE_3D_UNSPEC
};

using oeip_cb_output = void (*)(oeip_stage stage, oeip_buffer_color_space cs, void const *buffer, int bytes);
using oeip_cb_benchmark = void (*)(oeip_stage stage, unsigned microsecs);

PAPI oeip_handle oeip_open_video(char const *path);
PAPI void oeip_close_video(oeip_handle handle);

PAPI bool oeip_step(oeip_handle handle);

PAPI bool oeip_register_stage_output_callback(oeip_handle handle, oeip_cb_output fun);
PAPI bool oeip_register_stage_benchmark_callback(oeip_handle handle, oeip_cb_benchmark fun);
