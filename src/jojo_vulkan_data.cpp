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

    assert(scene != null);

    createAndUploadBuffer(engine->device, engine->chosenDevice, engine->commandPool, engine->queue, scene->vertices,
                          VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                          &(vertexBuffer), &(vertexBufferDeviceMemory));
    createAndUploadBuffer(engine->device, engine->chosenDevice, engine->commandPool, engine->queue, scene->indices,
                          VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                          &(indexBuffer), &(indexBufferDeviceMemory));


    // Calculate required alignment based on minimum device offset alignment
    VkPhysicalDeviceProperties properties;
    vkGetPhysicalDeviceProperties(engine->chosenDevice, &properties);

    size_t minUboAlignment = properties.limits.minUniformBufferOffsetAlignment;
    dynamicAlignment = sizeof(glm::mat4);
    if (minUboAlignment > 0) {
        dynamicAlignment = (dynamicAlignment + minUboAlignment - 1) & ~(minUboAlignment - 1);
    }

    size_t bufferSize = scene->mvps.size() * dynamicAlignment;

    alignedMvps = (glm::mat4*)alignedAlloc(bufferSize, dynamicAlignment);
    assert(alignedMvps);

    std::cout << "minUniformBufferOffsetAlignment = " << minUboAlignment << std::endl;
    std::cout << "dynamicAlignment = " << dynamicAlignment << std::endl;


    // TODO: actually do this for the dynamic buffers
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

void JojoVulkanMesh::updateAlignedUniforms(glm::mat4 proj_view) {
    assert(scene != nullptr);

    for(int i = 0; i < scene->mvps.size(); ++i) {
        // cast pointer to number to circumvent the step size of glm::mat4
        glm::mat4 *alignedMatrix = (glm::mat4*)(((uint64_t)alignedMvps + (i * dynamicAlignment)));
        *alignedMatrix = proj_view * scene->mvps[i];
    }

}

void JojoVulkanMesh::drawNode(VkCommandBuffer &commandBuffer, VkPipelineLayout &pipelineLayout, const JojoNode &node) {
    for(const JojoPrimitive &primitive : node.primitives) {
        uint32_t dynamicOffset = primitive.dynamicOffset * static_cast<uint32_t>(dynamicAlignment);

        vkCmdBindDescriptorSets(commandBuffer,
                                VK_PIPELINE_BIND_POINT_GRAPHICS,
                                pipelineLayout,
                                0,
                                1,
                                &uniformDescriptorSet,
                                1,
                                &dynamicOffset);

        vkCmdDrawIndexed(commandBuffer, primitive.indexCount, 1, primitive.indexOffset, 0, 0);
    }
    for(const JojoNode &child : node.children) {
        drawNode(commandBuffer, pipelineLayout, node);
    }
}

void JojoVulkanMesh::goDrawYourself(VkCommandBuffer &commandBuffer, VkPipelineLayout &pipelineLayout) {
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer, offsets);
    vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);

    for(JojoNode &child : scene->children) {
        drawNode(commandBuffer, pipelineLayout, child);
    }

}


