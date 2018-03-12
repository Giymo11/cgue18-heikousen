//
// Created by benja on 3/12/2018.
//

#pragma once

#include "INIReader.h"

class Config {
private:
    Config(const uint32_t width, const uint32_t height) : width(width), height(height) {}

public:
    uint32_t width, height;

    static Config readFromFile(std::string filename) {
        INIReader reader(filename);
        if (reader.ParseError() < 0) {
            throw std::runtime_error("cannot read config file");
        }
        uint32_t width = reader.GetInteger("window", "width", 800);
        uint32_t height = reader.GetInteger("window", "height", 480);
        return Config(width, height);
    }
};
