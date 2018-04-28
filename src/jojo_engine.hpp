

#pragma once


#include <vector>


#include <GLFW/glfw3.h>
#include "jojo_data.hpp"
#include "jojo_engine_helper.hpp"

class JojoWindow {
public:

    GLFWwindow *window;

    void startGlfw(Config &config);
    void shutdownGlfw();

    std::vector<const char *> getUsedExtensions();
    void createSurface(VkInstance instance, VkSurfaceKHR *surface);
};

class JojoEngine {

public:
    JojoWindow *jojoWindow;

    VkInstance instance;
    VkSurfaceKHR surface;

    VkPhysicalDevice chosenDevice;
    VkDevice device;
    VkQueue queue;

    VkCommandPool commandPool;
    VkDescriptorPool descriptorPool;

    void startVulkan();

    void initialieDescriptorPool(uint32_t count);

    void destroyDescriptorPool();

    void shutdownVulkan();
};

