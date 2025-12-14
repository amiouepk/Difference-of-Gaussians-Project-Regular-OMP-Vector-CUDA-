#include <iostream>
#include <string>
#include <sstream>
#include <fstream>
#include <vector>
#include <omp.h>

#include "file_manager.h"
#include "seq_diff_gauss.hpp"
#include "omp_diff_gauss.hpp" 
#include "cuda_diff_gauss.cuh"

// Helper function to read floats from the shader text file
bool loadShaderParams(const std::string& filename, std::vector<float>& params) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open shader file: " << filename << "\n";
        return false;
    }
    float value;
    while (file >> value) {
        params.push_back(value);
    }
    return !params.empty();
}

void printUsage(const std::string& programName) {
    std::cout   << "Usage: " << programName << " [options]\n"
                << "Options:\n"
                << "  -h, --help       Show this help message and exit\n"
                << "  --GPU, -g        Use GPU (CUDA) for processing\n"
                << "  --omp            Use CPU Parallelism (OpenMP)\n"
                << "  --input <file>   Specify input file location\n"
                << "  --output <file>  Specify output file location\n"
                << "  --shader <file>  Specify shader file location (optional)\n"
                << "  --sigma <val>    XDoG Sigma (default 1.0)\n"
                << "  --k <val>        XDoG K (default 1.6)\n"
                << "  --tau <val>      XDoG P/Tau (Strength) (default 20.0)\n"
                << "  --epsilon <val>  XDoG Epsilon (Threshold) (default 50.0)\n"
                << "  --phi <val>      XDoG Phi (Softness) (default 10.0)\n";
}

void getUserInput(int argc, char* argv[], std::string* flags) {
    if (argc < 2) { 
        std::cerr << "Error: No input provided.\n";
        printUsage(argv[0]);
        exit(-1);
    } 
    
    // flags[0] usage: "0"=Sequential, "1"=CUDA, "2"=OpenMP
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--GPU" || arg == "-g") {
            flags[0] = "1"; // CUDA Mode
        } 
        else if (arg == "--omp") {
            flags[0] = "2"; // OpenMP Mode
        }
        else if (arg == "--input") {
            flags[1] = "1"; 
            i++;
            if (i < argc) flags[2] = argv[i];
        } 
        else if (arg == "--output") {
            flags[3] = "1"; 
            i++;
            if (i < argc) flags[4] = argv[i];
        }
        else if (arg == "--shader"){
            flags[5] = "1"; 
            i++;
            if (i < argc) flags[6] = argv[i];
        }
    }

    if (flags[1] == "0" || flags[3] == "0") {
        std::cerr << "Error: Missing required options (Input or Output).\n";
        printUsage(argv[0]);
        exit(-1);
    }
}

// sequential
void runSeq(FileManager& inputImage, std::string outputPath, float sigma, float k, float p, float epsilon, float phi) {
    std::cout << "[Mode: CPU Sequential] Applying XDoG...\n";
    
    Image floatImage = convertToFloatImage(inputImage);
    
    Image dog = applyXDoG(floatImage, sigma, k, p, epsilon, phi);
    
    FileManager outputImage = convertToFMImage(dog);
    
    outputImage.setFilename("seq_xdog_" + inputImage.getFilename());
    if (!outputImage.saveImage(outputPath)) {
        std::cerr << "Error: Failed to save output image.\n";
        exit(-1);
    }
    std::cout << "Saved: " << outputPath << "/" << outputImage.getFilename() << "\n";
}

// openmp
void runOMP(FileManager& inputImage, std::string outputPath, float sigma, float k, float p, float epsilon, float phi) {
    std::cout << "[Mode: CPU OpenMP] Applying XDoG on " << omp_get_max_threads() << " threads...\n";

    // 1. Convert to float (Parallel)
    Image floatImage = convertToFloatImage_OMP(inputImage);
    
    // 2. Process (Parallel)
    Image dog = applyXDoG_OMP(floatImage, sigma, k, p, epsilon, phi);
    
    // 3. Convert back (Parallel)
    FileManager outputImage = convertToFMImage_OMP(dog);
    
    outputImage.setFilename("omp_xdog_" + inputImage.getFilename());
    if (!outputImage.saveImage(outputPath)) {
        std::cerr << "Error: Failed to save output image.\n";
        exit(-1);
    }
    std::cout << "Saved: " << outputPath << "/" << outputImage.getFilename() << "\n";
}

// cuda
void runCUDA(FileManager& inputImage, std::string outputPath, float sigma, float k, float p, float epsilon, float phi) {
    std::cout << "[Mode: GPU CUDA] Applying XDoG...\n";

 
    FileManager* outputImage = applyXDoG_CUDA(inputImage, sigma, k, p, epsilon, phi);

    if (outputImage == nullptr) {
        std::cerr << "CUDA Error: Output is null (Check CUDA Memory/Kernel).\n";
        return;
    }

    outputImage->setFilename("cuda_xdog_" + inputImage.getFilename());

    
    std::string fullPath = outputPath + "/" + outputImage->getFilename();

    if (outputPath.find(".png") != std::string::npos || outputPath.find(".jpg") != std::string::npos) {
        fullPath = outputPath; 
    }

    if (!outputImage->saveImage(fullPath)) {
        std::cerr << "Error: Failed to save output image to " << fullPath << "\n";
    } else {
        std::cout << "Saved: " << fullPath << "\n";
    }

    delete outputImage; 
}


int main(int argc, char* argv[]) {
    // flags[0] = Mode ("0"=Seq, "1"=CUDA, "2"=OMP)
    // flags[2] = Input Path
    // flags[4] = Output Path
    // flags[6] = Shader Path
    std::string flags[7] = { "0", "0", "", "0", "", "0", "" };
    getUserInput(argc, argv, flags);

    // Default Parameters (Tuned for 0-255 range)
    float sigma = 1.0f;
    float k_val = 1.6f;
    float p     = 20.0f; 
    float eps   = 50.0f; 
    float phi   = 10.0f;

    if (flags[5] == "1") {
        std::vector<float> params;
        std::cout << "Reading shader file: " << flags[6] << "\n";
        if (loadShaderParams(flags[6], params)) {
            if (params.size() > 0) sigma = params[0];
            if (params.size() > 1) k_val = params[1];
            if (params.size() > 2) p     = params[2];
            if (params.size() > 3) eps   = params[3];
            if (params.size() > 4) phi   = params[4];
        }
    }

    FileManager inputImage(flags[2], "image");
    if (!inputImage.isValid()) {
        std::cerr << "Error: Failed to load input image.\n";
        return -1;
    }
    
    std::cout << "Loaded input: " << inputImage.getFilename() << "\n";
    std::cout << "Params -> Sigma:" << sigma << " K:" << k_val 
              << " p:" << p << " Eps:" << eps << " Phi:" << phi << "\n";

    // Switch based on Mode
    if (flags[0] == "1") {
        runCUDA(inputImage, flags[4], sigma, k_val, p, eps, phi);
    } 
    else if (flags[0] == "2") {
        runOMP(inputImage, flags[4], sigma, k_val, p, eps, phi);
    } 
    else {
        runSeq(inputImage, flags[4], sigma, k_val, p, eps, phi);
    }

    return 0;
}