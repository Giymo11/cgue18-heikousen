#include <array>
#include <stb_image.h>
#include "jojo_vulkan_utils.hpp"
#include "jojo_vulkan_textures.hpp"

/*
    TODO: Resources to deal with:
    * Staging Buffer
    * Staging Memory
    * Image
    * Device Memory
    * Sampler
    * ImageView
*/

static void setImageLayout (
    VkCommandBuffer cmd,
    VkImage image,
    VkImageAspectFlags aspectMask,
    VkImageLayout oldLayout,
    VkImageLayout newLayout,
    VkImageSubresourceRange range
) {
    VkImageMemoryBarrier b = {};
    b.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    b.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    b.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    b.oldLayout = oldLayout;
    b.newLayout = newLayout;
    b.image = image;
    b.subresourceRange = range;

    VkPipelineStageFlags src = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    VkPipelineStageFlags dst = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

    switch (oldLayout) {
    case VK_IMAGE_LAYOUT_UNDEFINED:
        b.srcAccessMask = 0;
        break;
    case VK_IMAGE_LAYOUT_PREINITIALIZED:
        b.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
        break;
    case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
        b.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        src = VK_PIPELINE_STAGE_TRANSFER_BIT;
        break;
    }

    switch (newLayout) {
    case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
        b.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        break;
    case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
        b.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        dst = VK_PIPELINE_STAGE_TRANSFER_BIT;
        break;
    case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
        b.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        dst = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        break;
    }

    vkCmdPipelineBarrier (
        cmd,
        src,
        dst,
        0,
        0, nullptr,
        0, nullptr,
        1, &b);
}

