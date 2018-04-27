//
// Created by giymo11 on 3/10/18.
//

#pragma once


#include <cstring>

#include "debug_trap.h"


#define ASSERT_VULKAN(val)\
if(val != VK_SUCCESS)\
    psnip_trap();\

uint32_t findMemoryTypeIndex(VkPhysicalDevice chosenDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties physicalDeviceMemoryProperties;
    vkGetPhysicalDeviceMemoryProperties(chosenDevice, &physicalDeviceMemoryProperties);

    for (uint32_t i = 0; i < physicalDeviceMemoryProperties.memoryTypeCount; ++i) {
        // typefilter is a bitfield marking the indices of valid memory types
        // also, check if that memory type has all the properties we want set
        if (typeFilter & (1 << i) &&
            (physicalDeviceMemoryProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }
}

uint32_t findMemoryTypeIndex(
	VkPhysicalDeviceMemoryProperties deviceProperties,
	uint32_t typeBits,
	VkMemoryPropertyFlags properties
) {
	for (uint32_t i = 0; i < deviceProperties.memoryTypeCount; i++) {
		if ((typeBits & 1) == 1) {
			if ((deviceProperties.memoryTypes[i].propertyFlags & properties) == properties)
				return i;
		}
		typeBits >>= 1;
	}

	psnip_trap();
}

bool isFormatSupported(VkPhysicalDevice chosenDeice, VkFormat format, VkImageTiling tiling,
                       VkFormatFeatureFlags featureFlags) {
    VkFormatProperties formatProperties;
    vkGetPhysicalDeviceFormatProperties(chosenDeice, format, &formatProperties);

    return ((tiling == VK_IMAGE_TILING_LINEAR &&
             (formatProperties.linearTilingFeatures & featureFlags) == featureFlags) ||
            (tiling == VK_IMAGE_TILING_OPTIMAL &&
             (formatProperties.optimalTilingFeatures & featureFlags) == featureFlags));
}

VkFormat findSupportedFormat(VkPhysicalDevice chosenDeice, std::vector<VkFormat> formats, VkImageTiling tiling,
                             VkFormatFeatureFlags featureFlags) {
    for (VkFormat format : formats) {
        if (isFormatSupported(chosenDeice, format, tiling, featureFlags))
            return format;
    }
    throw std::runtime_error("No format supported");
}

bool isStencilFormat(VkFormat format) {
    return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}

void createBuffer(VkDevice device, VkPhysicalDevice chosenDevice,
                  VkDeviceSize bufferSize, VkBufferUsageFlags bufferUsageFlags, VkBuffer *buffer,
                  VkMemoryPropertyFlags memoryPropertyFlags, VkDeviceMemory *deviceMemory) {
    VkResult result;
    VkBufferCreateInfo bufferCreateInfo;
    bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferCreateInfo.pNext = nullptr;
    bufferCreateInfo.flags = 0;
    bufferCreateInfo.size = bufferSize;
    bufferCreateInfo.usage = bufferUsageFlags;
    bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    bufferCreateInfo.queueFamilyIndexCount = 0;
    bufferCreateInfo.pQueueFamilyIndices = nullptr;

    result = vkCreateBuffer(device, &bufferCreateInfo, nullptr, buffer);
    ASSERT_VULKAN(result)

    VkMemoryRequirements memoryRequirements;
    vkGetBufferMemoryRequirements(device, *buffer, &memoryRequirements);

    VkMemoryAllocateInfo memoryAllocateInfo;
    memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memoryAllocateInfo.pNext = nullptr;
    memoryAllocateInfo.allocationSize = memoryRequirements.size;
    memoryAllocateInfo.memoryTypeIndex = findMemoryTypeIndex(chosenDevice, memoryRequirements.memoryTypeBits,
                                                             memoryPropertyFlags);

    result = vkAllocateMemory(device, &memoryAllocateInfo, nullptr, deviceMemory);
    ASSERT_VULKAN(result)

    result = vkBindBufferMemory(device, *buffer, *deviceMemory, 0);
    ASSERT_VULKAN(result)
}

void allocateAndBeginSingleUseBuffer(const VkDevice device, const VkCommandPool commandPool,
                                     VkCommandBuffer *commandBuffer) {
    VkCommandBufferAllocateInfo commandBufferAllocateInfo;
    commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    commandBufferAllocateInfo.pNext = nullptr;
    commandBufferAllocateInfo.commandPool = commandPool;
    commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    commandBufferAllocateInfo.commandBufferCount = 1;

    VkResult result = vkAllocateCommandBuffers(device, &commandBufferAllocateInfo, commandBuffer);
    ASSERT_VULKAN(result)

    VkCommandBufferBeginInfo commandBufferBeginInfo;
    commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    commandBufferBeginInfo.pNext = nullptr;
    commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    commandBufferBeginInfo.pInheritanceInfo = nullptr;

    result = vkBeginCommandBuffer(*commandBuffer, &commandBufferBeginInfo);
    ASSERT_VULKAN(result)
}

void endAndSubmitCommandBuffer(const VkDevice device, const VkCommandPool commandPool, const VkQueue queue,
                               VkCommandBuffer commandBuffer) {
    VkResult result = vkEndCommandBuffer(commandBuffer);
    ASSERT_VULKAN(result)

    VkSubmitInfo submitInfo;
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.pNext = nullptr;
    submitInfo.waitSemaphoreCount = 0;
    submitInfo.pWaitSemaphores = nullptr;
    submitInfo.pWaitDstStageMask = nullptr;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;
    submitInfo.signalSemaphoreCount = 0;
    submitInfo.pSignalSemaphores = nullptr;

    // GRAPHICS queues are always able to TRANSFER, TODO: check for separate TRANSFER queue
    result = vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
    ASSERT_VULKAN(result)

    // could wait for fence instead
    result = vkQueueWaitIdle(queue);
    ASSERT_VULKAN(result)

    vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
}


void copyBuffer(VkDevice device, VkCommandPool commandPool, VkQueue queue,
                VkBuffer src, VkBuffer dst, VkDeviceSize size) {

    VkCommandBuffer commandBuffer;
    allocateAndBeginSingleUseBuffer(device, commandPool, &commandBuffer);

    VkBufferCopy bufferCopy;
    bufferCopy.srcOffset = 0;
    bufferCopy.dstOffset = 0;
    bufferCopy.size = size;

    vkCmdCopyBuffer(commandBuffer, src, dst, 1, &bufferCopy);

    endAndSubmitCommandBuffer(device, commandPool, queue, commandBuffer);
}

template<typename T>
void createAndUploadBuffer(VkDevice device, VkPhysicalDevice chosenDevice, VkCommandPool commandPool, VkQueue queue,
                           std::vector<T> data, VkBufferUsageFlags usageFlags, VkBuffer *buffer,
                           VkDeviceMemory *deviceMemory) {

    VkDeviceSize bufferSize = sizeof(T) * data.size();

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    createBuffer(device, chosenDevice, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, &stagingBuffer,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &stagingBufferMemory);

    void *rawData;
    VkResult result = vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &rawData);
    ASSERT_VULKAN(result)
    memcpy(rawData, data.data(), bufferSize);
    vkUnmapMemory(device, stagingBufferMemory);

    createBuffer(device, chosenDevice, bufferSize, usageFlags | VK_BUFFER_USAGE_TRANSFER_DST_BIT, buffer,
                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, deviceMemory);

    copyBuffer(device, commandPool, queue, stagingBuffer, *buffer, bufferSize);

    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingBufferMemory, nullptr);
}


