#include <iostream>
#include <vector>

// #include <vulkan/vulkan.h>
#define GLFW_INCLUDE_VULKAN

#include <GLFW/glfw3.h>

#include "debug_trap.h"

#define ASSERT_VULKAN(val)\
if(val != VK_SUCCESS)\
    psnip_trap();\


VkInstance instance;
VkSurfaceKHR surface;
VkDevice device;

GLFWwindow *window;


void printStats(VkPhysicalDevice &device);

void startGlfw() {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    window = glfwCreateWindow(1200, 720, "heikousen", nullptr, nullptr);
}

void startVulkan() {
    VkResult result;

    VkApplicationInfo applicationInfo = {};
    // describes what kind of struct this is, because otherwise the type info is lost when passed to the driver
    applicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    // used for extensions
    applicationInfo.pNext = nullptr;
    applicationInfo.pApplicationName = "heikousen";
    applicationInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    applicationInfo.pEngineName = "JoJo Engine";
    applicationInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    applicationInfo.apiVersion = VK_API_VERSION_1_0;


    uint32_t numberOfLayers = 0;
    vkEnumerateInstanceLayerProperties(&numberOfLayers, nullptr);
    std::vector<VkLayerProperties> layers;
    layers.resize(numberOfLayers);

    vkEnumerateInstanceLayerProperties(&numberOfLayers, layers.data());

    std::cout << std::endl << "Amount of instance layers: " << numberOfLayers << std::endl << std::endl;
    for (int i = 0; i < numberOfLayers; ++i) {
        std::cout << "Name:            " << layers[i].layerName << std::endl;
        std::cout << "Spec Version:    " << layers[i].specVersion << std::endl;
        std::cout << "Description:     " << layers[i].description << std::endl << std::endl;
    }


    uint32_t numberOfExtensions = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &numberOfExtensions, nullptr);
    std::vector<VkExtensionProperties> extensions;
    extensions.resize(numberOfExtensions);
    vkEnumerateInstanceExtensionProperties(nullptr, &numberOfExtensions, extensions.data());

    std::cout << std::endl << "Amount of instance extensions: " << numberOfExtensions << std::endl << std::endl;
    for (int i = 0; i < numberOfExtensions; ++i) {
        std::cout << "Name:            " << extensions[i].extensionName << std::endl;
        std::cout << "Spec Version:    " << extensions[i].specVersion << std::endl << std::endl;
    }

    const std::vector<const char *> usedLayers = {
            "VK_LAYER_LUNARG_standard_validation"
    };

    uint32_t indexOfNumberOfGlfwExtensions = 0;
    auto glfwExtensions = glfwGetRequiredInstanceExtensions(&indexOfNumberOfGlfwExtensions);

    // vector constructor parameters take begin and end pointer
    std::vector<const char *> usedExtensions(glfwExtensions, glfwExtensions + indexOfNumberOfGlfwExtensions);
    // TODO: add validation layer callback extension -
    // TODO: https://vulkan-tutorial.com/Drawing_a_triangle/Setup/Validation_layers#page_Message_callback


    VkInstanceCreateInfo instanceCreateInfo = {};
    instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceCreateInfo.pNext = nullptr;
    // Flags are reserved for future use
    instanceCreateInfo.flags = 0;
    instanceCreateInfo.pApplicationInfo = &applicationInfo;
    // Layers are used for debugging, profiling, error handling,
    instanceCreateInfo.enabledLayerCount = static_cast<uint32_t>(usedLayers.size());
    instanceCreateInfo.ppEnabledLayerNames = usedLayers.data();
    // Extensions are used to extend vulkan functionality
    instanceCreateInfo.enabledExtensionCount = usedExtensions.size();
    instanceCreateInfo.ppEnabledExtensionNames = usedExtensions.data();


    // vulkan instance is similar to OpenGL context
    // by telling vulkan which extensions we plan on using, it can disregard all others
    // -> performance gain in comparison to openGL
    result = vkCreateInstance(&instanceCreateInfo, nullptr, &instance);
    ASSERT_VULKAN(result);


    result = glfwCreateWindowSurface(instance, window, nullptr, &surface);
    ASSERT_VULKAN(result);


    uint32_t numberOfPhysicalDevices = 0;

    // if passed nullptr as third parameter, outputs the number of GPUs to the second parameter
    result = vkEnumeratePhysicalDevices(instance, &numberOfPhysicalDevices, nullptr);
    ASSERT_VULKAN(result);

    std::vector<VkPhysicalDevice> physicalDevices;
    physicalDevices.resize(numberOfPhysicalDevices);
    // actually enumerates the GPUs for use
    result = vkEnumeratePhysicalDevices(instance, &numberOfPhysicalDevices, physicalDevices.data());
    ASSERT_VULKAN(result);

    std::cout << std::endl << "GPUs Found: " << numberOfPhysicalDevices << std::endl << std::endl;

    for (int i = 0; i < numberOfPhysicalDevices; ++i) {
        printStats(physicalDevices[i]);
    }


    float queuePriorities[]{1.0f};

    VkDeviceQueueCreateInfo deviceQueueCreateInfo;
    deviceQueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    deviceQueueCreateInfo.pNext = nullptr;
    deviceQueueCreateInfo.flags = 0;
    deviceQueueCreateInfo.queueFamilyIndex = 0; // TODO: choose the best queue family
    deviceQueueCreateInfo.queueCount = 1; // TODO: check how many families are supported, 4 would be better
    deviceQueueCreateInfo.pQueuePriorities = queuePriorities;

    VkPhysicalDeviceFeatures usedFeatures = {};

    VkDeviceCreateInfo deviceCreateInfo;
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.pNext = nullptr;
    deviceCreateInfo.flags = 0;
    deviceCreateInfo.queueCreateInfoCount = 1;
    deviceCreateInfo.pQueueCreateInfos = &deviceQueueCreateInfo;
    deviceCreateInfo.enabledLayerCount = 0;
    deviceCreateInfo.ppEnabledLayerNames = nullptr;
    deviceCreateInfo.enabledExtensionCount = 0;
    deviceCreateInfo.ppEnabledExtensionNames = nullptr;
    deviceCreateInfo.pEnabledFeatures = &usedFeatures;


    // TODO: choose right physical device
    result = vkCreateDevice(physicalDevices[0], &deviceCreateInfo, nullptr, &device);
    ASSERT_VULKAN(result)

    VkQueue queue;
    vkGetDeviceQueue(device, 0, 0, &queue);
}

