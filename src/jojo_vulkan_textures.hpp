#pragma once

#include <vulkan/vulkan.h>

namespace Textures {

VkDescriptorImageInfo generateTextureArray (
    VkDevice device,
    const VkPhysicalDeviceMemoryProperties &memoryProperties,
    VkCommandPool commandPool,
    VkQueue queue
);

VkDescriptorImageInfo fontTexture (
    VkDevice device,
    const VkPhysicalDeviceMemoryProperties &memoryProperties,
    VkCommandPool commandPool,
    VkQueue queue
);

struct Texture {
    VkImage        image;
    VkDeviceMemory memory;
    VkSampler      sampler;
    VkImageView    view;
};

}