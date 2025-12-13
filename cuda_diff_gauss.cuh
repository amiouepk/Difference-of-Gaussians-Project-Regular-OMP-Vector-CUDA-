#ifndef CUDA_DIFF_GAUSS_H
#define CUDA_DIFF_GAUSS_H

#include "file_manager.h"
#include <vector>
#include <cuda_runtime.h>

// --- Helper Struct for GPU Data ---
// We use this internally to manage pointers easily
struct GPUImage {
    int width;
    int height;
    float* d_data; // Device pointer (on GPU)

    GPUImage(int w, int h);
    ~GPUImage();
    
    // Copy data from Host (CPU) vector to Device (GPU)
    void upload(const std::vector<float>& host_data);
    
    // Copy data from Device (GPU) back to Host (CPU)
    std::vector<float> download();
};

// --- Main CUDA Functions ---

// Applies Difference of Gaussians on GPU
// Returns a pointer to a NEW FileManager object (must be deleted by user)
FileManager* applyDoG_CUDA(const FileManager& input, float sigma, float k, float tau);

// Applies XDoG (Extended DoG) with tanh thresholding on GPU
// Returns a pointer to a NEW FileManager object (must be deleted by user)
FileManager* applyXDoG_CUDA(const FileManager& input, float sigma, float k, float tau, float epsilon, float phi);

#endif