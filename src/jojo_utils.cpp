//
// Created by benja on 4/28/2018.
//

#include "jojo_utils.hpp"

#include <fstream>

#include "INIReader.h"


std::vector<char> readFile(const std::string &filename) {
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
    bool vsync = reader.GetBoolean("window", "vsync", true);
    bool fullscreen = reader.GetBoolean("window", "fullscreen", false);
    uint32_t refreshrate = reader.GetInteger("window", "refreshrate", 60);
    float gamma = static_cast<float>(reader.GetReal ("window", "gamma", 1.22));
    int dofTaps = reader.GetInteger("postproc", "doftaps", 16);
    auto map = reader.Get("gameplay", "map", "2");
    return Config(width, height, 25, 2, vsync, fullscreen, refreshrate, gamma, 1.0, map, dofTaps);
}

Config::Config(uint32_t width,
               uint32_t height,
               int navigationScreenPercentage,
               int deadzoneScreenPercentage,
               bool vsync,
               bool fullscreen,
               uint32_t refreshrate,
               float gamma,
               float hdrMode,
               std::string map,
               int dofTaps,
               int normalMode,
               bool isFrametimeOutputEnabled,
               bool isWireframeEnabled) : width(width),
                              height(height),
                              navigationScreenPercentage(navigationScreenPercentage),
                              deadzoneScreenPercentage(deadzoneScreenPercentage),
                              vsync(vsync),
                              fullscreen(fullscreen),
                              refreshrate(refreshrate),
                              gamma(gamma),
                              hdrMode(hdrMode),
                              normalMode(normalMode),
                              isFrametimeOutputEnabled(isFrametimeOutputEnabled),
                              isWireframeEnabled(isWireframeEnabled),
                              map(map),
                              dofTaps(dofTaps) {}


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
#if    defined(_MSC_VER) || defined(__MINGW32__)
    _aligned_free(data);
#else
    free(data);
#endif
}
