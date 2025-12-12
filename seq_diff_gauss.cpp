#include "seq_diff_gauss.hpp"
#include "file_manager.h"
#include <iostream>
#include <algorithm>

std::vector<float> create1dGaussianKernel(float sigma) {
    int radius = std::ceil(3.0f * sigma);
    int size = 2 * radius + 1;
    std::vector<float> kernel(size);
    
    float sum = 0.0f;
    for (int i = 0; i < size; ++i) {
        int x = i - radius; 
        kernel[i] = std::exp(-(x * x) / (2.0f * sigma * sigma));
        sum += kernel[i];
    }
    
    // Normalize
    for (float &val : kernel) {
        val /= sum;
    }
    return kernel;
}

void convolve_x(const Image& input, Image& output, const std::vector<float>& kernel) {
    int radius = kernel.size() / 2;
    for (int y = 0; y < input.height; ++y) {
        
        for (int x = 0; x < input.width; ++x) {

            float sum = 0.0f;

            for (size_t k = 0; k < kernel.size(); ++k) {
                int offset = k - radius;
                int nx = x + offset;
                
                if (nx < 0) nx = 0;

                if (nx >= input.width) nx = input.width - 1;
                
                sum += input.data[y * input.width + nx] * kernel[k];
            }
            output.data[y * input.width + x] = sum;
        }
    }
}

void convolve_y(const Image& input, Image& output, const std::vector<float>& kernel) {
    
    int radius = kernel.size() / 2;

    for (int y = 0; y < input.height; ++y) {

        for (int x = 0; x < input.width; ++x) {

            float sum = 0.0f;

            for (size_t k = 0; k < kernel.size(); ++k) {

                int offset = k - radius;
                int ny = y + offset;

                if (ny < 0) ny = 0;

                if (ny >= input.height) ny = input.height - 1;
                
                sum += input.data[ny * input.width + x] * kernel[k];
            }

            output.data[y * input.width + x] = sum;
        }
    }
}

Image GaussianBlur(const Image& input, float sigma) {
    
    std::vector<float> kernel = create1dGaussianKernel(sigma);
    Image temp(input.width, input.height);
    Image output(input.width, input.height);
    
    convolve_x(input, temp, kernel);
    convolve_y(temp, output, kernel);
    
    return output;
}

Image applyDoG(const Image& input, float sigma, float k, float tau) {
    
    float sigma1 = sigma;
    float sigma2 = k * sigma;
    
    Image g1 = GaussianBlur(input, sigma1);
    
    Image g2 = GaussianBlur(input, sigma2);
    
    Image output(input.width, input.height);
    
    for (size_t i = 0; i < input.data.size(); ++i) {
        //G1 - tau * G2
        output.data[i] = g1.data[i] - (tau * g2.data[i]);
    }
    
    return output;
}



Image convertToFloatImage(const FileManager& fm) {
    
    int w = fm.getWidth();
    int h = fm.getHeight();
    int c = fm.getChannels();
    std::vector<unsigned char> raw = fm.getImageData();
    
    Image img(w, h);

    for (int i = 0; i < w * h; ++i) {

        if (c == 1) {
            img.data[i] = static_cast<float>(raw[i]);
        } 
        else if (c >= 3) {
            int r = raw[i * c + 0];
            int g = raw[i * c + 1];
            int b = raw[i * c + 2];
            img.data[i] = 0.299f * r + 0.587f * g + 0.114f * b;
        }
    }

    return img;
}

std::vector<unsigned char> convertToBytes(const Image& img) {
    std::vector<unsigned char> bytes(img.width * img.height);
    
    for (size_t i = 0; i < img.data.size(); ++i) {
        float val = img.data[i];
    
        if (val < 0.0f) val = 0.0f;

        if (val > 255.0f) val = 255.0f;
        
        bytes[i] = static_cast<unsigned char>(val);
    }

    return bytes;
}


