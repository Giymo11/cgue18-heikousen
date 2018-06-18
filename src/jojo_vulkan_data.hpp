//
// Created by giymo11 on 3/11/18.
//

#pragma once


#include <vector>
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

#include "jojo_scene.hpp"
#include "jojo_engine.hpp"
#include "jojo_pipeline.hpp"
#include "jojo_vulkan_textures.hpp"
#include "Rendering/DescriptorSets.h"

class JojoVulkanMesh {
public:
    struct LightSource {
        glm::vec3 position;
        float pad1;
        glm::vec3 color;
        float pad2;
        glm::vec3 attenuation;
        float pad3;
    };

    struct LightBlock {
        /**
         * x = gamma
         * y = HDR enable (0.0 = false default, 1.0 = true)
         * z = HDR exposure (1.0 default)
         */
        glm::vec4 parameters;
        LightSource sources[8];
    };

    struct GlobalTransformations {
        glm::mat4 projection;
        glm::mat4 view;
    };

    struct ModelTransformations {
        glm::mat4 model;
        glm::mat4 normalMatrix;
    };

    Scene::SceneTemplates *scene = nullptr;

    uint32_t alignModelTrans;
    uint32_t alignMaterialInfo;

    // Staging always
    VkBuffer lightInfo;
    VkBuffer modelTrans;
    VkBuffer globalTrans;
    VkBuffer stage_lightInfo;
    VkBuffer stage_modelTrans;
    VkBuffer stage_globalTrans;

    // Staging once
    VkBuffer vertex;
    VkBuffer index;
    VkBuffer materialInfo;
    VkBuffer stage_vertex;
    VkBuffer stage_index;
    VkBuffer stage_materialInfo;

    // Staging always
    VmaAllocation mem_lightInfo;
    VmaAllocation mem_modelTrans;
    VmaAllocation mem_globalTrans;
    VmaAllocation mems_lightInfo;
    VmaAllocation mems_modelTrans;
    VmaAllocation mems_globalTrans;
    VmaAllocationInfo alli_lightInfo;
    VmaAllocationInfo alli_modelTrans;
    VmaAllocationInfo alli_globalTrans;

    // Staging once
    VmaAllocation mem_vertex;
    VmaAllocation mem_index;
    VmaAllocation mem_materialInfo;
    VmaAllocation mems_vertex;
    VmaAllocation mems_index;
    VmaAllocation mems_materialInfo;

    Textures::Texture textures;

    VkDescriptorSet descriptorSet;

    JojoVulkanMesh();


    static VkVertexInputBindingDescription getVertexInputBindingDescription();

    static std::vector<VkVertexInputAttributeDescription> getVertexInputAttributeDescriptions();


    void initializeBuffers(JojoEngine *engine, Rendering::Set set);

    void destroyBuffers(JojoEngine *engine);

    void drawNode (
        const VkCommandBuffer   cmd,
        const VkPipelineLayout  pipelineLayout,
        const Scene::Node      *node
    ) const;
};



