#pragma once
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>

#include "jojo_bsp.hpp"
#include "jojo_vulkan_textures.hpp"

class JojoEngine;

namespace Level {

using namespace glm;

struct Vertex {
    vec3 pos;
    vec2 uv;
    vec2 light_uv;
    vec3 normal;
};

struct JojoLevel {
    std::unique_ptr<BSP::BSPData> bsp;

    VkBuffer          vertex;
    VkBuffer          index;
    VkBuffer          indexStaging;
    VkDeviceMemory    vertexMemory;
    VkDeviceMemory    indexMemory;
    VkDeviceMemory    indexStagingMemory;

    Textures::Texture texDiffuse;
    Textures::Texture texNormal;
    Textures::Texture texLightmap;
};

JojoLevel *alloc (
    const JojoEngine      *engine,
    const std::string     &bsp
);

VkVertexInputBindingDescription                vertexInputBinding ();
std::vector<VkVertexInputAttributeDescription> vertexAttributes   ();

void free (
    const JojoEngine      *engine,
    JojoLevel             *level
);

void stageVertexData (
    const JojoEngine      *engine,
    const JojoLevel       *level,
    const VkCommandBuffer  transferCmd
);

void cmdBuildAndStageIndicesNaively (
    const VkDevice         device,
    const VkQueue          transferQueue,
    const JojoLevel       *level,
    const VkCommandBuffer  transferCmd
);

void loadAndStageTextures (
    const JojoLevel       *level
);

}