//
// Created by benja on 4/28/2018.
//


#include <algorithm>
#include <iostream>
#include "jojo_engine_helper.hpp"
#include "jojo_vulkan.hpp"
#include "jojo_vulkan_utils.hpp"


void JojoSwapchain::createSwapchainAndChildren(Config &config, JojoEngine *engine) {
    VkResult result;
    auto chosenImageFormat = VK_FORMAT_B8G8R8A8_UNORM;   // TODO: check if valid via surfaceFormats[i].format

    result = createSwapchain(engine->device, engine->surface, swapchain, &swapchain, chosenImageFormat, config.width, config.height);
    ASSERT_VULKAN(result)


    uint32_t numberOfImagesInSwapchain = 0;
    result = vkGetSwapchainImagesKHR(engine->device, swapchain, &numberOfImagesInSwapchain, nullptr);
    ASSERT_VULKAN(result)

    // TODO: actually adjust the number of command buffers
    if (numberOfImagesInSwapchain > numberOfCommandBuffers)
        throw std::runtime_error("we didnt plan for this... (more than three swapchain images)");

    // doesnt have to be freed because they are allocated by the swapchain -> error when trying to destroy
    std::vector<VkImage> swapchainImages;
    swapchainImages.resize(numberOfImagesInSwapchain);
    result = vkGetSwapchainImagesKHR(engine->device, swapchain, &numberOfImagesInSwapchain, swapchainImages.data());
    ASSERT_VULKAN(result)

    imageViews.resize(numberOfImagesInSwapchain);
    for (size_t i = 0; i < numberOfImagesInSwapchain; ++i) {
        result = createImageView(engine->device, swapchainImages[i], &(imageViews[i]), chosenImageFormat,
                                 VK_IMAGE_ASPECT_COLOR_BIT);
        ASSERT_VULKAN(result)
    }

    VkFormat depthFormat = findSupportedFormat(engine->chosenDevice,
                                               {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT,
                                                VK_FORMAT_D24_UNORM_S8_UINT},
                                               VK_IMAGE_TILING_OPTIMAL,
                                               VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
    );
    createImage(engine->device, engine->chosenDevice, config.width, config.height, depthFormat, VK_IMAGE_TILING_OPTIMAL,
                VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, depthImage,
                &depthImageMemory);
    ASSERT_VULKAN(result)
    result = createImageView(engine->device, depthImage, &depthImageView, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);
    ASSERT_VULKAN(result)

    changeImageLayout(engine->device, engine->commandPool, engine->queue, depthImage, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED,
                      VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);


    result = createRenderpass(engine->device, &swapchainRenderPass, chosenImageFormat, depthFormat);
    ASSERT_VULKAN(result)


    framebuffers.resize(numberOfImagesInSwapchain);
    for (size_t i = 0; i < numberOfImagesInSwapchain; ++i) {
        result = createFramebuffer(engine->device, swapchainRenderPass, imageViews[i], depthImageView, &(framebuffers[i]), config.width,
                                   config.height);
        ASSERT_VULKAN(result)
    }
}

void JojoSwapchain::createCommandBuffers(JojoEngine *engine)  {
    VkResult result;
    result = allocateCommandBuffers(engine->device, engine->commandPool, commandBuffers, numberOfCommandBuffers);
    ASSERT_VULKAN(result)


    VkFenceCreateInfo fenceCreateInfo = {};
    fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    commandBufferFences.resize(numberOfCommandBuffers);
    for (auto &fence : commandBufferFences) {
        result = vkCreateFence(engine->device, &fenceCreateInfo, nullptr, &fence);
        ASSERT_VULKAN(result)
    }

    result = createSemaphore(engine->device, &semaphoreImageAvailable);
    ASSERT_VULKAN(result)
    result = createSemaphore(engine->device, &semaphoreRenderingDone);
    ASSERT_VULKAN(result)
}

void JojoSwapchain::destroySwapchainChildren(JojoEngine *engine) {
    for (auto &framebuffer : framebuffers) {
        vkDestroyFramebuffer(engine->device, framebuffer, nullptr);
    }

    vkDestroyImageView(engine->device, depthImageView, nullptr);
    vkDestroyImage(engine->device, depthImage, nullptr);
    vkFreeMemory(engine->device, depthImageMemory, nullptr);

    vkDestroyRenderPass(engine->device, swapchainRenderPass, nullptr);

    for (auto &imageView : imageViews) {
        vkDestroyImageView(engine->device, imageView, nullptr);
    }
}

void JojoSwapchain::recreateSwapchain(Config &config, JojoEngine *engine, JojoWindow *window)  {
    VkResult result;

    result = vkDeviceWaitIdle(engine->device);
    ASSERT_VULKAN(result)

    VkSurfaceCapabilitiesKHR surfaceCapabilitiesKHR;
    result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(engine->chosenDevice, engine->surface, &surfaceCapabilitiesKHR);
    ASSERT_VULKAN(result)

    int newWidth, newHeight;
    glfwGetWindowSize(window->window, &newWidth, &newHeight);

    config.width = (uint32_t) std::min(newWidth, (int) surfaceCapabilitiesKHR.maxImageExtent.width);;
    config.height = (uint32_t) std::min(newHeight, (int) surfaceCapabilitiesKHR.maxImageExtent.height);

    destroySwapchainChildren(engine);

    VkSwapchainKHR oldSwapchain = swapchain;

    createSwapchainAndChildren(config, engine);

    vkDestroySwapchainKHR(engine->device, oldSwapchain, nullptr);
}


void JojoSwapchain::destroyCommandBuffers(JojoEngine *engine) {
    for (auto &fence : commandBufferFences) {
        vkDestroyFence(engine->device, fence, nullptr);
    }

    vkDestroySemaphore(engine->device, semaphoreImageAvailable, nullptr);
    vkDestroySemaphore(engine->device, semaphoreRenderingDone, nullptr);

    vkFreeCommandBuffers(engine->device, engine->commandPool, static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data());

}