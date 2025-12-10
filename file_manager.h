#ifndef FILE_MANAGER_H
#define FILE_MANAGER_H

#include <string>
#include <vector>

class FileManager {
    private:
    unsigned char* text_data;
    unsigned char* image_data;
    std::string file_type;
    bool valid;
    int 
    
    public:
    FileManager(const std::string& filepath, const std::string& type);
    FileManager(const unsigned char* image_data);
    ~FileManager();

    std::vector<unsigned char> getTextData() const;
    std::vector<unsigned char> getImageData() const;

    bool saveImage(const std::string& filepath) const;

    bool isValid() const;

};

#endif