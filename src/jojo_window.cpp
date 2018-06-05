//
// Created by benja on 4/28/2018.
//

#include <iostream>

#include "jojo_window.hpp"

#include "jojo_vulkan_utils.hpp"


void onWindowResized(GLFWwindow *window, int newWidth, int newHeight) {
    if (newWidth > 0 && newHeight > 0) {
        //recreateSwapchain();
        //glfwSetCursorPos(window, newWidth / 2.0f, newHeight / 2.0f);
    }
}

void keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }

    /*
     *
     *     F1 - Help (if available)
     *     F2 - Frame Time on/off
     *     F3 - Wire Frame on/off
     *     F4-F7 - Enable/Disable effect (if necessary, see Effects)
     *     F8 - View-frustum Culling on/off
     */

    Config *config = static_cast<Config *>(glfwGetWindowUserPointer(window));

    if(key == GLFW_KEY_F2 && action == GLFW_PRESS) {
        config->isFrametimeOutputEnabled = !config->isFrametimeOutputEnabled;
        std::cout << "frametime is " << config->isFrametimeOutputEnabled << std::endl;
    }

    if(key == GLFW_KEY_F3 && action == GLFW_PRESS) {
        config->isWireframeEnabled = !config->isWireframeEnabled;
        config->rebuildPipelines();
        std::cout << "wireframe is " << config->isWireframeEnabled << std::endl;
    }

    // toggle HDR
    if(key == GLFW_KEY_F4 && action == GLFW_PRESS) {
        if(config->hdrMode > 1.5) {
            config->hdrMode = 0.0;
            std::cout << "HDR mode set to SDR" << std::endl;
        } else if(config->hdrMode > 0.5) {
            config->hdrMode = 2.0;
            std::cout << "HDR mode set to Exposure Mapping" << std::endl;
        } else {
            config->hdrMode = 1.0;
            std::cout << "HDR mode set to Reinhard Mapping" << std::endl;
        }
    }

}

void JojoWindow::startGlfw(Config &config) {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    auto moni = glfwGetPrimaryMonitor();
    auto mode = glfwGetVideoMode(moni);

    glfwWindowHint(GLFW_RED_BITS, mode->redBits);
    glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
    glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
    // TODO: make the refresh rate parameter actually do sth
    if (config.refreshrate == 0) {
        glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);
    } else {
        glfwWindowHint(GLFW_REFRESH_RATE, config.refreshrate);
    }

    window = glfwCreateWindow(config.width, config.height, "heikousen", nullptr, nullptr);
    glfwSetWindowSizeCallback(window, onWindowResized);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    glfwSetCursorPos(window, config.width / 2.0f, config.height / 2.0f);

    if (config.fullscreen) {
        glfwSetWindowMonitor(window, moni, 0, 0, mode->width, mode->height, mode->refreshRate);
        std::cout << "am fullscreen" << std::endl;
    }

    glfwSetWindowUserPointer(window, &config);
    glfwSetKeyCallback(window, keyCallback);
}

void JojoWindow::shutdownGlfw() {
    glfwDestroyWindow(window);
    glfwTerminate();
}

std::vector<const char *> JojoWindow::getUsedExtensions() {
    uint32_t indexOfNumberOfGlfwExtensions = 0;
    auto glfwExtensions = glfwGetRequiredInstanceExtensions(&indexOfNumberOfGlfwExtensions);
    // vector constructor parameters take begin and end pointer
    std::vector<const char *> usedExtensions(glfwExtensions, glfwExtensions + indexOfNumberOfGlfwExtensions);
    return usedExtensions;

}

void JojoWindow::createSurface(VkInstance instance, VkSurfaceKHR *surface) {
    VkResult result = glfwCreateWindowSurface(instance, window, nullptr, surface);
    ASSERT_VULKAN(result)
}

void JojoWindow::getWindowSize(int *newWidth, int *newHeight) {
    glfwGetWindowSize(window, newWidth, newHeight);
}

