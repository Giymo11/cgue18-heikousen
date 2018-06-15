//
// Created by benja on 4/28/2018.
//

#include <iostream>
#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

#include "jojo_engine.hpp"
#include "jojo_window.hpp"
#include "jojo_vulkan.hpp"
#include "jojo_vulkan_utils.hpp"
#include "jojo_vulkan_info.hpp"

#include "Rendering/DescriptorSets.h"


void JojoEngine::startVulkan() {
    VkResult result;

    auto usedExtensions = jojoWindow->getUsedExtensions();

    result = createInstance(&instance, usedExtensions);
    ASSERT_VULKAN (result);

#ifdef ENABLE_VALIDATION
    vkCreateDebugReportCallbackEXT =
        reinterpret_cast<PFN_vkCreateDebugReportCallbackEXT>
        (vkGetInstanceProcAddr (instance, "vkCreateDebugReportCallbackEXT"));
    vkDebugReportMessageEXT =
        reinterpret_cast<PFN_vkDebugReportMessageEXT>
        (vkGetInstanceProcAddr (instance, "vkDebugReportMessageEXT"));
    vkDestroyDebugReportCallbackEXT =
        reinterpret_cast<PFN_vkDestroyDebugReportCallbackEXT>
        (vkGetInstanceProcAddr (instance, "vkDestroyDebugReportCallbackEXT"));

    VkDebugReportCallbackCreateInfoEXT callbackCreateInfo;
    callbackCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT;
    callbackCreateInfo.pNext = nullptr;
    callbackCreateInfo.flags =
        VK_DEBUG_REPORT_ERROR_BIT_EXT |
        VK_DEBUG_REPORT_WARNING_BIT_EXT |
        VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT |
        VK_DEBUG_REPORT_INFORMATION_BIT_EXT;
    callbackCreateInfo.pfnCallback = &debugCallback;
    callbackCreateInfo.pUserData = nullptr;

    result = vkCreateDebugReportCallbackEXT (instance, &callbackCreateInfo, nullptr, &mCallback);
    ASSERT_VULKAN (result);
#endif

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

    VmaAllocatorCreateInfo allocatorInfo = {};
    allocatorInfo.physicalDevice = chosenDevice;
    allocatorInfo.device = device;
    ASSERT_VULKAN (vmaCreateAllocator (&allocatorInfo, &allocator));

    vkGetDeviceQueue(device, chosenQueueFamilyIndex, 0, &queue);

    result = checkSurfaceSupport(chosenDevice, surface, chosenQueueFamilyIndex);
    ASSERT_VULKAN (result);

    result = createCommandPool(device, &commandPool, chosenQueueFamilyIndex);
    ASSERT_VULKAN (result);
}

void JojoEngine::initializeDescriptorPool(uint32_t uniformCount,
                                         uint32_t dynamicUniformCount,
                                         uint32_t samplerCount) {
    VkResult result = createDescriptorPool(device, &descriptorPool, uniformCount, dynamicUniformCount, samplerCount);
    ASSERT_VULKAN (result);

    descriptors = new Rendering::DescriptorSets (device);
    descriptors->allocate (descriptorPool);
}

void JojoEngine::shutdownVulkan() {
    delete descriptors;
    descriptors = nullptr;
    vkDestroyCommandPool(device, commandPool, nullptr);
    vmaDestroyAllocator (allocator);
    vkDestroyDevice(device, nullptr);
    vkDestroySurfaceKHR(instance, surface, nullptr);
#ifdef ENABLE_VALIDATION
    vkDestroyDebugReportCallbackEXT (instance, mCallback, nullptr);
#endif
    vkDestroyInstance(instance, nullptr);
}

