//
// Created by giymo11 on 3/11/18.
//

#pragma once


#include <vector>

#include <vulkan/vulkan.h>

#include "jojo_scene.hpp"
#include "jojo_engine.hpp"
#include "jojo_pipeline.hpp"

class JojoVulkanMesh {
public:

    JojoScene *scene = nullptr;

    size_t dynamicAlignment;
    glm::mat4 *alignedMvps = nullptr;

    VkBuffer vertexBuffer;
    VkDeviceMemory vertexBufferDeviceMemory;
    VkBuffer indexBuffer;
    VkDeviceMemory indexBufferDeviceMemory;
    VkBuffer uniformBuffer;
    VkDeviceMemory uniformBufferDeviceMemory;
    VkDescriptorSet uniformDescriptorSet;

    VkDescriptorImageInfo texture;

    JojoVulkanMesh();


    static VkVertexInputBindingDescription getVertexInputBindingDescription();

    static std::vector<VkVertexInputAttributeDescription> getVertexInputAttributeDescriptions();


    void initializeBuffers(JojoEngine *engine, JojoPipeline *pipeline);

    void bindBufferToDescriptorSet(VkDevice device, VkBuffer uniformBuffer, VkDescriptorSet descriptorSet);

    void destroyBuffers(JojoEngine *engine);

    void updateAlignedUniforms(glm::mat4 proj_view);

    void goDrawYourself(VkCommandBuffer &commandBuffer, VkPipelineLayout &pipelineLayout);

    void drawNode(VkCommandBuffer &commandBuffer, VkPipelineLayout &pipelineLayout, const JojoNode &node);
};



