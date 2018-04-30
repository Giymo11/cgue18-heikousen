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
    Config(uint32_t width,
           uint32_t height,
           int navigationScreenPercentage,
           int deadzoneScreenPercentage,
           bool vsync,
           bool fullscreen,
           uint32_t refreshrate);

public:
    uint32_t width, height, navigationScreenPercentage, deadzoneScreenPercentage, refreshrate;
    const bool vsync, fullscreen;

    static Config readFromFile(std::string filename);
};
