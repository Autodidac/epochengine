#pragma once

#include <vector>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <cstring>
#include <string>
#include <algorithm>
#include <iostream>

namespace almondnamespace {

    struct ImageData {
        int width = 0;
        int height = 0;
        int channels = 0;
        std::vector<uint8_t> pixels;
    };

    inline void ListSupportedImageTypes() {
        std::cout << "Currently supported image types are: Bmp, Tga, and Ppm.";
    }

    // Loads an image from a file (supports BMP, TGA, PPM)
    inline ImageData LoadAlmondImage(const std::filesystem::path& filepath) {
        auto ext = filepath.extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

        if (ext == ".bmp") {
            return LoadBMP(filepath);
        }
        else if (ext == ".tga") {
            return LoadTGA(filepath);
        }
        else if (ext == ".ppm") {
            return LoadPPM(filepath);
        }
        else {
            throw std::runtime_error("Unsupported file format: " + filepath.string());
        }
    }

    // Loads a BMP image
    inline ImageData LoadBMP(const std::filesystem::path& filepath) {
        std::ifstream file(filepath, std::ios::binary);
        if (!file) {
            throw std::runtime_error("Failed to open BMP file: " + filepath.string());
        }

        char header[54];
        file.read(header, 54);
        if (std::memcmp(header, "BM", 2) != 0) {
            throw std::runtime_error("Invalid BMP file: " + filepath.string());
        }

        int width = *reinterpret_cast<int*>(&header[18]);
        int height = *reinterpret_cast<int*>(&header[22]);
        int bitsPerPixel = *reinterpret_cast<short*>(&header[28]);
        int channels = bitsPerPixel / 8;

        if (bitsPerPixel != 24 && bitsPerPixel != 32) {
            throw std::runtime_error("Unsupported BMP bit depth: " + filepath.string());
        }

        int rowPadding = (4 - (width * channels) % 4) % 4;
        std::vector<uint8_t> pixels(width * height * channels);

        file.seekg(*reinterpret_cast<int*>(&header[10]));
        for (int y = 0; y < height; ++y) {
            file.read(reinterpret_cast<char*>(&pixels[(height - y - 1) * width * channels]), width * channels);
            file.ignore(rowPadding);
        }

        if (channels == 3) {
            std::vector<uint8_t> rgbaData(width * height * 4);
            for (int i = 0; i < width * height; ++i) {
                rgbaData[i * 4 + 0] = pixels[i * 3 + 0];
                rgbaData[i * 4 + 1] = pixels[i * 3 + 1];
                rgbaData[i * 4 + 2] = pixels[i * 3 + 2];
                rgbaData[i * 4 + 3] = 255;
            }
            return { width, height, 4, rgbaData };
        }

        return { width, height, channels, pixels };
    }

    // Loads a TGA image
    inline ImageData LoadTGA(const std::filesystem::path& filepath) {
        std::ifstream file(filepath, std::ios::binary);
        if (!file) {
            throw std::runtime_error("Failed to open TGA file: " + filepath.string());
        }

        char header[18];
        file.read(header, 18);

        int width = *reinterpret_cast<short*>(&header[12]);
        int height = *reinterpret_cast<short*>(&header[14]);
        int bitsPerPixel = header[16];
        int channels = bitsPerPixel / 8;

        if (bitsPerPixel != 24 && bitsPerPixel != 32) {
            throw std::runtime_error("Unsupported TGA bit depth: " + filepath.string());
        }

        std::vector<uint8_t> pixels(width * height * channels);
        file.read(reinterpret_cast<char*>(pixels.data()), pixels.size());

        return { width, height, channels, pixels };
    }

    // Loads a PPM image (P6 format only)
    inline ImageData LoadPPM(const std::filesystem::path& filepath) {
        std::ifstream file(filepath, std::ios::binary);
        if (!file) {
            throw std::runtime_error("Failed to open PPM file: " + filepath.string());
        }

        std::string header;
        file >> header;
        if (header != "P6") {
            throw std::runtime_error("Unsupported PPM format: " + filepath.string());
        }

        int width, height, maxVal;
        file >> width >> height >> maxVal;
        file.ignore(1); // Skip the newline after the header

        if (maxVal != 255) {
            throw std::runtime_error("Unsupported PPM max value: " + filepath.string());
        }

        std::vector<uint8_t> pixels(width * height * 3);
        file.read(reinterpret_cast<char*>(pixels.data()), pixels.size());

        std::vector<uint8_t> rgbaData(width * height * 4);
        for (int i = 0; i < width * height; ++i) {
            rgbaData[i * 4 + 0] = pixels[i * 3 + 0]; // Red
            rgbaData[i * 4 + 1] = pixels[i * 3 + 1]; // Green
            rgbaData[i * 4 + 2] = pixels[i * 3 + 2]; // Blue
            rgbaData[i * 4 + 3] = 255;              // Alpha
        }

        return { width, height, 4, rgbaData };
    }

} // namespace almond
