//
// Created by benja on 4/28/2018.
//

#include "jojo_pipeline.hpp"

#include "jojo_vulkan.hpp"
#include "jojo_vulkan_utils.hpp"

void JojoPipeline::destroyPipeline(JojoEngine *engine) {
    vkDestroyPipeline(engine->device, pipeline, nullptr);
    vkDestroyPipelineLayout(engine->device, pipelineLayout, nullptr);
    vkDestroyShaderModule(engine->device, shaderModuleVert, nullptr);
    vkDestroyShaderModule(engine->device, shaderModuleFrag, nullptr);
}

void JojoPipeline::createPipelineHelper (
    Config &config,
    JojoEngine *engine,
    VkRenderPass renderPass,
    const std::string &shaderName,
    VkDescriptorSetLayout descriptorLayout,
    const std::vector<VkVertexInputBindingDescription> &vertexBindingDescriptions,
    const std::vector<VkVertexInputAttributeDescription> &vertexAttributeDescriptions
) {
    descriptorSetLayout = descriptorLayout;

    VkPipelineShaderStageCreateInfo shaderStageCreateInfoVert;
    VkResult result = createShaderStageCreateInfo(engine->device, "shader/" + shaderName + ".vert.spv",
                                                  VK_SHADER_STAGE_VERTEX_BIT,
                                                  &shaderStageCreateInfoVert, &shaderModuleVert);
    ASSERT_VULKAN(result)

    VkPipelineShaderStageCreateInfo shaderStageCreateInfoFrag;
    result = createShaderStageCreateInfo(engine->device, "shader/" + shaderName + ".frag.spv",
                                         VK_SHADER_STAGE_FRAGMENT_BIT,
                                         &shaderStageCreateInfoFrag, &shaderModuleFrag);
    ASSERT_VULKAN(result)

    VkPipelineShaderStageCreateInfo shaderStages[] = {shaderStageCreateInfoVert, shaderStageCreateInfoFrag};

    result = createPipelineLayout(engine->device, &descriptorSetLayout, &pipelineLayout);
    ASSERT_VULKAN(result)

    result = createPipeline (
        engine->device,
        shaderStages,
        renderPass, 
        pipelineLayout, 
        &pipeline, 
        config.width,
        config.height,
        vertexBindingDescriptions,
        vertexAttributeDescriptions,
        config.isWireframeEnabled
    );

    ASSERT_VULKAN(result)
}

void JojoPipeline::rebuild(Config &config) {

}
