#include "seq_diff_gauss.hpp"
#include <iostream>
#include <algorithm>
#include <cmath>
#include <vector>

// ... (Kernels and Convolutions remain exactly the same as you have them) ...
// ... Copy paste your previous create1dGaussianKernel, convolve_x, convolve_y, GaussianBlurRaw ...

std::vector<float> create1dGaussianKernel(float sigma) {
    int radius = std::ceil(3.0f * sigma);
    int size = 2 * radius + 1;
    std::vector<float> kernel(size);
    float sigma2 = 2.0f * sigma * sigma;
    float sum = 0.0f;
    for (int i = 0; i < size; ++i) {
        int x = i - radius; 
        kernel[i] = std::exp(-(x * x) / sigma2);
        sum += kernel[i];
    }
    float invSum = 1.0f / sum;
    for (float &val : kernel) {
        val *= invSum;
    }
    return kernel;
}

void convolve_x(const Image& input, Image& output, const std::vector<float>& kernel) {
    int radius = kernel.size() / 2;
    int w = input.width;
    int h = input.height;
    int kSize = kernel.size();
    const float* inData = input.data.data();
    float* outData = output.data.data();
    for (int y = 0; y < h; ++y) {
        int rowOffset = y * w;
        for (int x = 0; x < w; ++x) {
            float sum = 0.0f;
            for (int k = 0; k < kSize; ++k) {
                int nx = std::clamp(x + k - radius, 0, w - 1);
                sum += inData[rowOffset + nx] * kernel[k];
            }
            outData[rowOffset + x] = sum;
        }
    }
}

void convolve_y(const Image& input, Image& output, const std::vector<float>& kernel) {
    int w = input.width;
    int h = input.height;
    int kSize = kernel.size();
    int radius = kSize / 2;
    std::fill(output.data.begin(), output.data.end(), 0.0f);
    const float* inData = input.data.data();
    float* outData = output.data.data();
    for (int y = 0; y < h; ++y) {
        float* destRow = &outData[y * w];
        for (int k = 0; k < kSize; ++k) {
            int ny = std::clamp(y + k - radius, 0, h - 1);
            float weight = kernel[k];
            const float* srcRow = &inData[ny * w];
            for (int x = 0; x < w; ++x) {
                destRow[x] += srcRow[x] * weight;
            }
        }
    }
}

void GaussianBlurRaw(const Image& input, Image& output, Image& tempBuffer, float sigma) {
    if (tempBuffer.width != input.width || tempBuffer.height != input.height) 
        tempBuffer.resize(input.width, input.height);
    if (output.width != input.width || output.height != input.height) 
        output.resize(input.width, input.height);
    std::vector<float> kernel = create1dGaussianKernel(sigma);
    convolve_x(input, tempBuffer, kernel);
    convolve_y(tempBuffer, output, kernel);
}

// ... (applyDoG remains the same) ...

Image applyXDoG(const Image& input, float sigma, float k, float p, float epsilon, float phi) {
    Image g1(input.width, input.height);
    Image g2(input.width, input.height);
    Image temp(input.width, input.height); 

    GaussianBlurRaw(input, g1, temp, sigma);
    GaussianBlurRaw(input, g2, temp, sigma * k);

    Image output(input.width, input.height);
    size_t size = input.data.size();

    const float* pG1 = g1.data.data();
    const float* pG2 = g2.data.data();
    float* pOut = output.data.data();

    for (size_t i = 0; i < size; ++i) {
        float scaledDifference = (1.0f + p) * pG1[i] - p * pG2[i];
        float val = scaledDifference / 255.0f * 100.0f; 

        float result;
        if (val >= epsilon) {
            result = 1.0f; 
        } else {
            result = 1.0f + std::tanh(phi * (val - epsilon));
        }
        
        // --- CHANGE HERE ---
        // Invert the result to get "Black lines on White Background"
        // Original: pOut[i] = result * 255.0f;
        pOut[i] = 255.0f - (result * 255.0f);

        if (pOut[i] < 0.0f) pOut[i] = 0.0f;
        if (pOut[i] > 255.0f) pOut[i] = 255.0f;
    }

    return output;
}

// ... (Rest of file_manager logic remains the same) ...
Image convertToFloatImage(const FileManager& fm) {
    int w = fm.getWidth();
    int h = fm.getHeight();
    int c = fm.getChannels();
    std::vector<unsigned char> raw = fm.getImageData();
    Image img(w, h);
    size_t size = w * h;
    const unsigned char* pRaw = raw.data();
    float* pImg = img.data.data();
    if (c == 1) {
        for (size_t i = 0; i < size; ++i) pImg[i] = static_cast<float>(pRaw[i]);
    } else if (c >= 3) {
        for (size_t i = 0; i < size; ++i) {
            int idx = i * c;
            pImg[i] = 0.299f * pRaw[idx] + 0.587f * pRaw[idx+1] + 0.114f * pRaw[idx+2];
        }
    }
    return img;
}

FileManager convertToFMImage(const Image& img) {
    std::vector<unsigned char> bytes(img.width * img.height);
    size_t size = img.data.size();
    const float* pData = img.data.data();
    unsigned char* pBytes = bytes.data();
    for (size_t i = 0; i < size; ++i) {
        float val = pData[i];
        if (val < 0.0f) val = 0.0f;
        else if (val > 255.0f) val = 255.0f;
        pBytes[i] = static_cast<unsigned char>(val);
    }
    return FileManager(bytes.data(), img.width, img.height, 1);
}