#include "cuda_diff_gauss.cuh"
#include <iostream>
#include <cmath>
#include <algorithm>

// --- KERNEL 1: Row Convolution (X-Axis) ---
__global__ void d_convolve_x(float* input, float* output, int width, int height, float* kernel, int radius) {
    int x = blockIdx.x * blockDim.x + threadIdx.x;
    int y = blockIdx.y * blockDim.y + threadIdx.y;

    if (x >= width || y >= height) return;

    float sum = 0.0f;
    
    // Iterate over kernel
    for (int k = -radius; k <= radius; k++) {
        int sample_x = x + k;

        // Clamp to borders (Edge Handling)
        if (sample_x < 0) sample_x = 0;
        if (sample_x >= width) sample_x = width - 1;

        int index = y * width + sample_x;
        sum += input[index] * kernel[k + radius];
    }

    output[y * width + x] = sum;
}

// --- KERNEL 2: Column Convolution (Y-Axis) ---
__global__ void d_convolve_y(float* input, float* output, int width, int height, float* kernel, int radius) {
    int x = blockIdx.x * blockDim.x + threadIdx.x;
    int y = blockIdx.y * blockDim.y + threadIdx.y;

    if (x >= width || y >= height) return;

    float sum = 0.0f;

    for (int k = -radius; k <= radius; k++) {
        int sample_y = y + k;

        // Clamp to borders
        if (sample_y < 0) sample_y = 0;
        if (sample_y >= height) sample_y = height - 1;

        int index = sample_y * width + x;
        sum += input[index] * kernel[k + radius];
    }

    output[y * width + x] = sum;
}

// --- KERNEL 3: Apply DoG Math (Subtraction) ---
__global__ void d_calc_dog(float* g1, float* g2, float* output, int size, float tau) {
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= size) return;

    // DoG Formula: G1 - tau * G2
    output[i] = g1[i] - (tau * g2[i]);
    
    // Optional: Add XDoG specific mix if needed as per your CPU code logic
    // output[i] = (1.0f - tau) * g1[i] + tau * output[i];
}

// --- KERNEL 4: Apply XDoG (Subtraction + Tanh) ---
__global__ void d_calc_xdog(float* g1, float* g2, float* output, int size, float tau, float epsilon, float phi) {
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= size) return;

    // 1. Calculate Difference
    float D = g1[i] - (tau * g2[i]);
    
    // 2. Thresholding Logic
    float result = 0.0f;
    if (D > epsilon) {
        result = 1.0f;
    } else {
        result = 1.0f + tanh(phi * (D - epsilon));
    }

    // 3. Store result (0-255 scale usually, but here we keep float 0-1 or scale)
    // Your CPU code scales by 255 at the end, let's do it here
    output[i] = result * 255.0f; 
}


// --- CPU HELPER: Create Gaussian Kernel ---
std::vector<float> createGaussianKernel(float sigma) {
    int radius = std::ceil(3.0f * sigma);
    int size = 2 * radius + 1;
    std::vector<float> kernel(size);
    float sum = 0.0f;
    for (int i = 0; i < size; ++i) {
        int x = i - radius;
        kernel[i] = std::exp(-(x * x) / (2.0f * sigma * sigma));
        sum += kernel[i];
    }
    for (float &val : kernel) val /= sum;
    return kernel;
}

// --- GPUImage Helper Implementation ---
GPUImage::GPUImage(int w, int h) : width(w), height(h) {
    cudaMalloc(&d_data, width * height * sizeof(float));
}
GPUImage::~GPUImage() {
    cudaFree(d_data);
}
void GPUImage::upload(const std::vector<float>& host_data) {
    cudaMemcpy(d_data, host_data.data(), width * height * sizeof(float), cudaMemcpyHostToDevice);
}
std::vector<float> GPUImage::download() {
    std::vector<float> host_data(width * height);
    cudaMemcpy(host_data.data(), d_data, width * height * sizeof(float), cudaMemcpyDeviceToHost);
    return host_data;
}

