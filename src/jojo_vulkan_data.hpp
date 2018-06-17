//
// Created by giymo11 on 3/11/18.
//

#pragma once


#include <vector>

#include <vulkan/vulkan.h>

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

    Scene::SceneTemplates *scene = nullptr;

    size_t dynamicAlignment;
    size_t materialInfoAlignment;
    ModelTransformations *alignedTransforms = nullptr;

    VkBuffer materialInfoBuffer;
    VkDeviceMemory materialInfoMemory;

    VkBuffer lightInfoBuffer;
    VkDeviceMemory lightInfoMemory;

    VkBuffer vertexBuffer;
    VkDeviceMemory vertexBufferDeviceMemory;
    VkBuffer indexBuffer;
    VkDeviceMemory indexBufferDeviceMemory;
    VkBuffer uniformBuffer;
    VkDeviceMemory uniformBufferDeviceMemory;
    VkBuffer globalTransformationBuffer;
    VkDeviceMemory globalTransformationMemory;
    VkDescriptorSet descriptorSet;

    Textures::Texture diffuse256;

    JojoVulkanMesh();


    static VkVertexInputBindingDescription getVertexInputBindingDescription();

    static std::vector<VkVertexInputAttributeDescription> getVertexInputAttributeDescriptions();


    void initializeBuffers(JojoEngine *engine, Rendering::Set set);

    void destroyBuffers(JojoEngine *engine);

    void updateAlignedUniforms(const glm::mat4 &view);

    void drawNode (
        const VkCommandBuffer   cmd,
        const VkPipelineLayout  pipelineLayout,
        const Scene::Node      *node
    ) const;
};



