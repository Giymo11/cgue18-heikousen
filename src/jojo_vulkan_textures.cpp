#include <array>
#include <stb_image.h>

#include "jojo_vulkan_utils.hpp"
#include "jojo_vulkan_textures.hpp"
#include "jojo_engine.hpp"

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

static void create (
    const VmaAllocator  allocator,
    const VkDeviceSize  size,
    const VkFormat      format,
    const uint32_t      layers,
    const uint32_t      mipLevels,
    const uint32_t      width,
    const uint32_t      height,
    VkBuffer           *stagingBuffer,
    VmaAllocation      *stagingMem,
    VkImage            *image,
    VmaAllocation      *imageMemory
) {
    VkMemoryAllocateInfo memAllocInfo = {};
    VkMemoryRequirements memReqs = {};

    VkBufferCreateInfo bufferCreateInfo = {};
    bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferCreateInfo.size = size * layers;
    bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
    allocInfo.requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
    allocInfo.preferredFlags = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    ASSERT_VULKAN (vmaCreateBuffer (
        allocator, &bufferCreateInfo, &allocInfo,
        stagingBuffer, stagingMem, nullptr
    ));

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

    allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    allocInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    ASSERT_VULKAN (vmaCreateImage (
        allocator, &imageInfo, &allocInfo,
        image, imageMemory, nullptr
    ));
}

static void stage (
    const VmaAllocator   allocator,
    const VmaAllocation  stagingMem,
    const uint8_t       *imageData,
    const VkDeviceSize   imageDataSize
) {
    uint8_t *data;

    ASSERT_VULKAN (vmaMapMemory (allocator, stagingMem, (void **)&data));
    std::copy (imageData, imageData + imageDataSize, data);
    vmaUnmapMemory (allocator, stagingMem);
}

