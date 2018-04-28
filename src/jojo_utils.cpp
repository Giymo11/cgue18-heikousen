//
// Created by benja on 4/28/2018.
//

#include <fstream>

#include "jojo_utils.hpp"

std::vector<char> readFile(const std::string &filename){
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