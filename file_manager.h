#ifndef FILE_MANAGER_H
#define FILE_MANAGER_H

#include <string>
#include <vector>

class FileManager {
    private:
    unsigned char* text_data;
    unsigned char* image_data;
    std::string file_type;
    std::string filename;
    bool valid;
    bool is_image;
    size_t data_size;

    int width;
    int height;
    int channels;
    
    public:
    FileManager(const std::string& filepath, const std::string& type);
    FileManager(const unsigned char* image_data, int width, int height, int channels);
    ~FileManager();

    std::vector<unsigned char> getTextData() const;
    std::vector<unsigned char> getImageData() const;

    bool toBWImage();
    bool saveImage(const std::string& filepath) const;

    bool isValid() const;
    bool isImage() const;
    size_t getDataSize() const;
    int getWidth() const;
    int getHeight() const;
    int getChannels() const;
    std::string getFilename() const;
    

};

#endif