// --- Internal Function to Run Blur on GPU ---
void runGaussianBlur(GPUImage& img, GPUImage& temp, float sigma) {
    std::vector<float> h_kernel = createGaussianKernel(sigma);
    int radius = h_kernel.size() / 2;
    int kernel_bytes = h_kernel.size() * sizeof(float);

    // Alloc kernel on GPU
    float* d_kernel;
    cudaMalloc(&d_kernel, kernel_bytes);
    cudaMemcpy(d_kernel, h_kernel.data(), kernel_bytes, cudaMemcpyHostToDevice);

    // Calculate Grid/Block
    dim3 block(16, 16);
    dim3 grid((img.width + block.x - 1) / block.x, (img.height + block.y - 1) / block.y);

    // Pass 1: Convolve X (Read img -> Write temp)
    d_convolve_x<<<grid, block>>>(img.d_data, temp.d_data, img.width, img.height, d_kernel, radius);
    cudaDeviceSynchronize();

    // Pass 2: Convolve Y (Read temp -> Write img)
    d_convolve_y<<<grid, block>>>(temp.d_data, img.d_data, img.width, img.height, d_kernel, radius);
    cudaDeviceSynchronize();

    cudaFree(d_kernel);
}

// --- HELPER: Convert FileManager to Floats ---
std::vector<float> fmToFloat(const FileManager& input) {
    int w = input.getWidth();
    int h = input.getHeight();
    int c = input.getChannels();
    std::vector<unsigned char> raw = input.getImageData();
    std::vector<float> data(w * h);

    for (int i = 0; i < w * h; ++i) {
        if (c == 1) {
            data[i] = (float)raw[i];
        } else {
            // Luminosity method for RGB
            int idx = i * c;
            data[i] = 0.299f * raw[idx] + 0.587f * raw[idx+1] + 0.114f * raw[idx+2];
        }
    }
    return data;
}

// --- HELPER: Convert Floats to FileManager ---
FileManager* floatToFM(const std::vector<float>& data, int w, int h) {
    std::vector<unsigned char> bytes(w * h);
    for (int i = 0; i < w * h; i++) {
        float val = data[i];
        if (val < 0.0f) val = 0.0f;
        if (val > 255.0f) val = 255.0f;
        bytes[i] = (unsigned char)val;
    }
    // Assumes FileManager has constructor: FileManager(unsigned char* data, int w, int h, int c)
    return new FileManager(bytes.data(), w, h, 1);
}


// --- MAIN: Apply XDoG CUDA ---
FileManager* applyXDoG_CUDA(const FileManager& input, float sigma, float k, float tau, float epsilon, float phi) {
    if (!input.isValid()) return nullptr;
    int w = input.getWidth();
    int h = input.getHeight();

    // 1. Prepare Data
    std::vector<float> h_raw = fmToFloat(input);
    GPUImage g1(w, h);
    GPUImage g2(w, h);
    GPUImage temp(w, h); // Scratchpad for convolution

    // Upload raw image to both g1 and g2 initially
    g1.upload(h_raw);
    g2.upload(h_raw);

    // 2. Blur G1 (sigma)
    runGaussianBlur(g1, temp, sigma);

    // 3. Blur G2 (k * sigma)
    runGaussianBlur(g2, temp, k * sigma);

    // 4. Compute XDoG (Math + Threshold)
    // We can reuse 'temp' or 'g1' to store the output. Let's use g1.
    int total_pixels = w * h;
    int threads = 256;
    int blocks = (total_pixels + threads - 1) / threads;

    d_calc_xdog<<<blocks, threads>>>(g1.d_data, g2.d_data, g1.d_data, total_pixels, tau, epsilon, phi);
    cudaDeviceSynchronize();

    // 5. Download and Return
    std::vector<float> result = g1.download();
    return floatToFM(result, w, h);
}

// --- MAIN: Apply DoG CUDA (Without Threshold) ---
FileManager* applyDoG_CUDA(const FileManager& input, float sigma, float k, float tau) {
    if (!input.isValid()) return nullptr;
    int w = input.getWidth();
    int h = input.getHeight();

    std::vector<float> h_raw = fmToFloat(input);
    GPUImage g1(w, h);
    GPUImage g2(w, h);
    GPUImage temp(w, h);

    g1.upload(h_raw);
    g2.upload(h_raw);

    runGaussianBlur(g1, temp, sigma);
    runGaussianBlur(g2, temp, k * sigma);

    int total_pixels = w * h;
    int threads = 256;
    int blocks = (total_pixels + threads - 1) / threads;

    d_calc_dog<<<blocks, threads>>>(g1.d_data, g2.d_data, g1.d_data, total_pixels, tau);
    cudaDeviceSynchronize();

    std::vector<float> result = g1.download();
    return floatToFM(result, w, h);
}