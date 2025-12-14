#include "omp_diff_gauss.hpp"
#include <iostream>
#include <algorithm>
#include <cmath>
#include <vector>
#include <omp.h> 

// Reuse kernel generator
extern std::vector<float> create1dGaussianKernel(float sigma);

void convolve_x_OMP(const Image& input, Image& output, const std::vector<float>& kernel) {
    int radius = kernel.size() / 2;
    int w = input.width;
    int h = input.height;
    int kSize = kernel.size();

    const float* inData = input.data.data();
    float* outData = output.data.data();

    // 1. Thread Parallelism (Rows)
    #pragma omp parallel for
    for (int y = 0; y < h; ++y) {
        int rowOffset = y * w;
        for (int x = 0; x < w; ++x) {
            float sum = 0.0f;
            
            // 2. SIMD Vectorization (Kernel Calculation)
            // Reduction handles the summation into a single variable efficiently
            #pragma omp simd reduction(+:sum)
            for (int k = 0; k < kSize; ++k) {
                // Note: std::clamp is usually branchless, fine for SIMD
                int nx = std::clamp(x + k - radius, 0, w - 1);
                sum += inData[rowOffset + nx] * kernel[k];
            }
            outData[rowOffset + x] = sum;
        }
    }
}

void convolve_y_OMP(const Image& input, Image& output, const std::vector<float>& kernel) {
    int w = input.width;
    int h = input.height;
    int kSize = kernel.size();
    int radius = kSize / 2;

    // Reset output buffer (Parallel + SIMD)
    #pragma omp parallel for simd
    for (size_t i = 0; i < output.data.size(); ++i) {
        output.data[i] = 0.0f;
    }

    const float* inData = input.data.data();
    float* outData = output.data.data();

    // 1. Thread Parallelism (Rows)
    #pragma omp parallel for 
    for (int y = 0; y < h; ++y) {
        float* destRow = &outData[y * w];

        for (int k = 0; k < kSize; ++k) {
            int ny = std::clamp(y + k - radius, 0, h - 1);
            float weight = kernel[k];
            const float* srcRow = &inData[ny * w];

            // 2. SIMD Vectorization (Pixel addition)
            // This is the ideal case for SIMD: continuous memory add
            #pragma omp simd
            for (int x = 0; x < w; ++x) {
                destRow[x] += srcRow[x] * weight;
            }
        }
    }
}

void GaussianBlurRaw_OMP(const Image& input, Image& output, Image& tempBuffer, float sigma) {
    // Resize logic (Single thread safety)
    if (tempBuffer.width != input.width || tempBuffer.height != input.height) 
        tempBuffer.resize(input.width, input.height);
    if (output.width != input.width || output.height != input.height) 
        output.resize(input.width, input.height);

    std::vector<float> kernel = create1dGaussianKernel(sigma);

    convolve_x_OMP(input, tempBuffer, kernel);
    convolve_y_OMP(tempBuffer, output, kernel);
}

Image applyXDoG_OMP(const Image& input, float sigma, float k, float p, float epsilon, float phi) {
    Image g1(input.width, input.height);
    Image g2(input.width, input.height);
    Image temp(input.width, input.height); 

    // Parallel Blurs
    GaussianBlurRaw_OMP(input, g1, temp, sigma);
    GaussianBlurRaw_OMP(input, g2, temp, sigma * k);

    Image output(input.width, input.height);
    size_t size = input.data.size();

    const float* pG1 = g1.data.data();
    const float* pG2 = g2.data.data();
    float* pOut = output.data.data();

    // Parallel Thresholding with SIMD
    // Note: tanh might prevent vectorization on older compilers, 
    // but modern GCC/Clang can vectorize math functions with -O3 -ffast-math
    #pragma omp parallel for simd 
    for (size_t i = 0; i < size; ++i) {
        float scaledDifference = (1.0f + p) * pG1[i] - p * pG2[i];
        
        // 0-100 normalization
        float val = scaledDifference / 255.0f * 100.0f; 

        float result;
        if (val >= epsilon) {
            result = 1.0f; 
        } else {
            result = 1.0f + std::tanh(phi * (val - epsilon));
        }
        
        // Invert and Scale
        float finalVal = 255.0f - (result * 255.0f);

        // Clamp
        if (finalVal < 0.0f) finalVal = 0.0f;
        if (finalVal > 255.0f) finalVal = 255.0f;
        
        pOut[i] = finalVal;
    }

    return output;
}

Image convertToFloatImage_OMP(const FileManager& fm) {
    int w = fm.getWidth();
    int h = fm.getHeight();
    int c = fm.getChannels();
    std::vector<unsigned char> raw = fm.getImageData();
    Image img(w, h);
    size_t size = w * h;
    const unsigned char* pRaw = raw.data();
    float* pImg = img.data.data();

    if (c == 1) {
        #pragma omp parallel for simd
        for (size_t i = 0; i < size; ++i) {
            pImg[i] = static_cast<float>(pRaw[i]);
        }
    } 
    else if (c >= 3) {
        #pragma omp parallel for simd
        for (size_t i = 0; i < size; ++i) {
            int idx = i * c;
            // RGB to Grayscale
            pImg[i] = 0.299f * pRaw[idx] + 0.587f * pRaw[idx+1] + 0.114f * pRaw[idx+2];
        }
    }
    return img;
}

FileManager convertToFMImage_OMP(const Image& img) {
    std::vector<unsigned char> bytes(img.width * img.height);
    size_t size = img.data.size();
    const float* pData = img.data.data();
    unsigned char* pBytes = bytes.data();

    #pragma omp parallel for simd
    for (size_t i = 0; i < size; ++i) {
        float val = pData[i];
        if (val < 0.0f) val = 0.0f;
        else if (val > 255.0f) val = 255.0f;
        pBytes[i] = static_cast<unsigned char>(val);
    }
    return FileManager(bytes.data(), img.width, img.height, 1);
}