static void transfer (
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

static VkSampler sampler (
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

static VkImageView view (
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

void fontTexture (
    const VmaAllocator  allocator,
    const VkDevice      device,
    const VkCommandPool commandPool,
    const VkQueue       queue,
    Texture            *outTexture
) {
    VkBuffer              stagingBuffer;
    VmaAllocation         stagingMem;

    const uint32_t        mipLevels = 1;
    stbi_uc              *texData   = nullptr;
    uint32_t              width     = 0;
    uint32_t              height    = 0;
    VkDeviceSize          size      = 0;

    {
        int texWidth, texHeight, texChannels;
        texData = stbi_load ("fonts/font.png", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

        width = (uint32_t)texWidth;
        height = (uint32_t)texHeight;
        size = width * height * 4;
    }

    create (
        allocator,
        size,
        VK_FORMAT_R8G8B8A8_UNORM,
        1,
        mipLevels,
        width,
        height,
        &stagingBuffer,
        &stagingMem,
        &outTexture->image,
        &outTexture->memory
    );

    stage (
        allocator,
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
        outTexture->image,
        &copy,
        1,
        width, height,
        mipLevels
    );
    endAndSubmitCommandBuffer (device, commandPool, queue, commandBuffer);

    outTexture->sampler = sampler (
        device,
        mipLevels
    );

    outTexture->view = view (
        device,
        outTexture->image,
        VK_FORMAT_R8G8B8A8_UNORM,
        1,
        mipLevels
    );

    vmaDestroyBuffer (allocator, stagingBuffer, stagingMem);
}

void cmdTextureArrayFromData (
    const VmaAllocator              allocator,
    const VkDevice                  device,
    const VkCommandBuffer           transferCmd,
    const std::vector<
        Scene::TextureData
    >                              &texData,
    uint32_t                        numTextures,
    uint32_t                        width,
    uint32_t                        height,
    Texture                        *outTexture,
    VkBuffer                       *stagingBuffer,
    VmaAllocation                  *stagingMemory
) {
    const uint32_t mipLevels = 10;
    const uint32_t channels = 4;
    const uint32_t dataOffset = width * height * channels;
    const uint32_t layers = numTextures;
    std::vector<VkBufferImageCopy> copy (layers);

    create (
        allocator,
        dataOffset,
        VK_FORMAT_R8G8B8A8_UNORM,
        layers,
        mipLevels,
        width,
        height,
        stagingBuffer,
        stagingMemory,
        &outTexture->image,
        &outTexture->memory
    );

    stage (
        allocator,
        *stagingMemory,
        texData[0].data(),
        dataOffset * layers
    );

    for (uint32_t layer = 0, offset = 0; layer < layers; layer++) {
        auto &region = copy[layer];
        region = {};
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

        offset += dataOffset;
    }

    transfer (
        transferCmd,
        *stagingBuffer,
        outTexture->image,
        copy.data (),
        layers,
        width, height,
        mipLevels
    );

    outTexture->sampler = sampler (
        device,
        mipLevels
    );

    outTexture->view = view (
        device,
        outTexture->image,
        VK_FORMAT_R8G8B8A8_UNORM,
        layers,
        mipLevels
    );
}

void cmdTextureArrayFromList (
    const VmaAllocator              allocator,
    const VkDevice                  device,
    const VkCommandBuffer           transferCmd,
    const std::vector<std::string> &textureList,
    const uint32_t                  width,
    const uint32_t                  height,
    const bool                      generateDummyTexture,
    Texture                        *outTexture,
    VkBuffer                       *stagingBuffer,
    VmaAllocation                  *stagingMemory
) {
    const uint32_t mipLevels  = 10;
    const uint32_t channels   = 4;
    const uint32_t dataOffset = width * height * channels;
    const uint32_t layers     = (uint32_t)textureList.size () + 1;

    std::vector<VkBufferImageCopy> copy(layers);
    std::vector<uint8_t> texData (dataOffset * layers);
    uint32_t counter = 1;

    // Use dummy texture as zero-th texture
    auto tex_ptr = (uint32_t *)texData.data ();
    if (generateDummyTexture) {
        for (int i = 0; i < (int)height; i++, tex_ptr += width) {
            for (int j = 0; j < (int)width; j++) {
                tex_ptr[j] = -((i ^ j) >> 5 & 1) | 0xFFFF00FF;
            }
        }
    } else {
        for (int i = 0; i < (int)height; i++, tex_ptr += width) {
            for (int j = 0; j < (int)height; j++) {
                tex_ptr[j] = 0xFF000000;
            }
        }
    }

    for (const auto &name : textureList) {
        int dummy;

        auto full_name = std::string ("textures/") + name;
        auto tex = stbi_load (full_name.c_str (), &dummy, &dummy, &dummy, STBI_rgb_alpha);
        std::copy ((uint8_t *)tex, (uint8_t *)tex + dataOffset, texData.data () + dataOffset * counter);
        stbi_image_free (tex);

        counter += 1;
    }

    create (
        allocator,
        dataOffset,
        VK_FORMAT_R8G8B8A8_UNORM,
        layers,
        mipLevels,
        width,
        height,
        stagingBuffer,
        stagingMemory,
        &outTexture->image,
        &outTexture->memory
    );

    stage (
        allocator,
        *stagingMemory,
        texData.data (),
        dataOffset * layers
    );

    for (uint32_t layer = 0, offset = 0; layer < layers; layer++) {
        auto &region = copy[layer];
        region = {};
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

        offset += dataOffset;
    }

    transfer (
        transferCmd,
        *stagingBuffer,
        outTexture->image,
        copy.data (),
        layers,
        width, height,
        mipLevels
    );

    outTexture->sampler = sampler (
        device,
        mipLevels
    );

    outTexture->view = view (
        device,
        outTexture->image,
        VK_FORMAT_R8G8B8A8_UNORM,
        layers,
        mipLevels
    );
}

VkDescriptorImageInfo descriptor (
    const Texture                  *texture
) {
    VkDescriptorImageInfo info;
    info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    info.imageView   = texture->view;
    info.sampler     = texture->sampler;
    return info;
}

void freeTexture (
    const VmaAllocator              allocator,
    const VkDevice                  device,
    const Texture                  *texture
) {
    vkDestroySampler (device, texture->sampler, nullptr);
    vkDestroyImageView (device, texture->view, nullptr);
    vmaDestroyImage (allocator, texture->image, texture->memory);
}

}