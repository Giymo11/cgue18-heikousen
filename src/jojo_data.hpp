//
// Created by benja on 3/12/2018.
//

#pragma once

class Config {
private:
    Config(const uint32_t width, const uint32_t height, const uint32_t navigationScreenPercentage,
           const uint32_t deadzoneScreenPercentage);

public:
    uint32_t width, height, navigationScreenPercentage, deadzoneScreenPercentage;

    static Config readFromFile(std::string filename);
};
