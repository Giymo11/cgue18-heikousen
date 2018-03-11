#include <iostream>
#include <cstring>
#include <vector>
#include <algorithm>
#include <chrono>

#define GLFW_INCLUDE_VULKAN
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "jojo_vulkan_data.hpp"
#include "jojo_vulkan.hpp"
#include "jojo_vulkan_utils.hpp"
#include "jojo_vulkan_info.hpp"


VkInstance instance;
VkSurfaceKHR surface;
VkPhysicalDevice chosenDevice;
VkDevice device;
VkSwapchainKHR swapchain = VK_NULL_HANDLE;
VkQueue queue;

VkBuffer vertexBuffer;
VkDeviceMemory vertexBufferDeviceMemory;
VkBuffer indexBuffer;
VkDeviceMemory indexBufferDeviceMemory;
VkBuffer uniformBuffer;
VkDeviceMemory uniformBufferDeviceMemory;

std::vector<VkImageView> imageViews;
std::vector<VkFramebuffer> framebuffers;
std::vector<VkCommandBuffer> commandBuffers;

VkShaderModule shaderModuleVert;
VkShaderModule shaderModuleFrag;

VkPipelineLayout pipelineLayout;
VkRenderPass renderPass;
VkPipeline pipeline;
VkCommandPool commandPool;

VkSemaphore semaphoreImageAvailable;
VkSemaphore semaphoreRenderingDone;

VkDescriptorSetLayout descriptorSetLayout;
VkDescriptorPool descriptorPool;
VkDescriptorSet descriptorSet;

VkImage depthImage;
VkDeviceMemory depthImageMemory;
VkImageView depthImageView;


GLFWwindow *window;

// TODO: read from resource
uint32_t width = 1200;
uint32_t height = 720;

glm::mat4 mvp;


std::vector<Vertex> vertices = {
        Vertex({-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 1.0f}),
        Vertex({0.5f, 0.5f, 0.0f}, {1.0f, 1.0f, 0.0f}),
        Vertex({-0.5f, 0.5f, 0.0f}, {0.0f, 1.0f, 1.0f}),
        Vertex({0.5f, -0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}),

        Vertex({-0.5f, -0.5f, -1.0f}, {1.0f, 0.0f, 1.0f}),
        Vertex({0.5f, 0.5f, -1.0f}, {1.0f, 1.0f, 0.0f}),
        Vertex({-0.5f, 0.5f, -1.0f}, {0.0f, 1.0f, 1.0f}),
        Vertex({0.5f, -0.5f, -1.0f}, {1.0f, 1.0f, 1.0f})
};

std::vector<uint32_t> indices = {
        0, 1, 2,
        0, 3, 1,

        4, 5, 6,
        4, 7, 5,
};


void recordCommandBuffer(VkCommandBuffer commandBuffer, VkFramebuffer framebuffer) {
    std::array<VkClearValue, 2> clearValues = {};
    clearValues[0].color = {0.0f, 0.0f, 0.0f, 1.0f};
    clearValues[1].depthStencil = {1.0f, 0};

    VkRenderPassBeginInfo renderPassBeginInfo;
    renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassBeginInfo.pNext = nullptr;
    renderPassBeginInfo.renderPass = renderPass;
    renderPassBeginInfo.framebuffer = framebuffer;
    renderPassBeginInfo.renderArea.offset = {0, 0};
    renderPassBeginInfo.renderArea.extent = {width, height};
    renderPassBeginInfo.clearValueCount = clearValues.size();
    renderPassBeginInfo.pClearValues = clearValues.data();

    // _INLINE means to only use primary command buffers
    vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

    VkViewport viewport;
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = width;
    viewport.height = height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    VkRect2D scissor;
    scissor.offset = {0, 0};
    scissor.extent = {width, height};
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer, offsets);
    vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);

    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0,
                            nullptr);

    vkCmdDrawIndexed(commandBuffer, indices.size(), 1, 0, 0, 0);

    vkCmdEndRenderPass(commandBuffer);
}

