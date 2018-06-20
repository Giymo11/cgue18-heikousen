#pragma once
#include <vector>
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

namespace Pass {

struct Attachment {
    VkImage       image;
    VkImageView   imageView;
    VmaAllocation memory;

    VkFormat      format;
};

struct RenderPass {
    VkRenderPass            pass;
    VkFramebuffer           fb;
    VkSampler               sampler;
    std::vector<Attachment> attachments;
};

struct PassStorage {
    VkFormat                     depthFormat;

    RenderPass                   deferredPass;
    std::vector<VkCommandBuffer> deferredCmd;
    VkSemaphore                  deferredSema;
};

void allocPassStorage (
    const VkDevice       device,
    const VkCommandPool  commandPool,
    uint32_t             numCmdBuffers,
    PassStorage         *passes
);

void freePassStorage (
    const VkDevice       device,
    const VkCommandPool  commandPool,
    PassStorage         *passes
);

void allocPasses (
    const VkDevice      device,
    const VmaAllocator  allocator,
    uint32_t            width,
    uint32_t            height,
    PassStorage        *passes
);

void freePasses (
    const VkDevice      device,
    const VmaAllocator  allocator,
    PassStorage        *passes
);

}