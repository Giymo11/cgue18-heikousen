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
#include <glm/gtc/type_ptr.hpp>

#include <btBulletDynamicsCommon.h>

#include "jojo_data.hpp"
#include "jojo_script.hpp"

#include "jojo_vulkan_data.hpp"
#include "jojo_vulkan.hpp"
#include "jojo_vulkan_utils.hpp"
#include "jojo_vulkan_info.hpp"
#include "jojo_physics.hpp"


VkInstance instance;
VkSurfaceKHR surface;
VkPhysicalDevice chosenDevice;
VkDevice device;
VkSwapchainKHR swapchain = VK_NULL_HANDLE;
VkQueue queue;

const int numberOfCommandBuffers = 3;

// TODO: refactor out a Frame class
std::vector<VkImageView> imageViews;
std::vector<VkFramebuffer> framebuffers;
std::vector<VkCommandBuffer> commandBuffers;
std::vector<VkFence> commandBufferFences;

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

VkImage depthImage;
VkDeviceMemory depthImageMemory;
VkImageView depthImageView;

GLFWwindow *window;


void recordCommandBuffer(Config &config, VkCommandBuffer commandBuffer, VkFramebuffer framebuffer,
                         std::vector<JojoMesh> &meshes) {
    std::array<VkClearValue, 2> clearValues = {};
    clearValues[0].color = {0.0f, 0.0f, 0.0f, 1.0f};
    clearValues[1].depthStencil = {1.0f, 0};

    VkRenderPassBeginInfo renderPassBeginInfo;
    renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassBeginInfo.pNext = nullptr;
    renderPassBeginInfo.renderPass = renderPass;
    renderPassBeginInfo.framebuffer = framebuffer;
    renderPassBeginInfo.renderArea.offset = {0, 0};
    renderPassBeginInfo.renderArea.extent = {config.width, config.height};
    renderPassBeginInfo.clearValueCount = clearValues.size();
    renderPassBeginInfo.pClearValues = clearValues.data();

    // _INLINE means to only use primary command buffers
    vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

    VkViewport viewport;
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = config.width;
    viewport.height = config.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    VkRect2D scissor;
    scissor.offset = {0, 0};
    scissor.extent = {config.width, config.height};
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    VkDeviceSize offsets[] = {0};

    for (JojoMesh &mesh : meshes) {
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, &(mesh.vertexBuffer), offsets);
        vkCmdBindIndexBuffer(commandBuffer, mesh.indexBuffer, 0, VK_INDEX_TYPE_UINT32);

        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1,
                                &(mesh.uniformDescriptorSet), 0,
                                nullptr);

        vkCmdDrawIndexed(commandBuffer, mesh.indices.size(), 1, 0, 0, 0);
    }

    vkCmdEndRenderPass(commandBuffer);
}

