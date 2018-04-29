

#pragma once


#include <vector>
#include <memory>

#include <vulkan/vulkan.h>

class JojoWindow;
namespace Rendering {
class DescriptorSets;
}

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

    Rendering::DescriptorSets *descriptors = nullptr;

    void startVulkan();

    void destroyDescriptorPool();

    void shutdownVulkan();

    void initializeDescriptorPool(uint32_t uniformCount, uint32_t dynamicUniformCount, uint32_t samplerCount);
};

