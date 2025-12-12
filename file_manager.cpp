#include "file_manager.h"
#include <iostream>
#include <fstream>
#include <cstring> 
#include <filesystem>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

FileManager::FileManager(const std::string& filepath, const std::string& type) {
    // 1. Initialize all pointers to nullptr to prevent crashes
    text_data = nullptr;
    image_data = nullptr;
    width = 0; height = 0; channels = 0;
    data_size = 0;
    valid = false;
    file_type = type;
    std::filesystem::path p(filepath);
    filename = p.filename().string();

    if (type == "image") {
        is_image = true;
        // Load image using STB
        // force 0 to keep original channels, or 3 to force RGB
        image_data = stbi_load(filepath.c_str(), &width, &height, &channels, 0);
        
        if (image_data) {
            valid = true;
            data_size = width * height * channels;

            
        } else {
            std::cerr << "Error: Failed to load image " << filepath << std::endl;
        }
    } 
    else if (type == "text") {
        is_image = false;
        // Load text using standard fstream
        std::ifstream file(filepath, std::ios::binary | std::ios::ate); // Open at end to get size
        
        if (file.is_open()) {
            data_size = file.tellg();
            file.seekg(0, std::ios::beg); // Go back to start

            text_data = new unsigned char[data_size];
            if (file.read(reinterpret_cast<char*>(text_data), data_size)) {
                valid = true;
            } else {
                delete[] text_data;
                text_data = nullptr;
                valid = false;
            }
            file.close();
        } else {
            std::cerr << "Error: Failed to open text file " << filepath << std::endl;
        }
    }
    else {
        std::cerr << "Error: Unsupported file type. Only \"image\" and \"text\" are supported " << type << std::endl;
    }
}

FileManager::FileManager(const unsigned char* input_data, int w, int h, int c) {
    // Init pointers
    text_data = nullptr;
    image_data = nullptr;
    
    // Set properties
    width = w;
    height = h;
    channels = c;
    is_image = true;
    file_type = "image";
    data_size = width * height * channels;
    filename = "";

    // DEEP COPY: Allocate new memory and copy the input data into it.
    // We use malloc here because stbi_image_free (in destructor) expects malloc'd memory.
    if (input_data != nullptr && data_size > 0) {
        image_data = (unsigned char*)malloc(data_size);
        if (image_data) {
            std::memcpy(image_data, input_data, data_size);
            valid = true;
        } else {
            valid = false; // Malloc failed
            std::cerr << "Error: Malloc failed in FileManager constructor" << std::endl;
        }
    } else {
        valid = false;
    }
}

FileManager::~FileManager() {
    // If it's an image, use STB's free
    if (image_data != nullptr) {
        stbi_image_free(image_data);
        image_data = nullptr;
    }

    // If it's text, use standard delete
    if (text_data != nullptr) {
        delete[] text_data;
        text_data = nullptr;
    }
}

bool FileManager::isValid() const {
    return valid;
}
bool FileManager::isImage() const {
    return is_image;
}

size_t FileManager::getDataSize() const {
    return data_size;
}

int FileManager::getWidth() const {
    return width;
}

int FileManager::getHeight() const {
    return height;
}

int FileManager::getChannels() const {
    return channels;
}

std::string FileManager::getFilename() const {
    return filename;
}

void FileManager::setFilename(const std::string& new_filename) {
    filename = new_filename;
}

std::vector<unsigned char> FileManager::getTextData() const {
    if (!is_image && text_data != nullptr && data_size > 0) {
        return std::vector<unsigned char>(text_data, text_data + data_size);
    }
    return std::vector<unsigned char>();
}

std::vector<unsigned char> FileManager::getImageData() const {
    if (is_image && image_data != nullptr && data_size > 0) {
        return std::vector<unsigned char>(image_data, image_data + data_size);
    }
    return std::vector<unsigned char>();
}

bool FileManager::saveImage(const std::string& filepath) const {
    if (!valid || !is_image || !image_data) return false;

    // stbi_write_png(filename, w, h, comp, data, stride_in_bytes)
    // stride_in_bytes is usually width * channels
    
    std::string full_path = filepath + filename;

    int result = stbi_write_png(full_path.c_str(), width, height, channels, image_data, width * channels);

    return (result != 0);
}

bool FileManager::toBWImage() {
    // 1. Safety Checks
    if (!valid || !is_image || image_data == nullptr) return false;
    
    // If already 1 channel, nothing to do
    if (channels == 1) return true; 

    // 2. Allocate new memory for 1-channel image
    // New size is just width * height (since 1 byte per pixel)
    size_t new_data_size = width * height;
    unsigned char* new_data = (unsigned char*)malloc(new_data_size);

    if (new_data == nullptr) return false; // Allocation failed

    // 3. Convert Pixels
    // We iterate through every pixel
    for (int i = 0; i < width * height; i++) {
        // Calculate where this pixel starts in the OLD (RGB) array
        int old_index = i * channels; 

        unsigned char r = image_data[old_index];
        unsigned char g = image_data[old_index + 1];
        unsigned char b = image_data[old_index + 2];

        // Apply Luminosity Formula
        // Cast to float for math, then back to uchar
        unsigned char gray = (unsigned char)(0.299f * r + 0.587f * g + 0.114f * b);

        new_data[i] = gray;
    }

    // 4. Swap Data
    // Free the old RGB memory (using STB's free function to be safe)
    stbi_image_free(image_data);
    
    // Update the class to point to the new Grayscale memory
    image_data = new_data;
    channels = 1; // CRITICAL: Update channel count so saveImage knows what to do
    data_size = new_data_size;

    return true;
}