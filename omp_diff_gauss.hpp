#ifndef OMP_DIFF_GAUSS_H
#define OMP_DIFF_GAUSS_H

#include "seq_diff_gauss.hpp" // Include this to get the 'Image' struct definition
#include "file_manager.h"
#include <vector>

// We reuse the 'Image' struct from seq_diff_gauss.hpp.
// If you want to run this standalone, copy the struct definition here.

// Function declarations with _OMP suffix to avoid linker collisions
Image applyXDoG_OMP(const Image& input, float sigma, float k, float p, float epsilon, float phi);

Image convertToFloatImage_OMP(const FileManager& fm);
FileManager convertToFMImage_OMP(const Image& img);

#endif