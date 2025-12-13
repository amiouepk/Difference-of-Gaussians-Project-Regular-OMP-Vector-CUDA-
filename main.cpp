#include <iostream>
#include <string>
#include <sstream>
#include <fstream>
#include <vector>

#include "file_manager.h"
#include "seq_diff_gauss.hpp"
#include <omp.h> 
#include "cuda_diff_gauss.cuh"

// Load parameters from text file
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
                << "  --GPU, -g        Use GPU for processing\n"
                << "  --input <file>   Specify input file location\n"
                << "  --output <file>  Specify output file location\n"
                << "  --shader <file>  Specify shader file location\n";
}

// Standard argument parser
void getUserInput(int argc, char* argv[], std::string* flags) {
    if (argc < 2) { 
        std::cerr << "Error: No input provided.\n";
        printUsage(argv[0]);
        exit(-1);
    } 
    
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--GPU" || arg == "-g") {
            flags[0] = "1"; // <--- ADD QUOTES HERE
        } else if (arg == "--input") {
            flags[1] = "1"; // <--- ADD QUOTES HERE
            i++;
            if (i < argc) flags[2] = argv[i];
        } else if (arg == "--output") {
            flags[3] = "1"; // <--- ADD QUOTES HERE
            i++;
            if (i < argc) flags[4] = argv[i];
        }
        else if (arg == "--shader"){
            flags[5] = "1"; // <--- CRITICAL FIX: ADD QUOTES HERE
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

void XDogProcess(std::string flags[], float sigma, float k, float p, float epsilon, float phi){
    FileManager inputImage(flags[2], "image");
    if (!inputImage.isValid()) {
        std::cerr << "Error: Failed to load input image.\n";
        exit(-1);
    }
    std::cout << "Loaded input: " << inputImage.getFilename() << "\n";
    std::cout << "Running XDoG: Sigma=" << sigma << " K=" << k 
              << " p=" << p << " Eps=" << epsilon << " Phi=" << phi << "\n";
    
    Image floatImage = convertToFloatImage(inputImage);
    Image dog = applyXDoG(floatImage, sigma, k, p, epsilon, phi);
    FileManager outputImage = convertToFMImage(dog);
    
    outputImage.setFilename("xdog_" + inputImage.getFilename());
    if (!outputImage.saveImage(flags[4])) {
        std::cerr << "Error: Failed to save output image.\n";
        exit(-1);
    }
    std::cout << "Saved: " << flags[4] << "/" << outputImage.getFilename() << "\n";
}

int main(int argc, char* argv[]) {
    std::string flags[7] = { "0", "0", "", "0", "", "0", "" };
    getUserInput(argc, argv, flags);

    // DEFAULTS
    float sigma = 1.0f;
    float k_val = 1.6f;
    float p     = 20.0f; // Sharpening strength (Tau in old code)
    float eps   = 0.5f;  // Threshold (0.0 to 1.0 now)
    float phi   = 10.0f; // Softness

    // Check for shader file
    if (flags[5] == "1") {
        std::vector<float> params;
        if (loadShaderParams(flags[6], params)) {
            // Apply parameters in order if they exist in the file
            if (params.size() > 0) sigma = params[0];
            if (params.size() > 1) k_val = params[1];
            if (params.size() > 2) p     = params[2];
            if (params.size() > 3) eps   = params[3];
            if (params.size() > 4) phi   = params[4];
        }
    }

    XDogProcess(flags, sigma, k_val, p, eps, phi);
    return 0;
}