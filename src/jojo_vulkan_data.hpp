//
// Created by giymo11 on 3/11/18.
//

#pragma once


#include <vector>

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>

#include "jojo_scene.hpp"


class JojoVulkanMesh {
public:
    std::vector<JojoVertex> vertices;
    std::vector<uint32_t> indices;
    VkBuffer vertexBuffer;
    VkDeviceMemory vertexBufferDeviceMemory;
    VkBuffer indexBuffer;
    VkDeviceMemory indexBufferDeviceMemory;
    VkBuffer uniformBuffer;
    VkDeviceMemory uniformBufferDeviceMemory;
    VkDescriptorSet uniformDescriptorSet;
    glm::mat4 modelMatrix;

    JojoVulkanMesh() {};


    static VkVertexInputBindingDescription getVertexInputBindingDescription() {
        VkVertexInputBindingDescription vertexInputBindingDescription;

        vertexInputBindingDescription.binding = 0;
        vertexInputBindingDescription.stride = sizeof(JojoVertex);
        vertexInputBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return vertexInputBindingDescription;
    }

    static std::vector<VkVertexInputAttributeDescription> getVertexInputAttributeDescriptions() {
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
};



