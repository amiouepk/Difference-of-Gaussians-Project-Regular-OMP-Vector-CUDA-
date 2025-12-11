#include "file_manager.h"
#include <iostream>
#include <fstream>
#include <cstring> 

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

