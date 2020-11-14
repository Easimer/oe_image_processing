#pragma once

#define PAPI extern "C"

typedef struct oeip_handle_ *oeip_handle;

enum oeip_stage : int {
    OEIP_STAGE_INPUT = 0,

    OEIP_STAGE_CURRENT_EDGE_BUFFER,
    OEIP_STAGE_ACCUMULATED_EDGE_BUFFER,
    OEIP_STAGE_SUBTITLE_MASK,

    OEIP_STAGE_OUTPUT
};

enum oeip_buffer_color_space : int {
    OEIP_COLSPACE_R8 = 0,
    OEIP_COLSPACE_R16,
    OEIP_COLSPACE_RGB888_RGB,
    OEIP_COLSPACE_RGB888_YCBCR,

    OEIP_COLSPACE_RGB888_UNSPEC
};

using oeip_cb_output = void (*)(oeip_stage stage, oeip_buffer_color_space cs, void const *buffer, int bytes, int width, int height, int stride);
using oeip_cb_benchmark = void (*)(oeip_stage stage, unsigned microsecs);

PAPI oeip_handle oeip_open_video(char const *path);
PAPI void oeip_close_video(oeip_handle handle);

PAPI bool oeip_step(oeip_handle handle);

PAPI bool oeip_register_stage_output_callback(oeip_handle handle, oeip_cb_output fun);
PAPI bool oeip_register_stage_benchmark_callback(oeip_handle handle, oeip_cb_benchmark fun);

// Image inpainting debug/demo API

typedef struct oeip_inpainting_handle_ *oeip_inpainting_handle;

PAPI oeip_inpainting_handle oeip_begin_inpainting(char const *path_source, char const *path_mask);
PAPI void oeip_end_inpainting(oeip_inpainting_handle handle);
PAPI void oeip_inpaint(oeip_inpainting_handle handle, void const **buffer, int *bytes, int *width, int *height, int *stride);
