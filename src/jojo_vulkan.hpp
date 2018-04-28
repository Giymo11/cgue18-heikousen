//
// Created by giymo11 on 3/11/18.
//

#pragma once


#include <vector>
#include <vulkan/vulkan.h>


// TODO: encapsulate most of this in a class with VkDevice, VkSurfaceKHR etc inside
// TODO: encapsulate height, width and maybe chosenImageFormat, chosenPhysicalDevice, etc in a class

VkResult createShaderModule(const VkDevice device, const std::vector<char> &code, VkShaderModule *shaderModule);

VkResult createInstance(VkInstance *instance, std::vector<const char *> &usedExtensions);

VkResult createLogicalDevice(const VkPhysicalDevice chosenDevice,
                             VkDevice *device,
                             const uint32_t chosenQueueFamilyIndex);

VkResult checkSurfaceSupport(const VkPhysicalDevice chosenDevice, const VkSurfaceKHR surface,
                             const uint32_t chosenQueueFamilyIndex);


VkResult createSwapchain(const VkDevice device, const VkSurfaceKHR surface, const VkSwapchainKHR oldSwapchain,
                         VkSwapchainKHR *swapchain,
                         const VkFormat chosenImageFormat, uint32_t width, uint32_t height);


VkResult createImageView(const VkDevice device, const VkImage swapchainImage,
                         VkImageView *imageView,
                         const VkFormat chosenImageFormat, const VkImageAspectFlags aspectFlags);

VkResult
createShaderStageCreateInfo(const VkDevice device, const std::string &filename,
                            const VkShaderStageFlagBits &shaderStage,
                            VkPipelineShaderStageCreateInfo *shaderStageCreateInfo, VkShaderModule *shaderModule);


VkResult createRenderpass(const VkDevice device, VkRenderPass *renderPass, const VkFormat chosenImageFormat,
                          const VkFormat chosenDepthFormat);


VkResult createDescriptorSetLayout(const VkDevice device, VkDescriptorSetLayout *descriptorSetLayout);

VkResult createPipelineLayout(const VkDevice device, const VkDescriptorSetLayout *descriptorSetLayout,
                              VkPipelineLayout *pipelineLayout);

VkResult createPipeline(const VkDevice device, const VkPipelineShaderStageCreateInfo *shaderStages,
                        const VkRenderPass renderPass, const VkPipelineLayout pipelineLayout,
                        VkPipeline *pipeline,
                        const int width, const int height);


VkResult createFramebuffer(const VkDevice device, const VkRenderPass renderPass,
                           const VkImageView swapChainImageView, const VkImageView depthImageView,
                           VkFramebuffer *framebuffer,
                           uint32_t width, uint32_t height);


VkResult createCommandPool(const VkDevice device, VkCommandPool *commandPool, const uint32_t chosenQueueFamilyIndex);


VkResult allocateCommandBuffers(const VkDevice device, const VkCommandPool commandPool,
                                std::vector<VkCommandBuffer> &commandBuffers,
                                const uint32_t numberOfImagesInSwapchain);

VkResult beginCommandBuffer(const VkCommandBuffer commandBuffer);

VkResult createSemaphore(const VkDevice device, VkSemaphore *semaphore);

VkResult createDescriptorPool(const VkDevice device, VkDescriptorPool *descriptorPool, uint32_t descriptorCount);

VkResult allocateDescriptorSet(const VkDevice device, const VkDescriptorPool descriptorPool,
                               const VkDescriptorSetLayout descriptorSetLayout,
                               VkDescriptorSet *descriptorSet);


