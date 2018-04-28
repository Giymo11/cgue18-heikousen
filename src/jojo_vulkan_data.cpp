//
// Created by benja on 4/28/2018.
//

#include "jojo_vulkan_data.hpp"

#include <vector>

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>

#include "jojo_scene.hpp"
#include "jojo_vulkan_utils.hpp"
#include "jojo_vulkan.hpp"


JojoVulkanMesh::JojoVulkanMesh() {}

VkVertexInputBindingDescription JojoVulkanMesh::getVertexInputBindingDescription() {
    VkVertexInputBindingDescription vertexInputBindingDescription;

    vertexInputBindingDescription.binding = 0;
    vertexInputBindingDescription.stride = sizeof(JojoVertex);
    vertexInputBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    return vertexInputBindingDescription;
}

std::vector<VkVertexInputAttributeDescription> JojoVulkanMesh::getVertexInputAttributeDescriptions() {
    std::vector<VkVertexInputAttributeDescription> vertexInputAttributeDescriptions(2);
    vertexInputAttributeDescriptions[0].location = 0;   // location in shader
    vertexInputAttributeDescriptions[0].binding = 0;
    vertexInputAttributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;   // for vec2 in shader
    vertexInputAttributeDescriptions[0].offset = offsetof(JojoVertex, pos);

    vertexInputAttributeDescriptions[1].location = 1;
    vertexInputAttributeDescriptions[1].binding = 0;
    vertexInputAttributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;   // for vec3 in shader
    vertexInputAttributeDescriptions[1].offset = offsetof(JojoVertex, color);

    return vertexInputAttributeDescriptions;
}

void JojoVulkanMesh::bindBufferToDescriptorSet(VkDevice device,
                               VkBuffer uniformBuffer,
                               VkDescriptorSet descriptorSet) {

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
    writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    writeDescriptorSet.pImageInfo = nullptr;
    writeDescriptorSet.pBufferInfo = &descriptorBufferInfo;
    writeDescriptorSet.pTexelBufferView = nullptr;

    vkUpdateDescriptorSets(device, 1, &writeDescriptorSet, 0, nullptr);
}

void JojoVulkanMesh::initializeBuffers(JojoEngine *engine, JojoPipeline *pipeline) {

    createAndUploadBuffer(engine->device, engine->chosenDevice, engine->commandPool, engine->queue, scene->vertices,
                          VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                          &(vertexBuffer), &(vertexBufferDeviceMemory));
    createAndUploadBuffer(engine->device, engine->chosenDevice, engine->commandPool, engine->queue, scene->indices,
                          VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                          &(indexBuffer), &(indexBufferDeviceMemory));

    VkDeviceSize bufferSize = sizeof(glm::mat4);
    createBuffer(engine->device, engine->chosenDevice, bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                 &(uniformBuffer),
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 &(uniformBufferDeviceMemory));


    VkResult result = allocateDescriptorSet(engine->device, engine->descriptorPool, pipeline->descriptorSetLayout,
                                            &(uniformDescriptorSet));
    ASSERT_VULKAN(result)
    bindBufferToDescriptorSet(engine->device, uniformBuffer, uniformDescriptorSet);
}

void JojoVulkanMesh::destroyBuffers(JojoEngine *engine)  {
    vkFreeMemory(engine->device, uniformBufferDeviceMemory, nullptr);
    vkDestroyBuffer(engine->device, uniformBuffer, nullptr);

    vkFreeMemory(engine->device, indexBufferDeviceMemory, nullptr);
    vkDestroyBuffer(engine->device, indexBuffer, nullptr);

    vkFreeMemory(engine->device, vertexBufferDeviceMemory, nullptr);
    vkDestroyBuffer(engine->device, vertexBuffer, nullptr);
}