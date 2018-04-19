#pragma once

#include <cstdint>
#include "jojo_vulkan_utils.hpp"

namespace Textures {

void create(const VkDevice &device, const VkDeviceSize &size, const VkFormat &format,
	        uint32_t mipLevels, uint32_t width, uint32_t height, VkBuffer *stagingBuffer,
	        VkDeviceMemory *stagingMem, VkImage *image, VkDeviceMemory *imageMemory);

void stage(const VkDevice &device, const VkDeviceMemory &stagingMem, const uint8_t *imageData,
	       size_t imageDataSize);

void transfer(uint32_t mipLevels);

}