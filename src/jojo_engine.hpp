

#pragma once


#include <vector>

#include <vulkan/vulkan.h>

class JojoWindow;


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

