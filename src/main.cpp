#include <iostream>
#include <vulkan/vulkan.h>

#include "debug_trap.h"

#define ASSERT_VULKAN(val)\
if(val != VK_SUCCESS)\
    psnip_trap();\


int initVulkan();

VkInstance instance;

int main () {
    std::cout << "Hello World!" << std::endl;

    initVulkan();

    return 0;
}

int initVulkan() {
    VkResult result;


    VkApplicationInfo applicationInfo = {};
    // describes what kind of struct this is, because otherwise the type info is lost when passed to the driver
    applicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    // used for extensions
    applicationInfo.pNext = NULL;
    applicationInfo.pApplicationName = "heikousen";
    applicationInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    applicationInfo.pEngineName = "JoJo Engine";
    applicationInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    applicationInfo.apiVersion = VK_API_VERSION_1_0;


    VkInstanceCreateInfo instanceCreateInfo = {};
    instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceCreateInfo.pNext = NULL;
    // reserved for future use
    instanceCreateInfo.flags = 0;
    instanceCreateInfo.pApplicationInfo = &applicationInfo;
    // used for debugging, profiling, error handling,
    instanceCreateInfo.enabledLayerCount = 0;
    instanceCreateInfo.ppEnabledLayerNames = NULL;
    // used to extend vulkan functionality
    instanceCreateInfo.enabledExtensionCount = 0;
    instanceCreateInfo.ppEnabledExtensionNames = NULL;


    // vulkan instance is similar to OpenGL context
    // by telling vulkan which extensions we plan on using, it can disregard all others -> performance gain in comparison to openGL
    result = vkCreateInstance(&instanceCreateInfo, NULL, &instance);
    ASSERT_VULKAN(result);

    vkDestroyInstance(instance, NULL);

    return 0;
}