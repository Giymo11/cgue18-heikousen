//
// Created by giymo11 on 3/11/18.
//

#pragma once


#include <vector>

#include <vulkan/vulkan.h>

#include "jojo_scene.hpp"
#include "jojo_engine.hpp"
#include "jojo_pipeline.hpp"

#include "Rendering/DescriptorSets.h"

class JojoVulkanMesh {
public:
    struct GlobalTransformations {
        glm::mat4 projection;
        glm::mat4 view;
    };

    struct ModelTransformations {
        glm::mat4 model;
        glm::mat4 normalMatrix;
    };

    JojoScene *scene = nullptr;

    size_t dynamicAlignment;
    ModelTransformations *alignedTransforms = nullptr;

    VkBuffer vertexBuffer;
    VkDeviceMemory vertexBufferDeviceMemory;
    VkBuffer indexBuffer;
    VkDeviceMemory indexBufferDeviceMemory;
    VkBuffer uniformBuffer;
    VkDeviceMemory uniformBufferDeviceMemory;
    VkBuffer globalTransformationBuffer;
    VkDeviceMemory globalTransformationMemory;
    VkDescriptorSet descriptorSet;

    VkDescriptorImageInfo texture;

    JojoVulkanMesh();


    static VkVertexInputBindingDescription getVertexInputBindingDescription();

    static std::vector<VkVertexInputAttributeDescription> getVertexInputAttributeDescriptions();


    void initializeBuffers(JojoEngine *engine, JojoPipeline *pipeline, Rendering::Set set);

    void destroyBuffers(JojoEngine *engine);

    void updateAlignedUniforms(const glm::mat4 &view);

    void goDrawYourself(VkCommandBuffer &commandBuffer, VkPipelineLayout &pipelineLayout);

    void drawNode(VkCommandBuffer &commandBuffer, VkPipelineLayout &pipelineLayout, const JojoNode &node);
};



