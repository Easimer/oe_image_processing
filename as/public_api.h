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

struct oeip_progress_info {
    int current_frame;
    int total_frames;
};

using oeip_cb_output = void (*)(oeip_stage stage, oeip_buffer_color_space cs, void const *buffer, int bytes, int width, int height, int stride);
using oeip_cb_progress = void (*)(struct oeip_progress_info const *progress);

PAPI oeip_handle oeip_open_video(char const *pathToInput, char const *pathToOutput);
PAPI void oeip_close_video(oeip_handle handle);

// Feldolgoz egy kepkockat a kimenetbol.
// - Az elkeszult kepkocka nem kerul kiirasba fajlba.
// - A Stage Output Callback-ek meghivasra kerulnek.
// - A folyamat sikeressegevel ter vissza.
PAPI bool oeip_step(oeip_handle handle);

// Feldolgozza az egesz bemenetet es kiirja az elkeszult videot a kimeneti fajlba.
// - A Stage Output Callback-ek NEM kerulnek meghivasra!
// - A Progress Callback meghivasra kerul egy-egy kepkocka feldolgozasa utan.
// - A folyamat sikeressegevel ter vissza.
PAPI bool oeip_process(oeip_handle handle);

PAPI bool oeip_register_stage_output_callback(oeip_handle handle, oeip_cb_output fun);
PAPI bool oeip_register_progress_callback(oeip_handle handle, oeip_cb_progress fun);

// Image inpainting debug/demo API

typedef struct oeip_inpainting_handle_ *oeip_inpainting_handle;

PAPI oeip_inpainting_handle oeip_begin_inpainting(char const *path_source, char const *path_mask);
PAPI void oeip_end_inpainting(oeip_inpainting_handle handle);
PAPI void oeip_inpaint(oeip_inpainting_handle handle, void const **buffer, int *bytes, int *width, int *height, int *stride);
