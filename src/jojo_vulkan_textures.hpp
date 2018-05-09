#pragma once

#include <vulkan/vulkan.h>

namespace Textures {

void create (
    VkDevice device,
    VkPhysicalDeviceMemoryProperties memoryProperties,
    VkDeviceSize size,
    VkFormat format,
    uint32_t mipLevels,
    uint32_t width,
    uint32_t height,
    VkBuffer *stagingBuffer,
    VkDeviceMemory *stagingMem,
    VkImage *image,
    VkDeviceMemory *imageMemory
);

void stage (
    VkDevice device,
    VkDeviceMemory stagingMem,
    const uint8_t *imageData,
    VkDeviceSize imageDataSize
);

void transfer (
    VkCommandBuffer cmd,
    VkBuffer staging,
    VkImage image,
    const VkBufferImageCopy *copy,
    uint32_t copySize,
    uint32_t mipLevels
);

VkSampler sampler (
    VkDevice device,
    uint32_t mipLevels
);

VkImageView view (
    VkDevice device,
    VkImage image,
    VkFormat format,
    uint32_t mipLevels
);

VkDescriptorImageInfo generateTexture (
    VkDevice device,
    const VkPhysicalDeviceMemoryProperties &memoryProperties,
    VkCommandPool commandPool,
    VkQueue queue,
    uint32_t color
);

VkDescriptorImageInfo fontTexture (
    VkDevice device,
    const VkPhysicalDeviceMemoryProperties &memoryProperties,
    VkCommandPool commandPool,
    VkQueue queue
);

}