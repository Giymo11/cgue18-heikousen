#pragma once
#define ENABLE_VALIDATION
#ifdef ENABLE_VALIDATION
#include <vulkan/vulkan.h>

VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback (
    VkDebugReportFlagsEXT       flags,
    VkDebugReportObjectTypeEXT  objectType,
    uint64_t                    object,
    size_t                      location,
    int32_t                     messageCode,
    const char*                 pLayerPrefix,
    const char*                 pMessage,
    void*                       pUserData
);

#endif