//
// Created by giymo11 on 3/11/18.
//

#pragma once

#include <vector>
#include <iostream>
#include <fstream>


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