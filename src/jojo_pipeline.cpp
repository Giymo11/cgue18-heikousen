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

void JojoPipeline::createPipelineHelper(Config &config, JojoEngine *engine, VkRenderPass renderPass) {
    VkPipelineShaderStageCreateInfo shaderStageCreateInfoVert;
    VkResult result = createShaderStageCreateInfo(engine->device, "../shader/shader.vert.spv",
                                                  VK_SHADER_STAGE_VERTEX_BIT,
                                                  &shaderStageCreateInfoVert, &shaderModuleVert);
    ASSERT_VULKAN(result)

    VkPipelineShaderStageCreateInfo shaderStageCreateInfoFrag;
    result = createShaderStageCreateInfo(engine->device, "../shader/shader.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT,
                                         &shaderStageCreateInfoFrag, &shaderModuleFrag);
    ASSERT_VULKAN(result)

    VkPipelineShaderStageCreateInfo shaderStages[] = {shaderStageCreateInfoVert, shaderStageCreateInfoFrag};

    result = createPipelineLayout(engine->device, &descriptorSetLayout, &pipelineLayout);
    ASSERT_VULKAN(result)

    result = createPipeline(engine->device, shaderStages, renderPass, pipelineLayout, &pipeline, config.width,
                            config.height);
    ASSERT_VULKAN(result)
}

void JojoPipeline::initializeDescriptorSetLayout(JojoEngine *engine) {
    VkResult result = createDescriptorSetLayout(engine->device, &descriptorSetLayout);
    ASSERT_VULKAN(result)
}

void JojoPipeline::destroyDescriptorSetLayout(JojoEngine *engine) {
    vkDestroyDescriptorSetLayout(engine->device, descriptorSetLayout, nullptr);
}
