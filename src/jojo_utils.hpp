//
// Created by giymo11 on 3/11/18.
//

#pragma once

#include <vector>
#include <iostream>


std::vector<char> readFile(const std::string &filename);

class Config {
private:
    Config(const uint32_t width,
           const uint32_t height,
           const uint32_t navigationScreenPercentage,
           const uint32_t deadzoneScreenPercentage);

public:
    uint32_t width, height, navigationScreenPercentage, deadzoneScreenPercentage;

    static Config readFromFile(std::string filename);
};
