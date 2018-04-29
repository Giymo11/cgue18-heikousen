//
// Created by benja on 4/28/2018.
//

#include "jojo_window.hpp"

#include "jojo_vulkan_utils.hpp"


void onWindowResized(GLFWwindow *window, int newWidth, int newHeight) {
    if (newWidth > 0 && newHeight > 0) {
        //recreateSwapchain();
    }
}

void keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }

}

void JojoWindow::startGlfw(Config &config) {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    window = glfwCreateWindow(config.width, config.height, "heikousen", nullptr, nullptr);
    glfwSetWindowSizeCallback(window, onWindowResized);

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    glfwSetCursorPos(window, config.width / 2.0f, config.height / 2.0f);

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