void createAndUpdateDescriptorSet() {
    VkDescriptorBufferInfo descriptorBufferInfo;
    descriptorBufferInfo.buffer = uniformBuffer;
    descriptorBufferInfo.offset = 0;
    descriptorBufferInfo.range = sizeof(mvp);

    VkWriteDescriptorSet writeDescriptorSet;
    writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet.pNext = nullptr;
    writeDescriptorSet.dstSet = descriptorSet;
    writeDescriptorSet.dstBinding = 0;
    writeDescriptorSet.dstArrayElement = 0;
    writeDescriptorSet.descriptorCount = 1;
    writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    writeDescriptorSet.pImageInfo = nullptr;
    writeDescriptorSet.pBufferInfo = &descriptorBufferInfo;
    writeDescriptorSet.pTexelBufferView = nullptr;

    vkUpdateDescriptorSets(device, 1, &writeDescriptorSet, 0, nullptr);
}


void destroySwapchainChildren(bool preservePipeline = false) {
    vkFreeCommandBuffers(device, commandPool, commandBuffers.size(), commandBuffers.data());

    for (auto &framebuffer : framebuffers) {
        vkDestroyFramebuffer(device, framebuffer, nullptr);
    }

    if (!preservePipeline) {
        vkDestroyPipeline(device, pipeline, nullptr);
        vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
        vkDestroyShaderModule(device, shaderModuleVert, nullptr);
        vkDestroyShaderModule(device, shaderModuleFrag, nullptr);
    }

    vkDestroyRenderPass(device, renderPass, nullptr);

    for (auto &imageView : imageViews) {
        vkDestroyImageView(device, imageView, nullptr);
    }
}

void createSwapchainAndChildren(bool preservePipeline = false) {
    VkResult result;
    auto chosenImageFormat = VK_FORMAT_B8G8R8A8_UNORM;   // TODO: check if valid via surfaceFormats[i].format

    // TODO: use oldSwapchain!
    result = createSwapchain(device, surface, swapchain, &swapchain, chosenImageFormat, width, height);
    ASSERT_VULKAN(result)


    uint32_t numberOfImagesInSwapchain = 0;
    result = vkGetSwapchainImagesKHR(device, swapchain, &numberOfImagesInSwapchain, nullptr);
    ASSERT_VULKAN(result)

    std::vector<VkImage> swapchainImages;
    swapchainImages.resize(numberOfImagesInSwapchain);
    result = vkGetSwapchainImagesKHR(device, swapchain, &numberOfImagesInSwapchain, swapchainImages.data());
    ASSERT_VULKAN(result)

    imageViews.resize(numberOfImagesInSwapchain);
    for (size_t i = 0; i < numberOfImagesInSwapchain; ++i) {
        result = createImageView(device, swapchainImages[i], &(imageViews[i]), chosenImageFormat, VK_IMAGE_ASPECT_COLOR_BIT);
        ASSERT_VULKAN(result)
    }

    VkFormat depthFormat = findSupportedFormat(chosenDevice,
                                               {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT,
                                                VK_FORMAT_D24_UNORM_S8_UINT},
                                               VK_IMAGE_TILING_OPTIMAL,
                                               VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
    );
    createImage(device, chosenDevice, width, height, depthFormat, VK_IMAGE_TILING_OPTIMAL,
                VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, depthImage,
                depthImageMemory);
    ASSERT_VULKAN(result)
    result = createImageView(device, depthImage, &depthImageView, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);
    ASSERT_VULKAN(result)


    result = createRenderpass(device, &renderPass, chosenImageFormat, depthFormat);
    ASSERT_VULKAN(result)


    if (!preservePipeline) {
        VkPipelineShaderStageCreateInfo shaderStageCreateInfoVert;
        result = createShaderStageCreateInfo(device, "../shader/shader.vert.spv", VK_SHADER_STAGE_VERTEX_BIT,
                                             &shaderStageCreateInfoVert, &shaderModuleVert);
        ASSERT_VULKAN(result)

        VkPipelineShaderStageCreateInfo shaderStageCreateInfoFrag;
        result = createShaderStageCreateInfo(device, "../shader/shader.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT,
                                             &shaderStageCreateInfoFrag, &shaderModuleFrag);
        ASSERT_VULKAN(result)

        VkPipelineShaderStageCreateInfo shaderStages[] = {shaderStageCreateInfoVert, shaderStageCreateInfoFrag};

        result = createPipelineLayout(device, &descriptorSetLayout, &pipelineLayout);
        ASSERT_VULKAN(result)

        result = createPipeline(device, shaderStages, renderPass, pipelineLayout, &pipeline, width, height);
        ASSERT_VULKAN(result)
    }


    framebuffers.resize(numberOfImagesInSwapchain);
    for (size_t i = 0; i < numberOfImagesInSwapchain; ++i) {
        result = createFramebuffer(device, renderPass, imageViews[i], depthImageView, &(framebuffers[i]), width, height);
        ASSERT_VULKAN(result)
    }


    result = allocateCommandBuffers(device, commandPool, commandBuffers, numberOfImagesInSwapchain);
    ASSERT_VULKAN(result)

    for (size_t i = 0; i < numberOfImagesInSwapchain; ++i) {
        result = beginCommandBuffer(commandBuffers[i]);
        ASSERT_VULKAN(result)

        recordCommandBuffer(commandBuffers[i], framebuffers[i]);

        result = vkEndCommandBuffer(commandBuffers[i]);
        ASSERT_VULKAN(result)
    }
}

