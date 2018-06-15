#pragma once


#include <vector>
#include <memory>
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

#include "jojo_vulkan_debug.hpp"

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

    VmaAllocator allocator;

    VkCommandPool commandPool;
    VkDescriptorPool descriptorPool;

    Rendering::DescriptorSets *descriptors = nullptr;

#ifdef ENABLE_VALIDATION
    PFN_vkCreateDebugReportCallbackEXT vkCreateDebugReportCallbackEXT;
    PFN_vkDebugReportMessageEXT vkDebugReportMessageEXT;
    PFN_vkDestroyDebugReportCallbackEXT vkDestroyDebugReportCallbackEXT;
    VkDebugReportCallbackEXT mCallback;
#endif

    void startVulkan();

    void shutdownVulkan();

    void initializeDescriptorPool(uint32_t uniformCount, uint32_t dynamicUniformCount, uint32_t samplerCount);
};

