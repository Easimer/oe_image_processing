#include "public_api.h"
#include "image_inpainting.h"

#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

struct oeip_inpainting_handle_ {
    cv::Mat source, mask;
    cv::Mat result;
};

PAPI oeip_inpainting_handle oeip_begin_inpainting(char const *path_source, char const *path_mask) {
    assert(path_source != nullptr);
    assert(path_mask != nullptr);

    if (path_source == nullptr || path_mask == nullptr) {
        return nullptr;
    }

    auto source = cv::imread(path_source);
    auto mask = cv::imread(path_mask);
    
    if (source.empty() || mask.empty()) {
        return nullptr;
    }

    auto ret = new oeip_inpainting_handle_;
    ret->source = std::move(source);
    ret->mask = std::move(mask);

    return ret;
}

PAPI void oeip_end_inpainting(oeip_inpainting_handle handle) {
    assert(handle != nullptr);

    if (handle == nullptr) {
        return;
    }

    delete handle;
}

PAPI void oeip_inpaint(oeip_inpainting_handle handle, void const **buffer, int *bytes, int *width, int *height, int *stride) {
    assert(handle != nullptr);
    assert(buffer != nullptr);
    assert(bytes != nullptr);
    assert(width != nullptr);
    assert(height != nullptr);
    assert(stride != nullptr);

    if (buffer == nullptr) {
        return;
    }

    *buffer = nullptr;

    if (handle == nullptr || bytes == nullptr || width == nullptr || height == nullptr || stride == nullptr) {
        return;
    }

    auto &res = handle->result;

    if (!res.empty()) {
        auto buf_ptr = res.ptr<unsigned char>();
        auto buf_width = res.cols;
        auto buf_height = res.rows;
        auto buf_stride = res.step[0];
        *buffer = buf_ptr;
        *bytes = buf_height * buf_stride;
        *width = buf_width;
        *height = buf_height;
        *stride = buf_stride;
    }
}
