//
// Created by benja on 4/28/2018.
//

#include <vector>
#include <iostream>

#include <vulkan/vulkan.h>

#include "jojo_vulkan_info.hpp"

void printStats(const VkPhysicalDevice physicalDevice, const VkSurfaceKHR surface) {

    VkPhysicalDeviceProperties properties;
    vkGetPhysicalDeviceProperties(physicalDevice, &properties);

    std::cout << "GPU Name: " << properties.deviceName << std::endl;
    uint32_t version = properties.apiVersion;
    std::cout << "Max API Version:           " <<
              VK_VERSION_MAJOR(version) << "." <<
              VK_VERSION_MINOR(version) << "." <<
              VK_VERSION_PATCH(version) << std::endl;

    version = properties.driverVersion;
    std::cout << "Driver Version:            " <<
              VK_VERSION_MAJOR(version) << "." <<
              VK_VERSION_MINOR(version) << "." <<
              VK_VERSION_PATCH(version) << std::endl;

    std::cout << "Vendor ID:                 " << properties.vendorID << std::endl;
    std::cout << "Device ID:                 " << properties.deviceID << std::endl;

    auto deviceType = properties.deviceType;
    auto deviceTypeDescription =
            (deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU ?
             "discrete" :
             (deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU ?
              "integrated" :
              "other"
             )
            );
    std::cout << "Device Type:               " << deviceTypeDescription << " (type " << deviceType << ")" << std::endl;
    std::cout << "discreteQueuePrioritis:    " << properties.limits.discreteQueuePriorities << std::endl;


    VkPhysicalDeviceFeatures features;
    vkGetPhysicalDeviceFeatures(physicalDevice, &features);
    // ...
    std::cout << "can it do multi-viewport:  " << features.multiViewport << std::endl;


    VkPhysicalDeviceMemoryProperties memoryProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);
    // ...


    uint32_t numberOfQueueFamilies = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &numberOfQueueFamilies, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilyProperties;
    queueFamilyProperties.resize(numberOfQueueFamilies);
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &numberOfQueueFamilies, queueFamilyProperties.data());

    std::cout << "Amount of Queue Families:  " << numberOfQueueFamilies << std::endl << std::endl;

    for (size_t i = 0; i < numberOfQueueFamilies; ++i) {
        std::cout << "Queue Family #" << i << std::endl;
        auto flags = queueFamilyProperties[i].queueFlags;
        std::cout << "VK_QUEUE_GRAPHICS_BIT " << ((flags & VK_QUEUE_GRAPHICS_BIT) != 0) << std::endl;
        std::cout << "VK_QUEUE_COMPUTE_BIT  " << ((flags & VK_QUEUE_COMPUTE_BIT) != 0) << std::endl;
        std::cout << "VK_QUEUE_TRANSFER_BIT " << ((flags & VK_QUEUE_TRANSFER_BIT) != 0) << std::endl;
        std::cout << "Queue Count:          " << queueFamilyProperties[i].queueCount << std::endl;
        std::cout << "Timestamp Valid Bits: " << queueFamilyProperties[i].timestampValidBits << std::endl;
    }


    VkSurfaceCapabilitiesKHR surfaceCapabilitiesKHR;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &surfaceCapabilitiesKHR);
    std::cout << std::endl << std::endl << "Surface Capabilities" << std::endl << std::endl;
    std::cout << "minImageCount: " << surfaceCapabilitiesKHR.minImageCount << std::endl;
    std::cout << "maxImageCount: " << surfaceCapabilitiesKHR.maxImageCount << std::endl; // a 0 means no limit
    std::cout << "maxImageArrayLayers: " << surfaceCapabilitiesKHR.maxImageArrayLayers << std::endl;
    // ...


    uint32_t numberOfSurfaceFormats;
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &numberOfSurfaceFormats, nullptr);

    std::vector<VkSurfaceFormatKHR> surfaceFormats;
    surfaceFormats.resize(numberOfSurfaceFormats);
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &numberOfSurfaceFormats, surfaceFormats.data());

    std::cout << std::endl << "Amount of Surface Formats: " << numberOfSurfaceFormats << std::endl;
    for (size_t i = 0; i < numberOfSurfaceFormats; ++i) {
        std::cout << "Format: " << surfaceFormats[i].format << std::endl;
    }


    uint32_t numberOfPresentationModes = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &numberOfPresentationModes, nullptr);

    std::vector<VkPresentModeKHR> presentationModes;
    presentationModes.resize(numberOfPresentationModes);
    vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &numberOfPresentationModes,
                                              presentationModes.data());

    std::cout << std::endl << "Amount of Presentation Modes: " << numberOfPresentationModes << std::endl;
    for (size_t i = 0; i < numberOfPresentationModes; ++i) {
        std::cout << "Mode " << presentationModes[i] << std::endl;
    }


    std::cout << std::endl;
}


void printInstanceLayers() {
    uint32_t numberOfLayers = 0;
    vkEnumerateInstanceLayerProperties(&numberOfLayers, nullptr);

    std::vector<VkLayerProperties> layers;
    layers.resize(numberOfLayers);
    vkEnumerateInstanceLayerProperties(&numberOfLayers, layers.data());

    std::cout << std::endl << "Amount of instance layers: " << numberOfLayers << std::endl << std::endl;
    for (size_t i = 0; i < numberOfLayers; ++i) {
        std::cout << "Name:            " << layers[i].layerName << std::endl;
        std::cout << "Spec Version:    " << layers[i].specVersion << std::endl;
        std::cout << "Description:     " << layers[i].description << std::endl << std::endl;
    }
}

void printInstanceExtensions() {
    uint32_t numberOfExtensions = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &numberOfExtensions, nullptr);

    std::vector<VkExtensionProperties> extensions;
    extensions.resize(numberOfExtensions);
    vkEnumerateInstanceExtensionProperties(nullptr, &numberOfExtensions, extensions.data());

    std::cout << std::endl << "Amount of instance extensions: " << numberOfExtensions << std::endl << std::endl;
    for (size_t i = 0; i < numberOfExtensions; ++i) {
        std::cout << "Name:            " << extensions[i].extensionName << std::endl;
        std::cout << "Spec Version:    " << extensions[i].specVersion << std::endl << std::endl;
    }
}