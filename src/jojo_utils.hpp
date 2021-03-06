//
// Created by giymo11 on 3/11/18.
//

#pragma once

#include <vector>
#include <iostream>
#include <functional>


// Wrapper functions for aligned memory allocation
// There is currently no standard for this in C++ that works across all platforms and vendors, so we abstract this
void *alignedAlloc(size_t size, size_t alignment);

void alignedFree(void *data);

std::vector<char> readFile(const std::string &filename);

class Config {
private:
    Config(uint32_t width,
           uint32_t height,
           int navigationScreenPercentage,
           int deadzoneScreenPercentage,
           bool vsync,
           bool fullscreen,
           uint32_t refreshrate,
           float gamma,
           float hdrMode,
           std::string map,
           int dofTaps = 16,
           int normalMode = 2,
           bool isFrametimeOutputEnabled = false,
           bool isWireframeEnabled = false);

public:
    uint32_t width, height, navigationScreenPercentage, deadzoneScreenPercentage, refreshrate, normalMode;
    const bool vsync, fullscreen;
    float gamma, hdrMode;
    bool isFrametimeOutputEnabled, isWireframeEnabled;
    std::function<void()> rebuildPipelines;
    std::string map;

    float dofEnabled       = 1.0f;
    float dofFocalDistance = 7.0f;
    float dofFocalWidth    = 6.0f;
    int   dofTaps          = 16;

    static Config readFromFile(std::string filename);


};
