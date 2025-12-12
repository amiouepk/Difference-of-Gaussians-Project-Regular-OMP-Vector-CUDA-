#include <iostream>
#include <string>
#include <sstream>
#include <fstream>

#include "file_manager.h"
#include "seq_diff_gauss.hpp"

void printUsage(const std::string& programName) {
    std::cout   << "Usage: " << programName << " [options]\n"
                << "Options:\n"
                << "  -h, --help       Show this help message and exit\n"
                << "  --GPU, -g       Use GPU for processing\n"
                << "  --input <file>   Specify input file location\n"
                << "  --output <file>  Specify output file location\n"
                << "  --shader <file>  Specify shader file location\n";
}

void getUserInput(int argc, char* argv[], std::string* flags) {
    if (argc < 2) { 
        std::cerr << "Error: No input provided.\n";
        printUsage(argv[0]);
        exit(-1);
    } 
    
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--GPU" || arg == "-g") {
            flags[0] = 1;
        } else if (arg == "--input") {
            flags[1] = 1;
            i++;
            flags[2] = argv[i]; // Next argument is the input file
        } else if (arg == "--output") {
            flags[3] = 1;
            i++;
            flags[4] = argv[i]; // Next argument is the output file
        }
        else if (arg == "--shader"){
            flags[5] = 1;
            i++;
            flags[6] = argv[i]; // Next argument is the shader file
        }
        else {
            std::cerr << "Error: Unknown option '" << arg << "'\n";
            printUsage(argv[0]);
            exit(-1);
        }
    }
    if (flags[1] == "0" || flags[3] == "0" || flags[5] == "0") {
        std::cerr << "Error: Missing required options.\n";
        printUsage(argv[0]);
        exit(-1);
    }
}

std::vector<std::vector<int>> load2DArray(const std::string& filename) {
    std::vector<std::vector<int>> grid;
    std::ifstream file(filename);

    if (!file.is_open()) {
        std::cerr << "Error opening file!" << std::endl;
        return grid;
    }

    std::string line;
    // 1. Read line by line
    while (std::getline(file, line)) {
        std::vector<int> row;
        std::stringstream ss(line);
        int value;

        // 2. Read number by number from that line
        while (ss >> value) {
            row.push_back(value);
        }

        // Only add non-empty rows
        if (!row.empty()) {
            grid.push_back(row);
        }
    }

    return grid;
}

int main(int argc, char* argv[]) {
    std::string programName = argv[0];

    // Check for help option
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-h" || arg == "--help") {
            printUsage(programName);
            return 0;
        }
    }

    // Get user input
    std::string flags[7] = { "0", "0", "", "0", "", "0", "" };
    getUserInput( argc, argv, flags);


    // Testing file manager
    FileManager inputImage(flags[2], "image");
    if (!inputImage.isValid()) {
        std::cerr << "Error: Failed to load input image.\n";
        return -1;
    }
    std::cout << "Loaded image: " << flags[2] << " (" 
              << inputImage.getWidth() << "x" << inputImage.getHeight() 
              << ", " << inputImage.getChannels() << " channels)\n";
    
    // Convert to black and white
    printf("Converting image to black and white...\n");
    if (!inputImage.toBWImage()) {
        std::cerr << "Error: Failed to convert image to black and white.\n";
        return -1;
    }

    // Convert to float image
    Image floatImage = convertToFloatImage(inputImage);

    // Apply Gaussian blur
    // Image blurred = GaussianBlur(floatImage, 10.0f);

    // Apply DoG


    Image dog = applyDoG(floatImage, 10.4f, 2.6f, 0.5f);


    // Save output image
    FileManager outputImage = convertToFMImage(dog);
    outputImage.setFilename("dog_" + inputImage.getFilename());

    if (!outputImage.saveImage(flags[4])) {
        std::cerr << "Error: Failed to save output image to " << flags[4] << "\n";
        return -1;
    }

    return 0;
}