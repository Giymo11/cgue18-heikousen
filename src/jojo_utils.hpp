//
// Created by giymo11 on 3/11/18.
//

#pragma once

#include <vector>
#include <iostream>


// Wrapper functions for aligned memory allocation
// There is currently no standard for this in C++ that works across all platforms and vendors, so we abstract this
void *alignedAlloc(size_t size, size_t alignment);

void alignedFree(void *data);

std::vector<char> readFile(const std::string &filename);

class Config {
private:
    Config(const uint32_t width,
           const uint32_t height,
           const uint32_t navigationScreenPercentage,
           const uint32_t deadzoneScreenPercentage,
           const bool vsync);

public:
    uint32_t width, height, navigationScreenPercentage, deadzoneScreenPercentage;
    bool vsync = true;

    static Config readFromFile(std::string filename);
};
