//
// Created by benja on 4/28/2018.
//

#include "jojo_engine.hpp"

#include <iostream>

#include "jojo_window.hpp"
#include "jojo_vulkan.hpp"
#include "jojo_vulkan_utils.hpp"
#include "jojo_vulkan_info.hpp"

#include "Rendering/DescriptorSets.h"


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
    vkDestroyDevice(device, nullptr);
    vkDestroySurfaceKHR(instance, surface, nullptr);
    vkDestroyInstance(instance, nullptr);
}

