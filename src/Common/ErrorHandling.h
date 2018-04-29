#pragma once
#include <cstdlib>
#include <iostream>
#include <string>

// Use macros to include line number/file in crash

#define CHECK(result, msg) {\
if (!(result)) {\
std::cout << __FILE__ << ", " << __LINE__ << ": "; \
std::cout << msg; \
std::cout << std::endl;\
std::exit(-1);}}

#define CHECK_VK(result, msg) {\
if (result != VK_SUCCESS) {\
std::cout << __FILE__ << ", " << __LINE__ << ": "; \
std::cout << msg; \
std::cout << std::endl;\
std::exit(-1);}}
