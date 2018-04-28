//
// Created by giymo11 on 3/10/18.
//

#pragma once

#include <vulkan/vulkan.h>
#include <cstring>
#include <vector>

#include "debug_trap.h"


#define ASSERT_VULKAN(val)\
if(val != VK_SUCCESS)\
    psnip_trap();\


uint32_t findMemoryTypeIndex(VkPhysicalDevice chosenDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties);

uint32_t findMemoryTypeIndex(
	VkPhysicalDeviceMemoryProperties deviceProperties,
	uint32_t typeBits,
	VkMemoryPropertyFlags properties
);

bool isFormatSupported(VkPhysicalDevice chosenDeice, VkFormat format, VkImageTiling tiling,
                       VkFormatFeatureFlags featureFlags);

VkFormat findSupportedFormat(VkPhysicalDevice chosenDeice, std::vector<VkFormat> formats, VkImageTiling tiling,
                             VkFormatFeatureFlags featureFlags);

bool isStencilFormat(VkFormat format);

void createBuffer(VkDevice device, VkPhysicalDevice chosenDevice,
                  VkDeviceSize bufferSize, VkBufferUsageFlags bufferUsageFlags, VkBuffer *buffer,
                  VkMemoryPropertyFlags memoryPropertyFlags, VkDeviceMemory *deviceMemory);

void allocateAndBeginSingleUseBuffer(const VkDevice device, const VkCommandPool commandPool,
                                     VkCommandBuffer *commandBuffer);

void endAndSubmitCommandBuffer(const VkDevice device, const VkCommandPool commandPool, const VkQueue queue,
                               VkCommandBuffer commandBuffer);


void copyBuffer(VkDevice device, VkCommandPool commandPool, VkQueue queue,
                VkBuffer src, VkBuffer dst, VkDeviceSize size);

void createAndUploadBufferUntyped(VkDevice device, VkPhysicalDevice chosenDevice, VkCommandPool commandPool,
	VkQueue queue, VkBufferUsageFlags usageFlags, VkBuffer *buffer, VkDeviceMemory *deviceMemory,
	const uint8_t *data, VkDeviceSize dataSize);

void changeImageLayout(VkDevice device, VkCommandPool commandPool, VkQueue queue, VkImage image, VkFormat format,
                       VkImageLayout oldLayout, VkImageLayout newLayout);
void createImage(VkDevice device, VkPhysicalDevice chosenDevice, uint32_t width, uint32_t height, VkFormat format,
                 VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage &image,
                 VkDeviceMemory *imageMemory);

template<typename T>
inline void createAndUploadBuffer(VkDevice device, VkPhysicalDevice chosenDevice, VkCommandPool commandPool, VkQueue queue,
	std::vector<T> data, VkBufferUsageFlags usageFlags, VkBuffer *buffer,
	VkDeviceMemory *deviceMemory) {

	VkDeviceSize dataSize = sizeof(T) * data.size();
	createAndUploadBufferUntyped(device, chosenDevice, commandPool, queue, usageFlags, buffer, deviceMemory,
		reinterpret_cast<const uint8_t *>(data.data()), dataSize);
}