namespace Textures {

void create (
    VkDevice device,
    VkPhysicalDeviceMemoryProperties memoryProperties,
    VkDeviceSize size,
    VkFormat format,
    uint32_t layers,
    uint32_t mipLevels,
    uint32_t width,
    uint32_t height,
    VkBuffer *stagingBuffer,
    VkDeviceMemory *stagingMem,
    VkImage *image,
    VkDeviceMemory *imageMemory
) {
    VkMemoryAllocateInfo memAllocInfo = {};
    VkMemoryRequirements memReqs = {};

    VkBufferCreateInfo bufferCreateInfo = {};
    bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferCreateInfo.size = size * layers;
    bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    ASSERT_VULKAN (vkCreateBuffer (device, &bufferCreateInfo, nullptr, stagingBuffer));

    vkGetBufferMemoryRequirements (device, *stagingBuffer, &memReqs);
    memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memAllocInfo.allocationSize = memReqs.size;
    memAllocInfo.memoryTypeIndex = findMemoryTypeIndex (
        memoryProperties,
        memReqs.memoryTypeBits,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
    );
    ASSERT_VULKAN (vkAllocateMemory (device, &memAllocInfo, nullptr, stagingMem));
    ASSERT_VULKAN (vkBindBufferMemory (device, *stagingBuffer, *stagingMem, 0));

    VkImageCreateInfo imageInfo = {};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.format = format;
    imageInfo.mipLevels = mipLevels;
    imageInfo.arrayLayers = layers;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.extent = { width, height, 1 };
    imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    ASSERT_VULKAN (vkCreateImage (device, &imageInfo, nullptr, image));

    vkGetImageMemoryRequirements (device, *image, &memReqs);
    memAllocInfo.allocationSize = memReqs.size;
    memAllocInfo.memoryTypeIndex = findMemoryTypeIndex (
        memoryProperties,
        memReqs.memoryTypeBits,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
    );
    ASSERT_VULKAN (vkAllocateMemory (device, &memAllocInfo, nullptr, imageMemory));
    ASSERT_VULKAN (vkBindImageMemory (device, *image, *imageMemory, 0));
}

void stage (
    VkDevice device,
    VkDeviceMemory stagingMem,
    const uint8_t *imageData,
    VkDeviceSize imageDataSize
) {
    uint8_t *data;
    ASSERT_VULKAN (vkMapMemory (device, stagingMem, 0, imageDataSize, 0, (void **)&data));
    std::copy (imageData, imageData + imageDataSize, data);
    vkUnmapMemory (device, stagingMem);
}

void transfer (
    VkCommandBuffer cmd,
    VkBuffer staging,
    VkImage image,
    const VkBufferImageCopy *copy,
    uint32_t layers,
    uint32_t textureWidth,
    uint32_t textureHeight,
    uint32_t mipLevels
) {
    VkImageSubresourceRange subresourceRange = {};
    subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    subresourceRange.baseMipLevel = 0;
    subresourceRange.levelCount = mipLevels;
    subresourceRange.layerCount = layers;

    setImageLayout (
        cmd,
        image,
        VK_IMAGE_ASPECT_COLOR_BIT,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        subresourceRange);

    vkCmdCopyBufferToImage (
        cmd,
        staging,
        image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        layers,
        copy
    );

    VkImageMemoryBarrier barrier = {};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.image = image;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = layers;
    barrier.subresourceRange.levelCount = 1;

    int32_t mipWidth = static_cast<int32_t>(textureWidth);
    int32_t mipHeight = static_cast<int32_t>(textureHeight);

    for (uint32_t i = 1; i < mipLevels; i++) {
        barrier.subresourceRange.baseMipLevel = i - 1;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

        vkCmdPipelineBarrier (
            cmd,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
            0, nullptr,
            0, nullptr,
            1, &barrier
        );

        VkImageBlit blit = {};
        blit.srcOffsets[0] = { 0, 0, 0 };
        blit.srcOffsets[1] = { mipWidth, mipHeight, 1 };
        blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.srcSubresource.mipLevel = i - 1;
        blit.srcSubresource.baseArrayLayer = 0;
        blit.srcSubresource.layerCount = layers;
        blit.dstOffsets[0] = { 0, 0, 0 };
        blit.dstOffsets[1] = { mipWidth / 2, mipHeight / 2, 1 };
        blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.dstSubresource.mipLevel = i;
        blit.dstSubresource.baseArrayLayer = 0;
        blit.dstSubresource.layerCount = layers;

        vkCmdBlitImage (
            cmd,
            image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1, &blit,
            VK_FILTER_LINEAR
        );

        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier (
            cmd,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
            0, nullptr,
            0, nullptr,
            1, &barrier
        );

        if (mipWidth > 1) mipWidth /= 2;
        if (mipHeight > 1) mipHeight /= 2;
    }

    barrier.subresourceRange.baseMipLevel = mipLevels - 1;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier (
        cmd,
        VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
        0, nullptr,
        0, nullptr,
        1, &barrier
    );
}

VkSampler sampler (
    VkDevice device,
    uint32_t mipLevels
) {
    VkSampler sampler;
    VkSamplerCreateInfo s = {};
    s.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    /*
    TODO:
    * Anisotropic filtering
    */
    s.magFilter = VK_FILTER_LINEAR;
    s.minFilter = VK_FILTER_LINEAR;
    s.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    s.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    s.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    s.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    s.mipLodBias = 0.0f;
    s.compareEnable = VK_FALSE;
    s.compareOp = VK_COMPARE_OP_NEVER;
    s.minLod = 0.0f;
    s.maxLod = mipLevels > 1 ? static_cast<float>(mipLevels) : 0.0f;
    s.maxAnisotropy = 1.0;
    s.anisotropyEnable = VK_FALSE;
    s.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
    ASSERT_VULKAN (vkCreateSampler (device, &s, nullptr, &sampler));

    return sampler;
}

VkImageView view (
    VkDevice device,
    VkImage image,
    VkFormat format,
    uint32_t layers,
    uint32_t mipLevels
) {
    VkImageView view;
    VkImageViewCreateInfo v = {};
    v.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    v.image = image;
    v.viewType = layers > 1 ? VK_IMAGE_VIEW_TYPE_2D_ARRAY : VK_IMAGE_VIEW_TYPE_2D;
    v.format = format;
    v.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
    v.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    v.subresourceRange.baseMipLevel = 0;
    v.subresourceRange.baseArrayLayer = 0;
    v.subresourceRange.layerCount = layers;
    v.subresourceRange.levelCount = mipLevels;
    ASSERT_VULKAN (vkCreateImageView (device, &v, nullptr, &view));
    return view;
}

VkDescriptorImageInfo generateTextureArray (
    VkDevice device,
    const VkPhysicalDeviceMemoryProperties &memoryProperties,
    VkCommandPool commandPool,
    VkQueue queue
) {
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingMem;
    VkImage image;
    VkDeviceMemory imageMem;
    VkDescriptorImageInfo texture;

    const uint32_t mipLevels = 9;
    const uint32_t layers = 3;
    const uint32_t width = 256;
    const uint32_t height = 256;
    const uint32_t channels = 4;

    std::array<uint32_t, width * height * layers> texData;
    std::array<VkBufferImageCopy, layers> copy;

    // First color
    auto tex_ptr = texData.data ();
    for (int i = 0; i < 256; i++, tex_ptr += 256) {
        for (int j = 0; j < 256; j++) {
            tex_ptr[j] = -((i ^ j) >> 5 & 1) | 0xFFC4C4C4;
        }
    }

    // Second color
    for (int i = 0; i < 256; i++, tex_ptr += 256) {
        for (int j = 0; j < 256; j++) {
            tex_ptr[j] = -((i ^ j) >> 5 & 1) | 0xFF00FF00;
        }
    }

    // Third color
    for (int i = 0; i < 256; i++, tex_ptr += 256) {
        for (int j = 0; j < 256; j++) {
            tex_ptr[j] = -((i ^ j) >> 5 & 1) | 0xFF0000FF;
        }
    }

    create (
        device,
        memoryProperties,
        width * height * channels,
        VK_FORMAT_R8G8B8A8_SRGB,
        layers,
        mipLevels,
        width,
        height,
        &stagingBuffer,
        &stagingMem,
        &image,
        &imageMem
    );

    stage (
        device,
        stagingMem,
        (uint8_t *)texData.data (),
        width * height * channels * layers
    );

    for (uint32_t layer = 0, offset = 0; layer < layers; layer++) {
        auto &region = copy[layer];
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = layer;
        region.imageSubresource.layerCount = 1;
        region.imageExtent.width = width;
        region.imageExtent.height = height;
        region.imageExtent.depth = 1;
        region.bufferOffset = offset;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;

        offset += width * height * channels;
    }

    VkCommandBuffer commandBuffer;
    allocateAndBeginSingleUseBuffer (device, commandPool, &commandBuffer);
    transfer (
        commandBuffer,
        stagingBuffer,
        image,
        copy.data(),
        layers,
        256, 256,
        mipLevels
    );
    endAndSubmitCommandBuffer (device, commandPool, queue, commandBuffer);

    auto s = sampler (
        device,
        mipLevels
    );

    auto v = view (
        device,
        image,
        VK_FORMAT_R8G8B8A8_SRGB,
        mipLevels,
        1
    );

    texture.sampler = s;
    texture.imageView = v;
    texture.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    return texture;
}

VkDescriptorImageInfo fontTexture (
    VkDevice device,
    const VkPhysicalDeviceMemoryProperties &memoryProperties,
    VkCommandPool commandPool,
    VkQueue queue
) {
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingMem;
    VkImage image;
    VkDeviceMemory imageMem;
    VkDescriptorImageInfo texture;

    const uint32_t mipLevels = 1;
    stbi_uc *texData = nullptr;
    uint32_t width = 0;
    uint32_t height = 0;
    VkDeviceSize size = 0;

    {
        int texWidth, texHeight, texChannels;
        texData = stbi_load ("fonts/font.png", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

        width = (uint32_t)texWidth;
        height = (uint32_t)texHeight;
        size = width * height * 4;
    }

    create (
        device,
        memoryProperties,
        size,
        VK_FORMAT_R8G8B8A8_UNORM,
        1,
        mipLevels,
        width,
        height,
        &stagingBuffer,
        &stagingMem,
        &image,
        &imageMem
    );

    stage (
        device,
        stagingMem,
        (uint8_t *)texData,
        size
    );

    stbi_image_free (texData);

    VkBufferImageCopy copy = {};
    copy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    copy.imageSubresource.mipLevel = 0;
    copy.imageSubresource.baseArrayLayer = 0;
    copy.imageSubresource.layerCount = 1;
    copy.imageExtent.width = width;
    copy.imageExtent.height = height;
    copy.imageExtent.depth = 1;
    copy.bufferOffset = 0;
    copy.bufferRowLength = 0;
    copy.bufferImageHeight = 0;

    VkCommandBuffer commandBuffer;
    allocateAndBeginSingleUseBuffer (device, commandPool, &commandBuffer);
    transfer (
        commandBuffer,
        stagingBuffer,
        image,
        &copy,
        1,
        width, height,
        mipLevels
    );
    endAndSubmitCommandBuffer (device, commandPool, queue, commandBuffer);

    auto s = sampler (
        device,
        mipLevels
    );

    auto v = view (
        device,
        image,
        VK_FORMAT_R8G8B8A8_UNORM,
        1,
        mipLevels
    );

    texture.sampler = s;
    texture.imageView = v;
    texture.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    return texture;
}


}