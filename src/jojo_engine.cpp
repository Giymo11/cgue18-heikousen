//
// Created by benja on 4/28/2018.
//

#include <iostream>

#include <vulkan/vulkan.h>

#include "jojo_engine.hpp"
#include "jojo_vulkan.hpp"
#include "jojo_vulkan_utils.hpp"
#include "jojo_vulkan_info.hpp"


void onWindowResized(GLFWwindow *window, int newWidth, int newHeight) {
    if (newWidth > 0 && newHeight > 0) {
        //recreateSwapchain();
    }
}

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
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

void JojoWindow::shutdownGlfw()  {
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




void JojoEngine::startVulkan() {
    VkResult result;

    auto usedExtensions = jojoWindow->getUsedExtensions();

    result = createInstance(&instance, usedExtensions);
    ASSERT_VULKAN(result)

    //printInstanceLayers();
    //printInstanceExtensions();

    jojoWindow->createSurface(instance, &surface);


    uint32_t numberOfPhysicalDevices = 0;
    // if passed nullptr as third parameter, outputs the number of GPUs to the second parameter
    result = vkEnumeratePhysicalDevices(instance, &numberOfPhysicalDevices, nullptr);
    ASSERT_VULKAN(result)

    std::vector<VkPhysicalDevice> physicalDevices;
    physicalDevices.resize(numberOfPhysicalDevices);
    // actually enumerates the GPUs for use
    result = vkEnumeratePhysicalDevices(instance, &numberOfPhysicalDevices, physicalDevices.data());
    ASSERT_VULKAN(result)

    std::cout << std::endl << "GPUs Found: " << numberOfPhysicalDevices << std::endl << std::endl;
    for (size_t i = 0; i < numberOfPhysicalDevices; ++i)
        printStats(physicalDevices[i], surface);

    chosenDevice = physicalDevices[0];     // TODO: choose right physical device
    uint32_t chosenQueueFamilyIndex = 0;        // TODO: choose the best queue family

    result = createLogicalDevice(chosenDevice, &device, chosenQueueFamilyIndex);
    ASSERT_VULKAN(result)

    vkGetDeviceQueue(device, chosenQueueFamilyIndex, 0, &queue);

    result = checkSurfaceSupport(chosenDevice, surface, chosenQueueFamilyIndex);
    ASSERT_VULKAN(result)

    result = createCommandPool(device, &commandPool, chosenQueueFamilyIndex);
    ASSERT_VULKAN(result)
}

void JojoEngine::initialieDescriptorPool(uint32_t count) {
    VkResult result = createDescriptorPool(device, &descriptorPool, count);
    ASSERT_VULKAN(result)
}

void JojoEngine::destroyDescriptorPool() {
    vkDestroyDescriptorPool(device, descriptorPool, nullptr);
}

void JojoEngine::shutdownVulkan() {
    vkDestroyCommandPool(device, commandPool, nullptr);
    vkDestroyDevice(device, nullptr);
    vkDestroySurfaceKHR(instance, surface, nullptr);
    vkDestroyInstance(instance, nullptr);
}