void gameloop() {
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
    }
}

void shutdownVulkan() {
    // block until vulkan has finished
    vkDeviceWaitIdle(device);

    vkDestroyDevice(device, nullptr);
    vkDestroySurfaceKHR(instance, surface, nullptr);
    vkDestroyInstance(instance, nullptr);
}

void shutdownGlfw() {
    glfwDestroyWindow(window);
}

int main() {
    std::cout << "Hello World!" << std::endl << std::endl;

    startGlfw();
    startVulkan();

    gameloop();

    shutdownVulkan();
    shutdownGlfw();

    return 0;
}

void printStats(VkPhysicalDevice &device) {

    VkPhysicalDeviceProperties properties;
    vkGetPhysicalDeviceProperties(device, &properties);

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
    vkGetPhysicalDeviceFeatures(device, &features);
    // ...
    std::cout << "can it do multi-viewport:  " << features.multiViewport << std::endl;


    VkPhysicalDeviceMemoryProperties memoryProperties;
    vkGetPhysicalDeviceMemoryProperties(device, &memoryProperties);
    // ...


    uint32_t numberOfQueueFamilies = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &numberOfQueueFamilies, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilyProperties;
    queueFamilyProperties.resize(numberOfQueueFamilies);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &numberOfQueueFamilies, queueFamilyProperties.data());

    std::cout << "Amount of Queue Families:  " << numberOfQueueFamilies << std::endl << std::endl;

    for (int i = 0; i < numberOfQueueFamilies; ++i) {
        std::cout << "Queue Family #" << i << std::endl;
        auto flags = queueFamilyProperties[i].queueFlags;
        std::cout << "VK_QUEUE_GRAPHICS_BIT " << ((flags & VK_QUEUE_GRAPHICS_BIT) != 0) << std::endl;
        std::cout << "VK_QUEUE_COMPUTE_BIT  " << ((flags & VK_QUEUE_COMPUTE_BIT) != 0) << std::endl;
        std::cout << "VK_QUEUE_TRANSFER_BIT " << ((flags & VK_QUEUE_TRANSFER_BIT) != 0) << std::endl;
        std::cout << "Queue Count:          " << queueFamilyProperties[i].queueCount << std::endl;
        std::cout << "Timestamp Valid Bits: " << queueFamilyProperties[i].timestampValidBits << std::endl;
    }


    VkSurfaceCapabilitiesKHR surfaceCapabilitiesKHR;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &surfaceCapabilitiesKHR);
    std::cout << std::endl << std::endl << "Surface Capabilities" << std::endl << std::endl;
    std::cout << "maxImageArrayLayers: " << surfaceCapabilitiesKHR.maxImageArrayLayers << std::endl;
    // ...
    // TODO: check if triple buffering is supported with surfaceCapabilitiesKHR.maxImageCount


    uint32_t numberOfSurfaceFormats;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &numberOfSurfaceFormats, nullptr);

    std::vector<VkSurfaceFormatKHR> surfaceFormats;
    surfaceFormats.resize(numberOfSurfaceFormats);
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &numberOfSurfaceFormats, surfaceFormats.data());

    std::cout << std::endl << "Amount of Surface Formats: " << numberOfSurfaceFormats << std::endl;
    for (int i = 0; i < numberOfSurfaceFormats; ++i) {
        std::cout << "Format: " << surfaceFormats[i].format << std::endl;
    }
    // TODO: check for availability of what we want, for my GPU: 44, 50

    uint32_t numberOfPresentationModes = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &numberOfPresentationModes, nullptr);

    std::vector<VkPresentModeKHR> presentationModes;
    presentationModes.resize(numberOfPresentationModes);
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &numberOfPresentationModes, presentationModes.data());

    std::cout << std::endl << "Amount of Presentation Modes: " << numberOfPresentationModes << std::endl;
    for (int i = 0; i < numberOfPresentationModes; ++i) {
        std::cout << "Mode " << presentationModes[i] << std::endl;
    }
    // TODO: check if mailbox mode is supported, otherwise take FIFO


    std::cout << std::endl;
}
