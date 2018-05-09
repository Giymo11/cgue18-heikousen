#include "jojo_vulkan_debug.h"
#ifdef ENABLE_VALIDATION

#include <iostream>

VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback (
    VkDebugReportFlagsEXT       flags,
    VkDebugReportObjectTypeEXT  objectType,
    uint64_t                    object,
    size_t                      location,
    int32_t                     messageCode,
    const char*                 pLayerPrefix,
    const char*                 pMessage,
    void*                       pUserData
) {
    std::cerr << pMessage << std::endl;
    return VK_FALSE;
}

#endif