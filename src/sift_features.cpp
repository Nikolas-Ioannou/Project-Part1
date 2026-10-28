// SIFT extraction 

#include "sift_features.hpp"

#include <opencv2/features2d.hpp>
#include <opencv2/imgcodecs.hpp>
#include <stdexcept>
#include <vector>

using namespace std;

cv::Mat extractSift(const string& path) {
    // Exactly as in the assignment: both .ppm and .jpg are read this way
    cv::Mat image = cv::imread(path, cv::IMREAD_GRAYSCALE);
    if (image.empty()) throw runtime_error("cannot read image " + path);
    // One detector per thread, since images are processed in parallel
    // Default OpenCV SIFT parameters (all keypoints, no limit on their number)
    thread_local cv::Ptr<cv::SIFT> sift = cv::SIFT::create();
    vector<cv::KeyPoint> keypoints;
    cv::Mat descriptors;
    sift->detectAndCompute(image, cv::noArray(), keypoints, descriptors);
    if (descriptors.empty()) return cv::Mat(0, kSiftDim, CV_32F);
    return descriptors;
}
