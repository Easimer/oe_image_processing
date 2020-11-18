#pragma once

#include <opencv2/imgproc.hpp>

void oeip_inpaint_cvmat(cv::Mat &res, cv::UMat const &src, cv::UMat const &mask);

