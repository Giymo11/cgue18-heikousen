#pragma once

#include <cstdint>
#include "jojo_vulkan_utils.hpp"

void loadTexture(const VkDevice &device, const VkDeviceSize &size, const uint8_t *imageData,
	             size_t imageDataSize) {
	VkBuffer stagingBuffer;
	VkDeviceMemory stagingMem;
	VkBufferCreateInfo bufferCreateInfo = {};
	bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferCreateInfo.size = size;
	bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	ASSERT_VULKAN(vkCreateBuffer(device, &bufferCreateInfo, nullptr, &stagingBuffer));

	VkMemoryAllocateInfo memAllocInfo = {};
	VkMemoryRequirements memReqs = {};
	vkGetBufferMemoryRequirements(device, stagingBuffer, &memReqs);
	memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memAllocInfo.allocationSize = memReqs.size;
	// TODO
	// memAllocInfo.memoryTypeIndex
	ASSERT_VULKAN(vkAllocateMemory(device, &memAllocInfo, nullptr, &stagingMem));
	ASSERT_VULKAN(vkBindBufferMemory(device, stagingBuffer, stagingMem, 0));

	uint8_t *data;
	ASSERT_VULKAN(vkMapMemory(device, stagingMem, 0, memReqs.size, 0, reinterpret_cast<void **>(&data)));
	std::copy(imageData, imageData + imageDataSize, data);
	vkUnmapMemory(device, stagingMem);
}