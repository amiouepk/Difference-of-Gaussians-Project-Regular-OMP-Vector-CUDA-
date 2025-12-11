#ifndef SEQ_DIFF_GAUSS_H
#define SEQ_DIFF_GAUSS_H

#include <cstdint>
#include <cmath>
#include <vector>
#include <complex>
#include <iostream>
#include <fstream>
#include <webp/encode.h>

struct Image {
    int width;
    int height;
    std::vector<float> data; 

    Image(int w, int h) : width(w), height(h), data(w * h) {}
};

std::vector<float> create1dGaussianKernel(float sigma);

void convolve_x(const Image& input, Image& output, const std::vector<float>& kernel);

void convolve_y(const Image& input, Image& output, const std::vector<float>& kernel);

Image GaussianBlur(const Image& input, float sigma);

Image applyDoG(const Image& input, float sigma, float k, float tau);



#endif