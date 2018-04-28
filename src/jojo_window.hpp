//
// Created by benja on 4/28/2018.
//

#pragma once

#include <GLFW/glfw3.h>

#include "jojo_utils.hpp"

class JojoWindow {
public:

    GLFWwindow *window;

    void startGlfw(Config &config);

    void shutdownGlfw();

    std::vector<const char *> getUsedExtensions();

    void createSurface(VkInstance instance, VkSurfaceKHR *surface);

    void getWindowSize(int *newWidth, int *newHeight);
};