

#pragma once

#include <vulkan/vulkan.h>

#include "jojo_engine.hpp"
#include "jojo_vulkan_data.hpp"
#include "jojo_utils.hpp"

class JojoSwapchain {
public:

    const uint32_t numberOfCommandBuffers = 3;

    // TODO: refactor out a Frame class
    std::vector<VkImageView> imageViews;
    std::vector<VkFramebuffer> framebuffers;
    std::vector<VkCommandBuffer> commandBuffers;
    std::vector<VkFence> commandBufferFences;

    VkSwapchainKHR swapchain = VK_NULL_HANDLE;

    VkSemaphore semaphoreImageAvailable;
    VkSemaphore semaphoreRenderingDone;

    VkImage depthImage;
    VkDeviceMemory depthImageMemory;
    VkImageView depthImageView;

    VkRenderPass swapchainRenderPass;


    void createCommandBuffers(JojoEngine *engine);

    void createSwapchainAndChildren(Config &config, JojoEngine *engine);

    void destroySwapchainChildren(JojoEngine *engine);

    void recreateSwapchain(Config &config, JojoEngine *engine, JojoWindow *window);

    void destroyCommandBuffers(JojoEngine *engine);
};