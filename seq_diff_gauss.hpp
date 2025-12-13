#ifndef SEQ_DIFF_GAUSS_H
#define SEQ_DIFF_GAUSS_H

#include <vector>
#include <cmath>
#include <algorithm>
#include "file_manager.h" 

struct Image {
    int width;
    int height;
    std::vector<float> data; 

    Image(int w, int h) : width(w), height(h), data(w * h) {}
    
    void resize(int w, int h) {
        width = w;
        height = h;
        if (data.size() != static_cast<size_t>(w * h)) {
            data.resize(w * h);
        }
    }
};

// Internal helper for buffer reuse
void GaussianBlurRaw(const Image& input, Image& output, Image& tempBuffer, float sigma);

Image applyDoG(const Image& input, float sigma, float k, float tau);
Image applyXDoG(const Image& input, float sigma, float k, float tau, float epsilon, float phi);

Image convertToFloatImage(const FileManager& fm);
FileManager convertToFMImage(const Image& img);

#endif