void bindBufferToDescriptorSet(VkDevice device, VkBuffer uniformBuffer, VkDescriptorSet descriptorSet) {
    VkDescriptorBufferInfo descriptorBufferInfo;
    descriptorBufferInfo.buffer = uniformBuffer;
    descriptorBufferInfo.offset = 0;
    descriptorBufferInfo.range = sizeof(glm::mat4);

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

void destroyPipeline() {
    vkDestroyPipeline(device, pipeline, nullptr);
    vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
    vkDestroyShaderModule(device, shaderModuleVert, nullptr);
    vkDestroyShaderModule(device, shaderModuleFrag, nullptr);
}


void destroySwapchainChildren() {
    for (auto &framebuffer : framebuffers) {
        vkDestroyFramebuffer(device, framebuffer, nullptr);
    }

    vkDestroyImageView(device, depthImageView, nullptr);
    vkDestroyImage(device, depthImage, nullptr);
    vkFreeMemory(device, depthImageMemory, nullptr);

    vkDestroyRenderPass(device, renderPass, nullptr);

    for (auto &imageView : imageViews) {
        vkDestroyImageView(device, imageView, nullptr);
    }
}


void createPipeline(Config &config) {
    VkPipelineShaderStageCreateInfo shaderStageCreateInfoVert;
    VkResult result = createShaderStageCreateInfo(device, "../shader/shader.vert.spv", VK_SHADER_STAGE_VERTEX_BIT,
                                                  &shaderStageCreateInfoVert, &shaderModuleVert);
    ASSERT_VULKAN(result)

    VkPipelineShaderStageCreateInfo shaderStageCreateInfoFrag;
    result = createShaderStageCreateInfo(device, "../shader/shader.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT,
                                         &shaderStageCreateInfoFrag, &shaderModuleFrag);
    ASSERT_VULKAN(result)

    VkPipelineShaderStageCreateInfo shaderStages[] = {shaderStageCreateInfoVert, shaderStageCreateInfoFrag};

    result = createPipelineLayout(device, &descriptorSetLayout, &pipelineLayout);
    ASSERT_VULKAN(result)

    result = createPipeline(device, shaderStages, renderPass, pipelineLayout, &pipeline, config.width,
                            config.height);
    ASSERT_VULKAN(result)
}

void createSwapchainAndChildren(Config &config) {
    VkResult result;
    auto chosenImageFormat = VK_FORMAT_B8G8R8A8_UNORM;   // TODO: check if valid via surfaceFormats[i].format

    result = createSwapchain(device, surface, swapchain, &swapchain, chosenImageFormat, config.width, config.height);
    ASSERT_VULKAN(result)


    uint32_t numberOfImagesInSwapchain = 0;
    result = vkGetSwapchainImagesKHR(device, swapchain, &numberOfImagesInSwapchain, nullptr);
    ASSERT_VULKAN(result)

    // TODO: actually adjust the number of command buffers
    if (numberOfImagesInSwapchain > numberOfCommandBuffers)
        throw std::runtime_error("we didnt plan for this... (more than three swapchain images)");

    // doesnt have to be freed because they are allocated by the swapchain -> error when trying to destroy
    std::vector<VkImage> swapchainImages;
    swapchainImages.resize(numberOfImagesInSwapchain);
    result = vkGetSwapchainImagesKHR(device, swapchain, &numberOfImagesInSwapchain, swapchainImages.data());
    ASSERT_VULKAN(result)

    imageViews.resize(numberOfImagesInSwapchain);
    for (size_t i = 0; i < numberOfImagesInSwapchain; ++i) {
        result = createImageView(device, swapchainImages[i], &(imageViews[i]), chosenImageFormat,
                                 VK_IMAGE_ASPECT_COLOR_BIT);
        ASSERT_VULKAN(result)
    }

    VkFormat depthFormat = findSupportedFormat(chosenDevice,
                                               {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT,
                                                VK_FORMAT_D24_UNORM_S8_UINT},
                                               VK_IMAGE_TILING_OPTIMAL,
                                               VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
    );
    createImage(device, chosenDevice, config.width, config.height, depthFormat, VK_IMAGE_TILING_OPTIMAL,
                VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, depthImage,
                &depthImageMemory);
    ASSERT_VULKAN(result)
    result = createImageView(device, depthImage, &depthImageView, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);
    ASSERT_VULKAN(result)

    changeImageLayout(device, commandPool, queue, depthImage, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED,
                      VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);


    result = createRenderpass(device, &renderPass, chosenImageFormat, depthFormat);
    ASSERT_VULKAN(result)


    framebuffers.resize(numberOfImagesInSwapchain);
    for (size_t i = 0; i < numberOfImagesInSwapchain; ++i) {
        result = createFramebuffer(device, renderPass, imageViews[i], depthImageView, &(framebuffers[i]), config.width,
                                   config.height);
        ASSERT_VULKAN(result)
    }
}

void recreateSwapchain(Config &config) {
    VkResult result;

    result = vkDeviceWaitIdle(device);
    ASSERT_VULKAN(result)

    VkSurfaceCapabilitiesKHR surfaceCapabilitiesKHR;
    result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(chosenDevice, surface, &surfaceCapabilitiesKHR);
    ASSERT_VULKAN(result)

    int newWidth, newHeight;
    glfwGetWindowSize(window, &newWidth, &newHeight);

    config.width = (uint32_t) std::min(newWidth, (int) surfaceCapabilitiesKHR.maxImageExtent.width);;
    config.height = (uint32_t) std::min(newHeight, (int) surfaceCapabilitiesKHR.maxImageExtent.height);

    destroySwapchainChildren();

    VkSwapchainKHR oldSwapchain = swapchain;

    createSwapchainAndChildren(config);

    vkDestroySwapchainKHR(device, oldSwapchain, nullptr);
}


void startVulkan() {
    VkResult result;

    result = createInstance(&instance);
    ASSERT_VULKAN(result)

    //printInstanceLayers();
    //printInstanceExtensions();

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
    uint32_t chosenQueueFamilyIndex = 0;        // TODO: choose the best queue family

    result = createLogicalDevice(chosenDevice, &device, chosenQueueFamilyIndex);
    ASSERT_VULKAN(result)

    vkGetDeviceQueue(device, chosenQueueFamilyIndex, 0, &queue);

    result = checkSurfaceSupport(chosenDevice, surface, chosenQueueFamilyIndex);
    ASSERT_VULKAN(result)

    result = createCommandPool(device, &commandPool, chosenQueueFamilyIndex);
    ASSERT_VULKAN(result)

    result = allocateCommandBuffers(device, commandPool, commandBuffers, numberOfCommandBuffers);
    ASSERT_VULKAN(result)


    VkFenceCreateInfo fenceCreateInfo = {};
    fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    commandBufferFences.resize(numberOfCommandBuffers);
    for (auto &fence : commandBufferFences) {
        result = vkCreateFence(device, &fenceCreateInfo, nullptr, &fence);
        ASSERT_VULKAN(result)
    }

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

void startGlfw(Config &config) {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    window = glfwCreateWindow(config.width, config.height, "heikousen", nullptr, nullptr);
    glfwSetWindowSizeCallback(window, onWindowResized);
}


void drawFrame(Config &config, std::vector<JojoMesh> &meshes) {
    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(device, swapchain, std::numeric_limits<uint64_t>::max(),
                                            semaphoreImageAvailable, VK_NULL_HANDLE, &imageIndex);
    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        recreateSwapchain(config);
        return;
        // throw away this frame, because after recreating the swapchain, the vkAcquireNexImageKHR is
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

    result = vkWaitForFences(device, 1, &commandBufferFences[imageIndex], VK_TRUE, UINT64_MAX);
    ASSERT_VULKAN(result)
    result = vkResetFences(device, 1, &commandBufferFences[imageIndex]);
    ASSERT_VULKAN(result)


    result = beginCommandBuffer(commandBuffers[imageIndex]);
    ASSERT_VULKAN(result)

    recordCommandBuffer(config, commandBuffers[imageIndex], framebuffers[imageIndex], meshes);

    result = vkEndCommandBuffer(commandBuffers[imageIndex]);
    ASSERT_VULKAN(result)


    result = vkQueueSubmit(queue, 1, &submitInfo, commandBufferFences[imageIndex]);
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
        recreateSwapchain(config);
        return;
        // same as with vkAcquireNextImageKHR
    }
    ASSERT_VULKAN(result)
}

auto lastFrameTime = std::chrono::high_resolution_clock::now();

void updateMvp(Config &config, JojoPhysics &physics, std::vector<JojoMesh> &meshes) {
    auto now = std::chrono::high_resolution_clock::now();
    float timeSinceLastFrame = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastFrameTime).count() / 1000.0f;
    lastFrameTime = now;

    glm::mat4 view = glm::translate(glm::mat4(), glm::vec3(0.0f, 0.0f, -5.0f));
    //view[1][1] *= -1;
    glm::mat4 projection = glm::perspective(glm::radians(60.0f), config.width / (float) config.height, 0.001f, 100.0f);
    // openGL has the z dir flipped
    projection[1][1] *= -1;


    physics.dynamicsWorld->stepSimulation(timeSinceLastFrame);



    void *rawData;
    for (int i = 0; i < meshes.size(); ++i) {
        JojoMesh &mesh = meshes[i];


        // TODO: move this over to the physics class as well
        btCollisionObject *obj = physics.dynamicsWorld->getCollisionObjectArray()[i];
        btRigidBody *body = btRigidBody::upcast(obj);
        btTransform trans;
        if (body && body->getMotionState()) {
            body->getMotionState()->getWorldTransform(trans);
        } else {
            trans = obj->getWorldTransform();
        }
        btVector3 origin = trans.getOrigin();
        std::printf("%d " PRINTF_FLOAT " " PRINTF_FLOAT " " PRINTF_FLOAT " ",
                i, origin.getX(), origin.getY(), origin.getZ());
        auto& manifoldPoints = physics.objectsCollisions[body];
        if (manifoldPoints.empty()) {
            std::printf("0");
        } else {
            std::printf("1");
        }
        puts("");


        trans.getOpenGLMatrix(glm::value_ptr(mesh.modelMatrix));


        int direction = (i % 2 * 2 - 1);
        //mesh.modelMatrix = glm::rotate(mesh.modelMatrix, timeSinceLastFrame * glm::radians(30.0f) * direction, glm::vec3(0, 0, 1));
        glm::mat4 mvp = projection * view * mesh.modelMatrix;
        VkResult result = vkMapMemory(device, mesh.uniformBufferDeviceMemory, 0, sizeof(mvp), 0, &rawData);
        ASSERT_VULKAN(result)
        memcpy(rawData, &mvp, sizeof(mvp));
        vkUnmapMemory(device, mesh.uniformBufferDeviceMemory);
    }


}


