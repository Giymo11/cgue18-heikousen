#include <iostream>
#include <cstring>
#include <vector>
#include <algorithm>
#include <chrono>

#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_NO_STB_IMAGE
#define TINYGLTF_NO_STB_IMAGE_WRITE
#include <tiny_gltf.h>

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
#include "jojo_scene.hpp"


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

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }

}

void startGlfw(Config &config) {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    window = glfwCreateWindow(config.width, config.height, "heikousen", nullptr, nullptr);
    glfwSetWindowSizeCallback(window, onWindowResized);

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    glfwSetCursorPos(window, config.width / 2.0f, config.height / 2.0f);

    glfwSetKeyCallback(window, keyCallback);
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
    float timeSinceLastFrame =
            std::chrono::duration_cast<std::chrono::milliseconds>(now - lastFrameTime).count() / 1000.0f;
    lastFrameTime = now;

    std::cout << "frame time " << timeSinceLastFrame << std::endl;

    glm::mat4 view = glm::translate(glm::mat4(), glm::vec3(0.0f, 0.0f, -5.0f));
    //view[1][1] *= -1;
    glm::mat4 projection = glm::perspective(glm::radians(60.0f), config.width / (float) config.height, 0.001f, 100.0f);
    // openGL has the z dir flipped
    projection[1][1] *= -1;


    physics.dynamicsWorld->stepSimulation(timeSinceLastFrame);


    void *rawData;
    for (int i = 0; i < meshes.size(); ++i) {
        JojoMesh &mesh = meshes[i];

        // TODO: maybe move this over to the physics class as well
        btCollisionObject *obj = physics.dynamicsWorld->getCollisionObjectArray()[i];
        btRigidBody *body = btRigidBody::upcast(obj);

        btTransform trans;
        if (body && body->getMotionState()) {
            body->getMotionState()->getWorldTransform(trans);
        } else {
            trans = obj->getWorldTransform();
        }
        btVector3 origin = trans.getOrigin();

        auto &manifoldPoints = physics.objectsCollisions[body];

        trans.getOpenGLMatrix(glm::value_ptr(mesh.modelMatrix));

        //int direction = (i % 2 * 2 - 1);
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

        btVector3 relativeForce(0, 0, 0);

        int state = glfwGetKey(window, GLFW_KEY_W);
        if (state == GLFW_PRESS) {
            relativeForce = relativeForce + btVector3(0, 0, -1);
        }
        state = glfwGetKey(window, GLFW_KEY_S);
        if (state == GLFW_PRESS) {
            relativeForce = relativeForce + btVector3(0, 0, 1);
        }
        state = glfwGetKey(window, GLFW_KEY_A);
        if (state == GLFW_PRESS) {
            relativeForce = relativeForce + btVector3(-1, 0, 0);
        }
        state = glfwGetKey(window, GLFW_KEY_D);
        if (state == GLFW_PRESS) {
            relativeForce = relativeForce + btVector3(1, 0, 0);
        }
        state = glfwGetKey(window, GLFW_KEY_R);
        if (state == GLFW_PRESS) {
            relativeForce = relativeForce + btVector3(0, 1, 0);
        }
        state = glfwGetKey(window, GLFW_KEY_F);
        if (state == GLFW_PRESS) {
            relativeForce = relativeForce + btVector3(0, -1, 0);
        }

        state = glfwGetKey(window, GLFW_KEY_X);
        bool xPressed = state == GLFW_PRESS;

        state = glfwGetKey(window, GLFW_KEY_Z);
        bool yPressed = state == GLFW_PRESS;


        btVector3 relativeTorque(0, 0, 0);

        state = glfwGetKey(window, GLFW_KEY_Q);
        if (state == GLFW_PRESS) {
            relativeTorque = relativeTorque + btVector3(0, 1, 0);
        }
        state = glfwGetKey(window, GLFW_KEY_E);
        if (state == GLFW_PRESS) {
            relativeTorque = relativeTorque + btVector3(0, -1, 0);
        }

        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);

        double relXpos = xpos - config.width / 2.0f;
        double relYpos = ypos - config.height / 2.0f;

        double minX = config.width * config.deadzoneScreenPercentage / 100.0f;
        double minY = config.height * config.deadzoneScreenPercentage / 100.0f;

        double maxX = config.width * config.navigationScreenPercentage / 100.0f;
        double maxY = config.height * config.navigationScreenPercentage / 100.0f;

        double absXpos = glm::abs(relXpos);
        double absYpos = glm::abs(relYpos);

        if (absXpos > minX) {
            if (absXpos > maxX) {
                absXpos = maxX;
            }
            double torque = (absXpos - minX) / (maxX - minX) * glm::sign(relXpos) * -1;
            relativeTorque = relativeTorque + btVector3(0, 0, torque);
        }

        if (absYpos > minY) {
            if (absYpos > maxY) {
                absYpos = maxY;
            }
            double torque = (absYpos - minY) / (maxY - minY) * glm::sign(relYpos) * -1;
            relativeTorque = relativeTorque + btVector3(torque, 0, 0);
        }

        double newXpos = absXpos * glm::sign(relXpos) + config.width / 2.0f;
        double newYpos = absYpos * glm::sign(relYpos) + config.height / 2.0f;
        glfwSetCursorPos(window, newXpos, newYpos);
        // TODO: fix dis


        if (!relativeForce.isZero() || !relativeTorque.isZero() || xPressed || yPressed) {
            btCollisionObject *obj = physics.dynamicsWorld->getCollisionObjectArray()[0];
            btRigidBody *body = btRigidBody::upcast(obj);

            btTransform trans;
            if (body && body->getMotionState()) {
                body->getMotionState()->getWorldTransform(trans);

                btMatrix3x3 &boxRot = trans.getBasis();

                if (xPressed) {
                    if (body->getLinearVelocity().norm() < 0.01) {
                        // stop the jiggling around
                        body->setLinearVelocity(btVector3(0, 0, 0));
                    } else {
                        // counteract the current inertia
                        // TODO: think about maybe making halting easier than accelerating.
                        btVector3 correctedForce = (body->getLinearVelocity() * -1).normalized();
                        body->applyCentralForce(correctedForce);
                    }
                }
                if (yPressed) {
                    if (body->getAngularVelocity().norm() < 0.01) {
                        body->setAngularVelocity(btVector3(0, 0, 0));
                    } else {
                        btVector3 correctedTorque = (body->getAngularVelocity() * -1).normalized();
                        body->applyTorque(correctedTorque);
                    }
                }

                if (!xPressed) {
                    if (!relativeForce.isZero()) {
                        // TODO: decide about maybe normalizing
                        btVector3 correctedForce = boxRot * relativeForce;
                        body->applyCentralForce(correctedForce);
                    }
                }
                if (!yPressed) {
                    if (!relativeTorque.isZero()) {
                        btVector3 correctedTorque = boxRot * relativeTorque;
                        body->applyTorque(correctedTorque);
                    }
                }

            } else {
                // for later use
                trans = obj->getWorldTransform();
            }
        }

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


void
createCube(JojoMesh *meshes, JojoPhysics &physics, float width, float height, float depth, float x, float y, float z) {
    meshes->vertices = {
            Vertex({-width, -height, -depth}, {1.0f, 0.0f, 1.0f}),
            Vertex({width, -height, -depth}, {1.0f, 1.0f, 0.0f}),
            Vertex({width, height, -depth}, {0.0f, 1.0f, 1.0f}),
            Vertex({-width, height, -depth}, {1.0f, 1.0f, 1.0f}),
            Vertex({-width, -height, depth}, {1.0f, 0.0f, 1.0f}),
            Vertex({width, -height, depth}, {1.0f, 1.0f, 0.0f}),
            Vertex({width, height, depth}, {0.0f, 1.0f, 1.0f}),
            Vertex({-width, height, depth}, {1.0f, 1.0f, 1.0f})
    };
    meshes->indices = {0, 1, 2,
                       0, 3, 1,

                       1, 2, 6,
                       6, 5, 1,

                       4, 5, 6,
                       6, 7, 4,

                       2, 3, 6,
                       6, 3, 7,

                       0, 7, 3,
                       0, 4, 7,

                       0, 1, 5,
                       0, 5, 4
    };

    // bullet part
    btCollisionShape *colShape = new btBoxShape(btVector3(height, width, depth));

    physics.collisionShapes.push_back(colShape);

    btTransform startTransform;
    startTransform.setIdentity();


    startTransform.setOrigin(btVector3(x, y, z));
    meshes->modelMatrix = translate(glm::mat4(), glm::vec3(x, y, z));

    btVector3 localInertia(0, 1, 0);
    btScalar mass(1.0f);

    colShape->calculateLocalInertia(mass, localInertia);

    btDefaultMotionState *myMotionState = new btDefaultMotionState(startTransform);

    btRigidBody *body = new btRigidBody(
            btRigidBody::btRigidBodyConstructionInfo(mass, myMotionState, colShape, localInertia));
    body->setRestitution(objectRestitution);
    body->forceActivationState(DISABLE_DEACTIVATION);


    physics.dynamicsWorld->addRigidBody(body);

}


void loadNode(const tinygltf::Node &node, std::vector<JojoNode> nodes, const tinygltf::Model &model, std::vector<uint32_t>& indexBuffer, std::vector<Vertex>& vertexBuffer, std::vector<JojoMaterial>& materials)
{
    // Generate local node matrix
    glm::vec3 translation = glm::vec3(0.0f);
    if (node.translation.size() == 3) {
        translation = glm::make_vec3(node.translation.data());
    }
    glm::mat4 rotation = glm::mat4(1.0f);
    if (node.rotation.size() == 4) {
        glm::quat q = glm::make_quat(node.rotation.data());
        rotation = glm::mat4(q);
    }
    glm::vec3 scale = glm::vec3(1.0f);
    if (node.scale.size() == 3) {
        scale = glm::make_vec3(node.scale.data());
    }
    glm::mat4 localNodeMatrix = glm::mat4(1.0f);
    if (node.matrix.size() == 16) {
        localNodeMatrix = glm::make_mat4x4(node.matrix.data());
    } else {
        // T * R * S
        localNodeMatrix = glm::translate(glm::mat4(1.0f), translation) * rotation * glm::scale(glm::mat4(1.0f), scale);
    }
    // localNodeMatrix = parentMatrix * localNodeMatrix;

    JojoNode jojoNode;
    jojoNode.matrix = localNodeMatrix;
    jojoNode.name = node.name;


    // Node contains mesh data
    if (node.mesh > -1) {
        const tinygltf::Mesh mesh = model.meshes[node.mesh];
        jojoNode.name += " - " + mesh.name;

        for (const auto &primitive : mesh.primitives) {
            if (primitive.indices < 0) {
                continue;
            }
            uint32_t indexStart = static_cast<uint32_t>(indexBuffer.size());
            uint32_t vertexStart = static_cast<uint32_t>(vertexBuffer.size());
            uint32_t indexCount = 0;
            // Vertices
            {
                const float *bufferPos = nullptr;
                const float *bufferNormals = nullptr;
                const float *bufferTexCoords = nullptr;

                // Position attribute is required
                assert(primitive.attributes.find("POSITION") != primitive.attributes.end());

                const tinygltf::Accessor &posAccessor = model.accessors[primitive.attributes.find("POSITION")->second];
                const tinygltf::BufferView &posView = model.bufferViews[posAccessor.bufferView];
                bufferPos = reinterpret_cast<const float *>(&(model.buffers[posView.buffer].data[posAccessor.byteOffset + posView.byteOffset]));

                if (primitive.attributes.find("NORMAL") != primitive.attributes.end()) {
                    const tinygltf::Accessor &normAccessor = model.accessors[primitive.attributes.find("NORMAL")->second];
                    const tinygltf::BufferView &normView = model.bufferViews[normAccessor.bufferView];
                    bufferNormals = reinterpret_cast<const float *>(&(model.buffers[normView.buffer].data[normAccessor.byteOffset + normView.byteOffset]));
                }

                if (primitive.attributes.find("TEXCOORD_0") != primitive.attributes.end()) {
                    const tinygltf::Accessor &uvAccessor = model.accessors[primitive.attributes.find("TEXCOORD_0")->second];
                    const tinygltf::BufferView &uvView = model.bufferViews[uvAccessor.bufferView];
                    bufferTexCoords = reinterpret_cast<const float *>(&(model.buffers[uvView.buffer].data[uvAccessor.byteOffset + uvView.byteOffset]));
                }


                for (size_t v = 0; v < posAccessor.count; v++) {

                    //auto pos = localNodeMatrix * glm::vec4(glm::make_vec3(&bufferPos[v * 3]), 1.0f);
                    auto pos = glm::make_vec3(&bufferPos[v * 3]);
                    pos.y *= -1.0f;
                    //vert.normal = glm::normalize(glm::mat3(localNodeMatrix) * glm::vec3(bufferNormals ? glm::make_vec3(&bufferNormals[v * 3]) : glm::vec3(0.0f)));
                    //vert.uv = bufferTexCoords ? glm::make_vec2(&bufferTexCoords[v * 2]) : glm::vec3(0.0f);
                    // Vulkan coordinate system

                    //vert.normal.y *= -1.0f;

                    glm::vec3 color(1.0, 1.0, 1.0);

                    Vertex vert{pos, color};
                    vertexBuffer.push_back(vert);
                }
            }
            // Indices
            {
                const tinygltf::Accessor &accessor = model.accessors[primitive.indices];
                const tinygltf::BufferView &bufferView = model.bufferViews[accessor.bufferView];
                const tinygltf::Buffer &buffer = model.buffers[bufferView.buffer];

                indexCount = static_cast<uint32_t>(accessor.count);

                switch (accessor.componentType) {
                    case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT: {
                        uint32_t *buf = new uint32_t[accessor.count];
                        memcpy(buf, &buffer.data[accessor.byteOffset + bufferView.byteOffset], accessor.count * sizeof(uint32_t));
                        for (size_t index = 0; index < accessor.count; index++) {
                            indexBuffer.push_back(buf[index] + vertexStart);
                        }
                        break;
                    }
                    case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT: {
                        uint16_t *buf = new uint16_t[accessor.count];
                        memcpy(buf, &buffer.data[accessor.byteOffset + bufferView.byteOffset], accessor.count * sizeof(uint16_t));
                        for (size_t index = 0; index < accessor.count; index++) {
                            indexBuffer.push_back(buf[index] + vertexStart);
                        }
                        break;
                    }
                    case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE: {
                        uint8_t *buf = new uint8_t[accessor.count];
                        memcpy(buf, &buffer.data[accessor.byteOffset + bufferView.byteOffset], accessor.count * sizeof(uint8_t));
                        for (size_t index = 0; index < accessor.count; index++) {
                            indexBuffer.push_back(buf[index] + vertexStart);
                        }
                        break;
                    }
                    default:
                        std::cerr << "Index component type " << accessor.componentType << " not supported!" << std::endl;
                        return;
                }
            }
            jojoNode.primitives.push_back({ indexStart, indexCount, materials[primitive.material]});
        }
    }

    if (!node.children.empty()) {
        for (auto i = 0; i < node.children.size(); i++) {
            loadNode(model.nodes[node.children[i]], jojoNode.children, model, indexBuffer, vertexBuffer, materials);
        }
    }

    nodes.push_back(jojoNode);
}


int main(int argc, char *argv[]) {
    Scripting::Engine jojoScript;

    Scripting::GameObject helloObj;
    jojoScript.hookScript(helloObj, "../scripts/hello.js");
    helloObj.updateLogic();

    Config config = Config::readFromFile("../config.ini");
    startGlfw(config);




    JojoPhysics physics;





    int cubesAmount = 3;

    std::vector<JojoMesh> meshes;
    meshes.resize(cubesAmount + 1);

    tinygltf::Model gltfModel;
    tinygltf::TinyGLTF loader;
    std::string err;

    bool loaded = loader.LoadBinaryFromFile(&gltfModel, &err, "../models/duck.glb");
    if (!err.empty()) {
        std::cout << "Err: " << err << std::endl;
    }
    if (!loaded) {
        std::cout << "Failed to parse glTF: " << std::endl;
        return -1;
    }

    // loadImages(gltfModel, device, transferQueue);
    //loadMaterials(gltfModel, device, transferQueue);
    const tinygltf::Scene &scene = gltfModel.scenes[gltfModel.defaultScene];
    JojoScene jojoScene;

    std::vector<uint32_t> indexBuffer;
    std::vector<Vertex> vertexBuffer;

    // TODO: load materials and textures first
    std::vector<JojoMaterial> materials;

    for(auto &node : scene.nodes) {
        loadNode(gltfModel.nodes[node], jojoScene.children, gltfModel, indexBuffer, vertexBuffer, materials);
    }

    std::cout << "loaded " << jojoScene.children.size() << " own nodes" << std::endl;


    float width = 0.5f, height = 0.5f, depth = 0.5f;
    auto offset = -2.0f;

    for (int i = 1; i < meshes.size(); ++i) {
        createCube(&meshes[i], physics, width, height, depth, 0.0f, offset * i, 0.0f);
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
