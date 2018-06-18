#pragma once

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

#include "jojo_scene.hpp"

namespace Textures {

struct Texture {
    VkImage        image;
    VmaAllocation  memory;
    VkSampler      sampler;
    VkImageView    view;
};

void fontTexture (
    const VmaAllocator              allocator,
    const VkDevice                  device,
    const VkCommandPool             commandPool,
    const VkQueue                   queue,
    Texture                        *outTexture
);

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
);

void cmdTextureArrayFromList (
    const VmaAllocator              allocator,
    const VkDevice                  device,
    const VkCommandBuffer           transferCmd,
    const std::vector<std::string> &textureList,
    uint32_t                        width,
    uint32_t                        height,
    bool                            generateDummyTexture,
    Texture                        *outTexture,
    VkBuffer                       *stagingBuffer,
    VmaAllocation                  *stagingMemory
);

VkDescriptorImageInfo descriptor (
    const Texture                  *texture
);

void freeTexture (
    const VmaAllocator              allocator,
    const VkDevice                  device,
    const Texture                  *texture
);

}