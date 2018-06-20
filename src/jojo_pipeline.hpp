//
// Created by benja on 4/28/2018.
//

#pragma once

#include <vulkan/vulkan.h>

#include "jojo_engine.hpp"
#include "jojo_utils.hpp"

class JojoPipeline {
public:
    VkShaderModule shaderModuleVert;
    VkShaderModule shaderModuleFrag;

    VkPipelineLayout pipelineLayout;
    VkPipeline pipeline;

    VkDescriptorSetLayout descriptorSetLayout;

    void rebuild(Config &config);

    void destroyPipeline(JojoEngine *engine);

    void createPipelineHelper (
        Config                &config,
        JojoEngine            *engine,
        VkRenderPass           renderPass,
        const std::string     &shaderName,
        VkDescriptorSetLayout  descriptorLayout,
        uint32_t               attachmentCount,
        const std::vector<VkVertexInputBindingDescription> &vertexBindingDescriptions = {},
        const std::vector<VkVertexInputAttributeDescription> &vertexAttributeDescriptions = {}
    );
};