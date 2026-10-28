// SIFT descriptor extraction with OpenCV (section II.C). The same code is used for the
// HPatches .ppm images and the MIRFlickr .jpg images

#pragma once

#include <opencv2/core.hpp>
#include <string>

// Dimension of a SIFT descriptor
constexpr int kSiftDim = 128;

// Reads the image in grayscale and returns its SIFT descriptors: one row of 128 floats
// (CV_32F) per keypoint. Returns a 0-row matrix if no keypoint is found
// Throws std::runtime_error if the image cannot be read. Safe to call from several threads
cv::Mat extractSift(const std::string& path);
