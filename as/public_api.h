#pragma once

#define PAPI extern "C"

typedef struct oeip_handle *HOEIP;

/**
 * A kepfeldolgozasi folyamat fazisai.
 */
enum oeip_stage : int {
    OEIP_STAGE_INPUT = 0,

    OEIP_STAGE_CURRENT_EDGE_BUFFER,
    OEIP_STAGE_ACCUMULATED_EDGE_BUFFER,
    OEIP_STAGE_SUBTITLE_MASK,
    OEIP_STAGE_HISTOGRAM_CR,
    OEIP_STAGE_HISTOGRAM_CB,

    OEIP_STAGE_OUTPUT
};

/**
 * Stage Output Callback-ek altal atadott kepi adatok lehetseges formatumai.
 */
enum oeip_buffer_color_space : int {
    OEIP_COLSPACE_R8 = 0,
    OEIP_COLSPACE_R16,
    OEIP_COLSPACE_RGB888_RGB,
    OEIP_COLSPACE_RGB888_YCBCR,

    OEIP_COLSPACE_RGB888_UNSPEC,

    OEIP_COLSPACE_HISTOGRAM
};

/**
 * Konfiguracios flag-ek.
 */
enum oeip_flags : int {
    OEIP_FLAGS_NONE = 0,
    OEIP_FLAGS_APPLY_OTSU_BINARIZATION = 1 << 0,
};

/**
 * Progress Callback altal atadott progress informacio.
 */
struct oeip_progress_info {
    int current_frame;
    int total_frames;
};

/**
 * Stage Output Callback
 *
 * @param stage A jelenlegi fazis
 * @param cs A kepi adat formatuma
 * @param buffer Pointer a kepi adatra
 * @param bytes A kepi adat merete
 * @param width A kep szelessege
 * @param height A kep magassaga
 * @param stride A kep ket soranak eleje kozotti tavolsag bajtban
 */
using oeip_cb_output = void (*)(oeip_stage stage, oeip_buffer_color_space cs, void const *buffer, int bytes, int width, int height, int stride);

/**
 * Progress Callback
 *
 * @param progress Pointer a progress informaciora
 */
using oeip_cb_progress = void (*)(struct oeip_progress_info const *progress);

/**
 * Megnyit egy videot olvasasra es opcionalisan megegyet irasra.
 * 
 * @param pathToInput Utvonal a bemeneti videora. Nem lehet NULL.
 * @param pathToOutput Utvonal a kimeneti videora. Lehet NULL.
 * @param flags Konfiguracios flag-ek
 *
 * @return Visszater egy handle-el a session-re, vagy hiba eseten NULL-al.
 *
 * @note Lehetseges hibak: a bemeneti video utvonala NULL vagy nem lehetett
 * megnyitni a fajlt; a kimeneti video utvonala nem NULL, de a fajlt nem
 * lehetett megnyitni.
 */
PAPI HOEIP oeip_open_video(char const *pathToInput, char const *pathToOutput, int flags);

/**
 * Bezarja a session-t es felszabaditja a lefoglalt eroforrasokat.
 *
 * @param handle Session handle. Nem lehet NULL.
 */
PAPI void oeip_close_video(HOEIP handle);

/**
 * Single-step-eli a session-t.
 *
 * @param handle Session handle
 *
 * @return A folyamat sikeressegevel ter vissza.
 *
 * @note Az elkeszult kepkocka NEM kerul kiirasra a kimeneti fajlba.
 * @note A Stage Output Callback meghivasra kerul.
 * @note A Progress Callback NEM kerul meghivasra.
 */
PAPI bool oeip_step(HOEIP handle);

/**
 * Feldolgozza a bemeneti videot.
 *
 * @param handle Session handle
 *
 * @return A folyamat sikeressegevel ter vissza.
 *
 * @note Az elkeszult kepkockak kiirasra kerulnek a kimeneti fajlba.
 * @note Akkor is vegrehajtasra kerul a munka, ha nem volt megadva kimeneti fajl.
 * @note A Progress Callback meghivasra kerul.
 * @note A Stage Output Callback NEM kerul meghivasra.
 */
PAPI bool oeip_process(HOEIP handle);

/**
 * Beallitja a session-hoz tartozo Stage Output Callback fuggvenyt.
 *
 * @param handle Session handle
 * @param fun A callback fuggveny. Lehet NULL.
 *
 * @note NULL fuggveny megadasaval torolheto egy korabbi regisztracio.
 */
PAPI bool oeip_register_stage_output_callback(HOEIP handle, oeip_cb_output fun);

/**
 * Beallitja a session-hoz tartozo Progress Callback fuggvenyt.
 *
 * @param handle Session handle
 * @param fun A callback fuggveny. Lehet NULL.
 * @param mask Frame index maszk. A callback csak akkor kerul meghivasra, ha
 * `(frame index & maszk) == 0`.
 *
 * @note NULL fuggveny megadasaval torolheto egy korabbi regisztracio.
 * @note A mask parameterrel csokkentheto a callback hivasok szama.
 */
PAPI bool oeip_register_progress_callback(HOEIP handle, oeip_cb_progress fun, unsigned mask);

// Image inpainting debug/demo API

typedef struct oeip_inpainting_handle *HOEIPINPAINT;

PAPI HOEIPINPAINT oeip_begin_inpainting(char const *path_source, char const *path_mask);
PAPI void oeip_end_inpainting(HOEIPINPAINT handle);
PAPI void oeip_inpaint(HOEIPINPAINT handle, void const **buffer, int *bytes, int *width, int *height, int *stride);
