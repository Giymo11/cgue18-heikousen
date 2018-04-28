//
// Created by benja on 4/28/2018.
//

#include <vector>

#include <vulkan/vulkan.h>

#include <glm/glm.hpp>

#include "jojo_scene.hpp"

#include "jojo_vulkan_data.hpp"

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