#pragma once

#include <fstream>
#include <vector>
#include <iostream>

using namespace std;


/*
    NOTE 'pragma pack' allows these structs to be 'reinterpret_cast<const char*>' to a char array
    correctly. This allows to write them easily and correctly according to the bitmap image format.
*/


// Bitmap file header (14 bytes)
#pragma pack(push, 1)
struct BMPFileHeader {
    uint16_t file_type{0x4D42}; // File type always BM which is 0x4D42
    uint32_t file_size{0};      // Size of the file (in bytes)
    uint16_t reserved1{0};      // Reserved, always 0
    uint16_t reserved2{0};      // Reserved, always 0
    uint32_t offset_data{0};    // Start position of pixel data (bytes from the beginning of the file)
};
#pragma pack(pop)

// DIB header (40 bytes)
#pragma pack(push, 1)
struct BMPInfoHeader {
    uint32_t size{0};        // Size of this header (in bytes)
    int32_t width{0};        // width of bitmap in pixels
    int32_t height{0};       // height of bitmap in pixels
    uint16_t planes{1};      // No. of planes for the target device, this is always 1
    uint16_t bit_count{0};   // No. of bits per pixel
    uint32_t compression{0}; // 0 or 3 - uncompressed. This program does not compress
    uint32_t size_image{0};  // 0 - for uncompressed images
    int32_t x_pixels_per_meter{0};
    int32_t y_pixels_per_meter{0};
    uint32_t colors_used{0};      // No. color indexes in the color table. Use 0 for the max number of colors allowed by bit_count
    uint32_t colors_important{0}; // No. of colors used for displaying the bitmap. If 0 all colors are required
};
#pragma pack(pop)


void generateBitmapImage(uint8_t* pixel_data, int height, int width, const char* imageFileName) {
    BMPFileHeader file_header;
    BMPInfoHeader info_header;

    const int bytesPerPixel = 3; // RGB
    const int fileHeaderSize = 14;
    const int infoHeaderSize = 40;
    const int paddingAmount = (4 - (width * bytesPerPixel) % 4) % 4;

    info_header.size = infoHeaderSize;
    info_header.width = width;
    info_header.height = height;
    info_header.bit_count = 24; // 3 bytes or 24 bits per pixel
    info_header.compression = 0; // BI_RGB, no compression
    info_header.size_image = (width * bytesPerPixel + paddingAmount) * height;
    file_header.file_size = fileHeaderSize + infoHeaderSize + info_header.size_image;
    file_header.offset_data = fileHeaderSize + infoHeaderSize;

    std::ofstream file(imageFileName, std::ios::out | std::ios::binary);
    if (!file) {
        std::cerr << "Could not write to file" << std::endl;
        return;
    }

    file.write(reinterpret_cast<const char*>(&file_header), sizeof(file_header));
    file.write(reinterpret_cast<const char*>(&info_header), sizeof(info_header));

    std::vector<uint8_t> padding(paddingAmount, 0);

    for (int i = 0; i < height; ++i) {
        file.write(reinterpret_cast<const char*>(&pixel_data[i * width * bytesPerPixel]), width * bytesPerPixel);
        file.write(reinterpret_cast<const char*>(padding.data()), paddingAmount);
    }

    file.close();
}


/**
 * Convert a bitmap to a pixel array. The resulting array should be height * width * 3 long. Every 3 values is a pixel,
 * with each new pixel going along the width, and then height. 
 */
void convertBitmapToByteArray(const char* imageFileName, uint8_t* image_data, int height, int width) {
    std::ifstream file(imageFileName, std::ios::in | std::ios::binary);
    if (!file) {
        std::cerr << "Could not read file" << std::endl;
        return;
    }

    BMPFileHeader file_header;
    BMPInfoHeader info_header;

    file.read(reinterpret_cast<char*>(&file_header), sizeof(file_header));
    file.read(reinterpret_cast<char*>(&info_header), sizeof(info_header));

    const int bytesPerPixel = 3; // RGB
    const int paddingAmount = (4 - (width * bytesPerPixel) % 4) % 4;

    for (int i = 0; i < height; ++i) {
        file.read(reinterpret_cast<char*>(&image_data[i * width * bytesPerPixel]), width * bytesPerPixel);
        file.seekg(paddingAmount, std::ios::cur); // Skip padding
    }

    file.close();
}


uint8_t* scale_down(uint8_t* image, int height, int width, int new_height, int new_width) {
    uint8_t* new_image = new uint8_t[3*new_height*new_width];
    double step = std::max(
        width / (double) new_width,
        height / (double) new_height
    );
    
    int new_x = 0;
    int new_y = 0;
    for (double y = 0; y < height; y+=step) {
        new_x = 0;
        for (double x = 0; x < width; x+=step) {
            int old_x = 3 * (x/3);
            int old_y = 3 * (y/3);


            int new_index = 3 * new_y * new_width  + 3 * new_x;
            int index = 3 * old_y * width  + 3 * old_x;

            new_image[new_index] = image[index];
            new_image[new_index+1] = image[index+1];
            new_image[new_index+2] = image[index+2];

            ++new_x;
        }
        ++new_y;
    }

    return new_image;
}


uint8_t* rotate_90(uint8_t* array, int width, int height) {
    uint8_t* new_image = new uint8_t[3*width*height];
    int new_width = height;
    int new_height = width;

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {

            int fi = 3 * x + 3 * y * width;

            int new_x = y;
            int new_y = (new_height - 1 - x);
            int i = 3 * new_x + 3 * new_y * new_width;

            new_image[i] = array[fi];
            new_image[i+1] = array[fi+1];
            new_image[i+2] = array[fi+2];
        }
    }

    return new_image;
}

uint8_t* rotate_180(uint8_t* array, int width, int height) {

    uint8_t* flipped = new uint8_t[3*width*height];
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {

            int i = 3 * x + 3 * y * width;
            int fi = 3 * x + 3 * (height - y) * width;

            flipped[i] = array[fi];
            flipped[i+1] = array[fi+1];
            flipped[i+2] = array[fi+2];
        }
    }
    return flipped;
}