void changeImageLayout(VkDevice device, VkCommandPool commandPool, VkQueue queue, VkImage image, VkFormat format,
                       VkImageLayout oldLayout, VkImageLayout newLayout) {
    VkCommandBuffer commandBuffer;
    allocateAndBeginSingleUseBuffer(device, commandPool, &commandBuffer);

    VkImageMemoryBarrier imageMemoryBarrier;
    imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    imageMemoryBarrier.pNext = nullptr;
    imageMemoryBarrier.oldLayout = oldLayout;
    imageMemoryBarrier.newLayout = newLayout;
    imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    imageMemoryBarrier.image = image;

    VkPipelineStageFlags sourceStage;
    VkPipelineStageFlags destinationStage;

    if (oldLayout == VK_IMAGE_LAYOUT_PREINITIALIZED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {

        imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
        imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
               newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {

        imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
               newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {

        imageMemoryBarrier.srcAccessMask = 0;
        imageMemoryBarrier.dstAccessMask =
                VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    } else {
        throw std::invalid_argument("Image layout transition not yet supported");
    }

    imageMemoryBarrier.subresourceRange.baseMipLevel = 0;
    imageMemoryBarrier.subresourceRange.levelCount = 1;
    imageMemoryBarrier.subresourceRange.baseArrayLayer = 0;
    imageMemoryBarrier.subresourceRange.layerCount = 1;

    if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
        imageMemoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        if (isStencilFormat(format)) {
            imageMemoryBarrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
        }
    } else {
        imageMemoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    }

    vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0,
                         nullptr, 0, nullptr, 1, &imageMemoryBarrier);
}

void createImage(VkDevice device, VkPhysicalDevice chosenDevice, uint32_t width, uint32_t height, VkFormat format,
                 VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage &image,
                 VkDeviceMemory *imageMemory) {
    VkImageCreateInfo imageInfo;


    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.pNext = nullptr;
    imageInfo.flags = 0;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
    imageInfo.tiling = tiling;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = usage;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.queueFamilyIndexCount = 0;
    imageInfo.pQueueFamilyIndices = nullptr;

    VkResult result = vkCreateImage(device, &imageInfo, nullptr, &image);
    ASSERT_VULKAN(result)

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(device, image, &memRequirements);

    VkMemoryAllocateInfo allocInfo;
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.pNext = nullptr;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryTypeIndex(chosenDevice, memRequirements.memoryTypeBits, properties);

    result = vkAllocateMemory(device, &allocInfo, nullptr, imageMemory);
    ASSERT_VULKAN(result)

    result = vkBindImageMemory(device, image, *imageMemory, 0);
    ASSERT_VULKAN(result)
}