void gameloop(Config &config, JojoPhysics &physics, std::vector<JojoMesh> &meshes) {
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        updateMvp(config, physics, meshes);

        drawFrame(config, meshes);
    }
}


void shutdownVulkan(std::vector<JojoMesh> &meshes) {
    // block until vulkan has finished
    VkResult result = vkDeviceWaitIdle(device);
    ASSERT_VULKAN(result)

    vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
    vkDestroyDescriptorPool(device, descriptorPool, nullptr);

    for (JojoMesh &mesh : meshes) {
        vkFreeMemory(device, mesh.uniformBufferDeviceMemory, nullptr);
        vkDestroyBuffer(device, mesh.uniformBuffer, nullptr);

        vkFreeMemory(device, mesh.indexBufferDeviceMemory, nullptr);
        vkDestroyBuffer(device, mesh.indexBuffer, nullptr);

        vkFreeMemory(device, mesh.vertexBufferDeviceMemory, nullptr);
        vkDestroyBuffer(device, mesh.vertexBuffer, nullptr);
    }

    for (auto &fence : commandBufferFences) {
        vkDestroyFence(device, fence, nullptr);
    }

    vkDestroySemaphore(device, semaphoreImageAvailable, nullptr);
    vkDestroySemaphore(device, semaphoreRenderingDone, nullptr);

    vkFreeCommandBuffers(device, commandPool, commandBuffers.size(), commandBuffers.data());

    destroyPipeline();
    destroySwapchainChildren();
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

void initializeBuffers(std::vector<JojoMesh> &meshes) {
    VkResult result = createDescriptorPool(device, &descriptorPool, meshes.size());
    ASSERT_VULKAN(result)

    result = createDescriptorSetLayout(device, &descriptorSetLayout);
    ASSERT_VULKAN(result)

    for (JojoMesh &mesh : meshes) {
        createAndUploadBuffer(device, chosenDevice, commandPool, queue, mesh.vertices,
                              VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                              &(mesh.vertexBuffer), &(mesh.vertexBufferDeviceMemory));
        createAndUploadBuffer(device, chosenDevice, commandPool, queue, mesh.indices, VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                              &(mesh.indexBuffer), &(mesh.indexBufferDeviceMemory));

        VkDeviceSize bufferSize = sizeof(glm::mat4);
        createBuffer(device, chosenDevice, bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, &(mesh.uniformBuffer),
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                     &(mesh.uniformBufferDeviceMemory));


        result = allocateDescriptorSet(device, descriptorPool, descriptorSetLayout, &(mesh.uniformDescriptorSet));
        ASSERT_VULKAN(result)
        bindBufferToDescriptorSet(device, mesh.uniformBuffer, mesh.uniformDescriptorSet);
    }
}





int main(int argc, char *argv[]) {
    hello_v8(argv[0]);

    Config config = Config::readFromFile("../config.ini");
    startGlfw(config);


    std::vector<JojoMesh> meshes;
    meshes.resize(2);

    JojoPhysics physics;

    float width = 0.5f, height = 0.5f, depth = 0.5f;

    for(int i = 0; i < meshes.size(); ++i) {
        meshes[i].vertices = {
                Vertex({ -width, -height, -depth}, {1.0f, 0.0f, 1.0f}),
                Vertex({ width, -height, -depth }, {1.0f, 1.0f, 0.0f}),
                Vertex({ width,  height, -depth }, {0.0f, 1.0f, 1.0f}),
                Vertex({ -width,  height, -depth}, {1.0f, 1.0f, 1.0f}),
                Vertex({ -width, -height,  depth}, {1.0f, 0.0f, 1.0f}),
                Vertex({ width, -height,  depth }, {1.0f, 1.0f, 0.0f}),
                Vertex({ width,  height,  depth }, {0.0f, 1.0f, 1.0f}),
                Vertex({ -width,  height,  depth }, {1.0f, 1.0f, 1.0f})
        };
        meshes[i].indices = { 0, 1, 2,
                              0, 3, 1,

                              1,2,6,
                              6,5,1,

                              4,5,6,
                              6,7,4,

                              2,3,6,
                              6,3,7,

                              0,7,3,
                              0,4,7,

                              0,1,5,
                              0,5,4
        };

        btCollisionShape *colShape = new btBoxShape(btVector3(0.5, 0.5, 0.5));

        physics.collisionShapes.push_back(colShape);

        btTransform startTransform;
        startTransform.setIdentity();

        startTransform.setOrigin(btVector3(0.0f, -2.0f * i, 0.0f));
        meshes[i].modelMatrix = glm::translate(glm::mat4(), glm::vec3(0.0f, -2.0f * i, 0.0f));

        btVector3 localInertia(0, 1, 0);
        btScalar mass(i + 1.0f);

        colShape->calculateLocalInertia(mass, localInertia);

        btDefaultMotionState *myMotionState = new btDefaultMotionState(startTransform);

        btRigidBody *body = new btRigidBody(btRigidBody::btRigidBodyConstructionInfo(mass, myMotionState, colShape, localInertia));
        body->setRestitution(objectRestitution);

        if(i == 0) {
            body->applyCentralImpulse(btVector3(0.0f, -1.0f, 0.0f));
        }

        physics.dynamicsWorld->addRigidBody(body);
    }


    startVulkan();

    createSwapchainAndChildren(config);

    initializeBuffers(meshes);

    createPipeline(config);

    gameloop(config, physics, meshes);

    shutdownVulkan(meshes);
    shutdownGlfw();

    return 0;
}