void recreateSwapchain() {
    VkResult result;

    result = vkDeviceWaitIdle(device);
    ASSERT_VULKAN(result)

    VkSurfaceCapabilitiesKHR surfaceCapabilitiesKHR;
    result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(chosenDevice, surface, &surfaceCapabilitiesKHR);
    ASSERT_VULKAN(result)

    int newWidth, newHeight;
    glfwGetWindowSize(window, &newWidth, &newHeight);

    width = std::min(newWidth, (int) surfaceCapabilitiesKHR.maxImageExtent.width);;
    height = std::min(newHeight, (int) surfaceCapabilitiesKHR.maxImageExtent.height);

    // TODO: don't recreate the pipeline, but use dynamic states

    destroySwapchainChildren(true);

    VkSwapchainKHR oldSwapchain = swapchain;

    createSwapchainAndChildren(true);

    vkDestroySwapchainKHR(device, oldSwapchain, nullptr);
}


void startVulkan() {
    VkResult result;

    result = createInstance(&instance);
    ASSERT_VULKAN(result)

    printInstanceLayers();
    printInstanceExtensions();

    result = glfwCreateWindowSurface(instance, window, nullptr, &surface);
    ASSERT_VULKAN(result)

    uint32_t numberOfPhysicalDevices = 0;
    // if passed nullptr as third parameter, outputs the number of GPUs to the second parameter
    result = vkEnumeratePhysicalDevices(instance, &numberOfPhysicalDevices, nullptr);
    ASSERT_VULKAN(result)

    std::vector<VkPhysicalDevice> physicalDevices;
    physicalDevices.resize(numberOfPhysicalDevices);
    // actually enumerates the GPUs for use
    result = vkEnumeratePhysicalDevices(instance, &numberOfPhysicalDevices, physicalDevices.data());
    ASSERT_VULKAN(result)

    std::cout << std::endl << "GPUs Found: " << numberOfPhysicalDevices << std::endl << std::endl;
    for (size_t i = 0; i < numberOfPhysicalDevices; ++i)
        printStats(physicalDevices[i], surface);

    chosenDevice = physicalDevices[0];     // TODO: choose right physical device
    auto chosenQueueFamilyIndex = 0;        // TODO: choose the best queue family

    result = createLogicalDevice(chosenDevice, &device, chosenQueueFamilyIndex);
    ASSERT_VULKAN(result)

    vkGetDeviceQueue(device, chosenQueueFamilyIndex, 0, &queue);

    result = checkSurfaceSupport(chosenDevice, surface, chosenQueueFamilyIndex);
    ASSERT_VULKAN(result)

    result = createCommandPool(device, &commandPool, chosenQueueFamilyIndex);
    ASSERT_VULKAN(result)


    createAndUploadBuffer(device, chosenDevice, commandPool, queue, vertices, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                          vertexBuffer, vertexBufferDeviceMemory);
    createAndUploadBuffer(device, chosenDevice, commandPool, queue, indices, VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                          indexBuffer, indexBufferDeviceMemory);

    VkDeviceSize bufferSize = sizeof(mvp);
    createBuffer(device, chosenDevice, bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, uniformBuffer,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, uniformBufferDeviceMemory);

    result = createDescriptorPool(device, &descriptorPool);
    ASSERT_VULKAN(result)

    result = createDescriptorSetLayout(device, &descriptorSetLayout);
    ASSERT_VULKAN(result)


    result = allocateDescriptorSet(device, descriptorPool, descriptorSetLayout, &descriptorSet);
    ASSERT_VULKAN(result)
    createAndUpdateDescriptorSet();

    createSwapchainAndChildren(false);


    result = createSemaphore(device, &semaphoreImageAvailable);
    ASSERT_VULKAN(result)
    result = createSemaphore(device, &semaphoreRenderingDone);
    ASSERT_VULKAN(result)
}


