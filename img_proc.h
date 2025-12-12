#ifndef IMG_PROC_H
#define IMG_PROC_H

#include <file_manager.h>

class ImageProcessor {
    public:
    static bool applyThreshold(FileManager& imgFile, unsigned char threshold);

};

#endif // IMG_PROC_H