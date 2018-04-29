//
// Created by benja on 4/28/2018.
//

#include "jojo_utils.hpp"

#include <fstream>

#include "INIReader.h"


std::vector<char> readFile(const std::string &filename) {
    std::cout << "Reading file " << std::endl;
    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    if (file) {
        size_t fileSize = file.tellg();
        std::vector<char> fileBuffer(fileSize);
        file.seekg(0);
        file.read(fileBuffer.data(), fileSize);
        file.close();
        return fileBuffer;
    } else {
        throw std::runtime_error("Failed to open file");
    }
}


Config Config::readFromFile(std::string filename) {
    INIReader reader(filename);
    if (reader.ParseError() < 0) {
        throw std::runtime_error("cannot read config file");
    }
    uint32_t width = reader.GetInteger("window", "width", 800);
    uint32_t height = reader.GetInteger("window", "height", 480);
    return Config(width, height, 25, 2);
}

Config::Config(const uint32_t width,
               const uint32_t height,
               const uint32_t navigationScreenPercentage,
               const uint32_t deadzoneScreenPercentage) : width(width),
                                                          height(height),
                                                          navigationScreenPercentage(navigationScreenPercentage),
                                                          deadzoneScreenPercentage(deadzoneScreenPercentage) {}



void *alignedAlloc(size_t size, size_t alignment) {
    void *data = nullptr;
#if defined(_MSC_VER) || defined(__MINGW32__)
    data = _aligned_malloc(size, alignment);
#else
    int res = posix_memalign(&data, alignment, size);
	if (res != 0)
		data = nullptr;
#endif
    return data;
}


void alignedFree(void *data) {
#if	defined(_MSC_VER) || defined(__MINGW32__)
    _aligned_free(data);
#else
    free(data);
#endif
}
