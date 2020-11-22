#include "public_api.h"
#include "image_inpainting.h"

#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/photo.hpp>

struct oeip_inpainting_handle {
    cv::UMat source, mask;
    cv::Mat res;
};

void oeip_inpaint_cvmat(cv::Mat &res, cv::UMat const &src, cv::UMat const &mask) {
    cv::inpaint(src, mask, res, 2.4, cv::INPAINT_NS);
}

PAPI HOEIPINPAINT oeip_begin_inpainting(char const *path_source, char const *path_mask) {
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

    if (source.size != mask.size) {
        return nullptr;
    }

    auto ret = new oeip_inpainting_handle;
    source.copyTo(ret->source);
    cv::extractChannel(mask, ret->mask, 0);

    return ret;
}

PAPI void oeip_end_inpainting(HOEIPINPAINT handle) {
    assert(handle != nullptr);

    if (handle == nullptr) {
        return;
    }

    delete handle;
}

template<typename T>
static T sample_matrix_repeat(cv::Mat const &m, int i, int j) {
    auto wm = m.cols - 1;
    auto hm = m.rows - 1;
    if (i < 0) i = 0;
    if (j < 0) j = 0;
    if (i > wm) i = wm;
    if (j > hm) j = hm;

    return m.at<T>(j, i);
}

static float dot(cv::Vec2s const &lhs, cv::Vec2f const &rhs) {
    return (float)lhs[0] * rhs[0] + (float)lhs[1] * rhs[1];
}

template<int Component>
struct Forward_Delta {
    int operator()(cv::Mat const &m, int i, int j) {
        if constexpr (Component == 0) {
            return sample_matrix_repeat<unsigned char>(m, i + 1, j) - sample_matrix_repeat<unsigned char>(m, i - 1, j);
        } else {
            return sample_matrix_repeat<unsigned char>(m, i, j + 1) - sample_matrix_repeat<unsigned char>(m, i, j - 1);
        }
    }
};

template<int Component>
struct Backward_Delta {
    unsigned char operator()(cv::Mat const &m, int i, int j) {
        if constexpr (Component == 0) {
            return sample_matrix_repeat<unsigned char>(m, i - 1, j) - sample_matrix_repeat<unsigned char>(m, i + 1, j);
        } else {
            return sample_matrix_repeat<unsigned char>(m, i, j - 1) - sample_matrix_repeat<unsigned char>(m, i, j + 1);
        }
    }
};

struct Min_Delta {
    int operator()(int v) {
        return (0 < v) ? 0 : v;
    }
};

struct Max_Delta {
    int operator()(int v) {
        return (v < 0) ? 0 : v;
    }
};

template<typename Dir, typename Z>
int I_delta(cv::Mat const &m, int i, int j) {
    Z z;
    Dir d;

    return z(d(m, i, j));
}

static void f(cv::Mat &res, cv::Mat const &src, cv::Mat const &mask) {
    cv::Mat dLx, dLy;
    cv::Mat L;
    int ddepth = CV_16S;
    cv::Laplacian(src, L, ddepth);

    cv::Sobel(L, dLx, CV_16S, 1, 0);
    cv::Sobel(L, dLy, CV_16S, 0, 1);

    cv::Mat dL(src.rows, src.cols, CV_16SC2);

    for (int j = 0; j < src.rows; j++) {
        for (int i = 0; i < src.cols; i++) {
            short dx = sample_matrix_repeat<short>(L, i + 1, j) - sample_matrix_repeat<short>(L, i - 1, j);
            short dy = sample_matrix_repeat<short>(L, i, j + 1) - sample_matrix_repeat<short>(L, i, j - 1);
            dL.at<cv::Vec2s>(j, i) = { dx, dy };
        }
    }

    cv::Mat N(src.rows, src.cols, CV_32FC2);

    for (int j = 0; j < src.rows; j++) {
        for (int i = 0; i < src.cols; i++) {
            auto Nx = -dLy.at<short>(j, i);
            auto Ny = -dLx.at<short>(j, i);
            auto Nlen = sqrtf(Nx * Nx + Ny * Ny);
            if (Nlen != 0) {
                N.at<cv::Vec2f>(j, i) = { Nx / Nlen, Ny / Nlen };
            } else {
                N.at<cv::Vec2f>(j, i) = { 0, 0 };
            }
        }
    }

    cv::Mat beta(src.rows, src.cols, CV_32FC1);

    for (int j = 0; j < src.rows; j++) {
        for (int i = 0; i < src.cols; i++) {
            beta.at<float>(j, i) = 0;
            if (mask.at<unsigned char>(j, i) != 0x00) {
                continue;
            }

            beta.at<float>(j, i) = dot(dL.at<cv::Vec2s>(j, i), N.at<cv::Vec2f>(j, i));
        }
    }

    cv::Mat dI(src.rows, src.cols, CV_32FC1);

    for (int j = 0; j < src.rows; j++) {
        for (int i = 0; i < src.cols; i++) {
            dI.at<float>(j, i) = 0;
            if (mask.at<unsigned char>(j, i) != 0x00) {
                continue;
            }

            float B = beta.at<float>(j, i);

            float I;
            int c0, c1, c2, c3;

            if (B >= 0) {
                c0 = I_delta<Backward_Delta<0>, Min_Delta>(src, i, j);
                c1 = I_delta<Forward_Delta<0>, Max_Delta>(src, i, j);
                c2 = I_delta<Backward_Delta<1>, Min_Delta>(src, i, j);
                c3 = I_delta<Forward_Delta<1>, Max_Delta>(src, i, j);
            } else {
                c0 = I_delta<Backward_Delta<0>, Max_Delta>(src, i, j);
                c1 = I_delta<Forward_Delta<0>, Min_Delta>(src, i, j);
                c2 = I_delta<Backward_Delta<1>, Max_Delta>(src, i, j);
                c3 = I_delta<Forward_Delta<1>, Min_Delta>(src, i, j);
            }

            I = sqrtf(c0 * c0 + c1 * c1 + c2 * c2 + c3 * c3);

            dI.at<float>(j, i) = I;
        }
    }

    cv::Mat It(src.rows, src.cols, CV_32FC1);

    for (int j = 0; j < src.rows; j++) {
        for (int i = 0; i < src.cols; i++) {
            if (mask.at<unsigned char>(j, i) == 0x00) {
                It.at<float>(j, i) = beta.at<float>(j, i) * dI.at<float>(j, i);
            } else {
                It.at<float>(j, i) = 0.0f;
            }
        }
    }

    float It_max = It.at<float>(0, 0);
    for (int j = 0; j < src.rows; j++) {
        for (int i = 0; i < src.cols; i++) {
            auto v = fabs(It.at<float>(j, i));
            It_max = (It_max < v) ? v : It_max;
        }
    }

    for (int j = 0; j < src.rows; j++) {
        for (int i = 0; i < src.cols; i++) {
            auto& v = It.at<float>(j, i);
            v /= It_max;
        }
    }
}

PAPI void oeip_inpaint(HOEIPINPAINT handle, void const **buffer, int *bytes, int *width, int *height, int *stride) {
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

    oeip_inpaint_cvmat(handle->res, handle->source, handle->mask);

    auto &res = handle->res;
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
