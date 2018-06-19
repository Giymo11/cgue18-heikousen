#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include "jojo_vulkan_attachments.hpp"
#include "jojo_vulkan_utils.hpp"

namespace Attachments {

void create (
    const VkDevice              device,
    const VmaAllocator          allocator,
    const VkFormat              format,
    const uint32_t              width,
    const uint32_t              height,
    const VkImageUsageFlagBits  usage,
    VmaAllocation              *imageMemory,
    VkImage                    *image,
    VkImageView                *imageView,
) {
    VkImageAspectFlags aspectMask = 0;
    VkImageLayout      layout;

    if (usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) {
        aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    }
    if (usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) {
        aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
        layout     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    }

    VkImageCreateInfo iinfo = {};
    iinfo.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    iinfo.imageType     = VK_IMAGE_TYPE_2D;
    iinfo.format        = format;
    iinfo.extent.width  = width;
    iinfo.extent.height = height;
    iinfo.extent.depth  = 1;
    iinfo.mipLevels     = 1;
    iinfo.arrayLayers   = 1;
    iinfo.samples       = VK_SAMPLE_COUNT_1_BIT;
    iinfo.tiling        = VK_IMAGE_TILING_OPTIMAL;
    iinfo.usage         = usage | VK_IMAGE_USAGE_SAMPLED_BIT;
    iinfo.initialLayout = layout;

    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage         = VMA_MEMORY_USAGE_GPU_ONLY;
    allocInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    ASSERT_VULKAN (vmaCreateImage (
        allocator, &iinfo,
        &allocInfo, image,
        imageMemory, nullptr
    ));

    VkImageViewCreateInfo ivinfo = {};
    ivinfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    ivinfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    ivinfo.format = format;
    ivinfo.image = *image;
    ivinfo.subresourceRange = {};
    ivinfo.subresourceRange.aspectMask = XXX;
    ivinfo.subresourceRange.baseMipLevel = 0;
    ivinfo.subresourceRange.levelCount = 1;
    ivinfo.subresourceRange.baseArrayLayer = 0;
    ivinfo.subresourceRange.layerCount = 1;

    ASSERT_VULKAN (vkCreateImageView (
        device, &ivinfo,
        nullptr, imageView
    ));
}

}