void onWindowResized(GLFWwindow *window, int newWidth, int newHeight) {
    if (newWidth > 0 && newHeight > 0) {
        //recreateSwapchain();
    }
}

void startGlfw() {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    // glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    window = glfwCreateWindow(width, height, "heikousen", nullptr, nullptr);
    glfwSetWindowSizeCallback(window, onWindowResized);
}


void drawFrame() {
    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(device, swapchain, std::numeric_limits<uint64_t>::max(),
                                            semaphoreImageAvailable, VK_NULL_HANDLE, &imageIndex);
    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        recreateSwapchain();
        return; // throw away this frame, because after recreating the swapchain, the vkAcquireNexImageKHR is
        // not signaling the semaphoreImageAvailable anymore
    }
    ASSERT_VULKAN(result)

    VkPipelineStageFlags waitStageMask[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

    VkSubmitInfo submitInfo;
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.pNext = nullptr;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &semaphoreImageAvailable;
    submitInfo.pWaitDstStageMask = waitStageMask;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &(commandBuffers[imageIndex]);
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &semaphoreRenderingDone;

    result = vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
    ASSERT_VULKAN(result)

    VkPresentInfoKHR presentInfo;
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.pNext = nullptr;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &semaphoreRenderingDone;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &swapchain;
    presentInfo.pImageIndices = &imageIndex;
    presentInfo.pResults = nullptr;

    result = vkQueuePresentKHR(queue, &presentInfo);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        recreateSwapchain();
        return;
        // same as with vkAcquireNextImageKHR
    }
    ASSERT_VULKAN(result)
}

auto gameStartTime = std::chrono::high_resolution_clock::now();

void updateMvp() {
    auto now = std::chrono::high_resolution_clock::now();
    float timeSinceStart = std::chrono::duration_cast<std::chrono::milliseconds>(now - gameStartTime).count() / 1000.0f;

    glm::mat4 model = glm::rotate(glm::mat4(), timeSinceStart * glm::radians(30.0f), glm::vec3(0, 0, 1));
    glm::mat4 view = glm::lookAt(glm::vec3(1.0f, 1.0f, 1.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    glm::mat4 projection = glm::perspective(glm::radians(60.0f), width / (float) height, 0.001f, 100.0f);
    // openGL has the z dir flipped
    projection[1][1] *= -1;

    mvp = projection * view * model;
    void *rawData;
    VkResult result = vkMapMemory(device, uniformBufferDeviceMemory, 0, sizeof(mvp), 0, &rawData);
    ASSERT_VULKAN(result)
    memcpy(rawData, &mvp, sizeof(mvp));
    vkUnmapMemory(device, uniformBufferDeviceMemory);
}


void gameloop() {
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        updateMvp();

        drawFrame();
    }
}


void shutdownVulkan() {
    // block until vulkan has finished
    VkResult result = vkDeviceWaitIdle(device);
    ASSERT_VULKAN(result)

    vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
    vkDestroyDescriptorPool(device, descriptorPool, nullptr);
    vkFreeMemory(device, uniformBufferDeviceMemory, nullptr);
    vkDestroyBuffer(device, uniformBuffer, nullptr);

    vkFreeMemory(device, indexBufferDeviceMemory, nullptr);
    vkDestroyBuffer(device, indexBuffer, nullptr);

    vkFreeMemory(device, vertexBufferDeviceMemory, nullptr);
    vkDestroyBuffer(device, vertexBuffer, nullptr);

    vkDestroySemaphore(device, semaphoreImageAvailable, nullptr);
    vkDestroySemaphore(device, semaphoreRenderingDone, nullptr);

    destroySwapchainChildren(false);
    vkDestroySwapchainKHR(device, swapchain, nullptr);
    vkDestroyCommandPool(device, commandPool, nullptr);
    vkDestroyDevice(device, nullptr);
    vkDestroySurfaceKHR(instance, surface, nullptr);
    vkDestroyInstance(instance, nullptr);
}

void shutdownGlfw() {
    glfwDestroyWindow(window);
    glfwTerminate();
}


int main() {
    std::cout << "Hello World!" << std::endl << std::endl;

    startGlfw();
    startVulkan();

    gameloop();

    shutdownVulkan();
    shutdownGlfw();

    return 0;
}

