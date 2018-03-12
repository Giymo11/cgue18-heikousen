//
// Created by giymo11 on 3/11/18.
//

#pragma once

#include <vector>
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>


class Vertex {
public:
    glm::vec3 pos;
    glm::vec3 color;

    Vertex(glm::vec3 pos, glm::vec3 color)
            : pos(pos), color(color) {}

    static VkVertexInputBindingDescription getBindingDescription() {
        VkVertexInputBindingDescription vertexInputBindingDescription;

        vertexInputBindingDescription.binding = 0;
        vertexInputBindingDescription.stride = sizeof(Vertex);
        vertexInputBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return vertexInputBindingDescription;
    }

    static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions() {
        std::vector<VkVertexInputAttributeDescription> vertexInputAttributeDescriptions(2);
        vertexInputAttributeDescriptions[0].location = 0;   // location in shader
        vertexInputAttributeDescriptions[0].binding = 0;
        vertexInputAttributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;   // for vec2 in shader
        vertexInputAttributeDescriptions[0].offset = offsetof(Vertex, pos);

        vertexInputAttributeDescriptions[1].location = 1;
        vertexInputAttributeDescriptions[1].binding = 0;
        vertexInputAttributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;   // for vec3 in shader
        vertexInputAttributeDescriptions[1].offset = offsetof(Vertex, color);

        return vertexInputAttributeDescriptions;
    }
};




