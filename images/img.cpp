#include "img.hpp"
#include <iostream>
#include <stdexcept>

Img::Img() {
    // Constructor - img is automatically initialized as empty
}

Img& Img::read(const std::string& path,
               const std::pair<int, int>& size,
               bool keep_aspect,
               int interpolation) {
    img = cv::imread(path, cv::IMREAD_UNCHANGED);
    if (img.empty()) {
        throw std::runtime_error("Cannot load image: " + path);
    }

    if (size.first != 0 && size.second != 0) {  // Check if size is not empty
        int target_w = size.first;
        int target_h = size.second;
        int h = img.rows;
        int w = img.cols;

        if (keep_aspect) {
            double scale = std::min(static_cast<double>(target_w) / w, 
                                   static_cast<double>(target_h) / h);
            int new_w = static_cast<int>(w * scale);
            int new_h = static_cast<int>(h * scale);
            cv::resize(img, img, cv::Size(new_w, new_h), 0, 0, interpolation);
        } else {
            cv::resize(img, img, cv::Size(target_w, target_h), 0, 0, interpolation);
        }
    }

    return *this;
}

void Img::draw_on(Img& other_img, int x, int y) {
    if (img.empty() || other_img.img.empty()) {
        throw std::runtime_error("Both images must be loaded before drawing.");
    }

    cv::Mat source_img = img;
    cv::Mat target_img = other_img.img;

    int h = source_img.rows;
    int w = source_img.cols;
    int H = target_img.rows;
    int W = target_img.cols;

    if (y + h > H || x + w > W) {
        throw std::runtime_error("Image does not fit at the specified position.");
    }

    cv::Mat roi = target_img(cv::Rect(x, y, w, h));

    if (source_img.channels() != 4) {
        cv::Mat opaque = source_img;
        if (opaque.channels() != target_img.channels()) {
            cv::cvtColor(opaque, opaque,
                         target_img.channels() == 4 ? cv::COLOR_BGR2BGRA
                                                    : cv::COLOR_BGRA2BGR);
        }
        opaque.copyTo(roi);
        return;
    }

    // Blend the source over the region it covers, per pixel and per colour
    // channel: out = src * alpha + dst * (1 - alpha). The alpha is scaled into
    // floats first, because dividing the 8-bit channel would round every pixel
    // to a flat 0 or 1 and throw the gradations away.
    std::vector<cv::Mat> source_channels;
    cv::split(source_img, source_channels);

    cv::Mat alpha;
    source_channels[3].convertTo(alpha, CV_32F, 1.0 / 255.0);
    cv::Mat inverse_alpha = 1.0 - alpha;

    std::vector<cv::Mat> roi_channels;
    cv::split(roi, roi_channels);

    for (int c = 0; c < 3; ++c) {
        cv::Mat source_c;
        cv::Mat target_c;
        source_channels[c].convertTo(source_c, CV_32F);
        roi_channels[c].convertTo(target_c, CV_32F);

        cv::Mat blended = source_c.mul(alpha) + target_c.mul(inverse_alpha);
        blended.convertTo(roi_channels[c], roi_channels[c].type());
    }

    // A transparent target pixel must end up as opaque as what was drawn onto
    // it, otherwise the result stays invisible on a transparent background.
    if (roi_channels.size() == 4) {
        cv::Mat target_alpha;
        roi_channels[3].convertTo(target_alpha, CV_32F, 1.0 / 255.0);
        cv::Mat blended_alpha = alpha + target_alpha.mul(inverse_alpha);
        blended_alpha.convertTo(roi_channels[3], roi_channels[3].type(), 255.0);
    }

    cv::Mat blended_roi;
    cv::merge(roi_channels, blended_roi);
    blended_roi.copyTo(roi);
}

void Img::put_text(const std::string& txt, int x, int y, double font_size,
                   const cv::Scalar& color, int thickness) {
    if (img.empty()) {
        throw std::runtime_error("Image not loaded.");
    }
    
    cv::putText(img, txt, cv::Point(x, y),
                cv::FONT_HERSHEY_SIMPLEX, font_size,
                color, thickness, cv::LINE_AA);
}

void Img::show() {
    if (img.empty()) {
        throw std::runtime_error("Image not loaded.");
    }
    
    cv::imshow("Image", img);
    cv::waitKey(0);
    cv::destroyAllWindows();
} 