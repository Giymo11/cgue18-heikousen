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

    struct MaterialInfo {
        float ambient;
        float diffuse;
        float specular;
        float alpha;

        float texture;
        float param1;
        float param2;
        float param3;
    };

    JojoScene *scene = nullptr;

    size_t dynamicAlignment;
    size_t materialInfoAlignment;
    ModelTransformations *alignedTransforms = nullptr;

    VkBuffer materialInfoBuffer;
    VkDeviceMemory materialInfoMemory;

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

    void drawNode(VkCommandBuffer &commandBuffer, VkPipelineLayout &pipelineLayout, const JojoNode *node);
};



