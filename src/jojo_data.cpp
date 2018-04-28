//
// Created by benja on 4/28/2018.
//

#include "INIReader.h"

#include "jojo_data.hpp"

Config Config::readFromFile(std::string filename) {
    INIReader reader(filename);
    if (reader.ParseError() < 0) {
        throw std::runtime_error("cannot read config file");
    }
    uint32_t width = reader.GetInteger("window", "width", 800);
    uint32_t height = reader.GetInteger("window", "height", 480);
    return Config(width, height, 25, 2);
}

Config::Config(const uint32_t width, const uint32_t height, const uint32_t navigationScreenPercentage,
               const uint32_t deadzoneScreenPercentage) : width(width), height(height),
                                                          navigationScreenPercentage(navigationScreenPercentage),
                                                          deadzoneScreenPercentage(deadzoneScreenPercentage) {}

