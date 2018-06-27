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
    Config                &config,
    JojoEngine            *engine,
    VkRenderPass           renderPass,
    const std::string     &shaderName,
    VkDescriptorSetLayout  descriptorLayout,
    const uint32_t         attachmentCount,
    const std::vector<VkVertexInputBindingDescription> &vertexBindingDescriptions,
    const std::vector<VkVertexInputAttributeDescription> &vertexAttributeDescriptions,
    bool testDepth,
    bool writeDepth,
    bool alpha
) {
    descriptorSetLayout = descriptorLayout;

    VkPipelineShaderStageCreateInfo shaderStageCreateInfoVert;
    VkResult result = createShaderStageCreateInfo (
        engine->device,
        "shader/" + shaderName + ".vert.spv",
        VK_SHADER_STAGE_VERTEX_BIT,
        &shaderStageCreateInfoVert,
        &shaderModuleVert
    );
    ASSERT_VULKAN (result);

    VkPipelineShaderStageCreateInfo shaderStageCreateInfoFrag;
    result = createShaderStageCreateInfo (
        engine->device,
        "shader/" + shaderName + ".frag.spv",
        VK_SHADER_STAGE_FRAGMENT_BIT,
        &shaderStageCreateInfoFrag,
        &shaderModuleFrag
    );
    ASSERT_VULKAN (result);

    VkPipelineShaderStageCreateInfo shaderStages[] = {shaderStageCreateInfoVert, shaderStageCreateInfoFrag};

    result = createPipelineLayout(engine->device, &descriptorSetLayout, &pipelineLayout);
    ASSERT_VULKAN (result);

    VkPipelineColorBlendAttachmentState cbas = {};
    cbas.blendEnable         = VK_FALSE;
    cbas.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    cbas.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    cbas.colorBlendOp        = VK_BLEND_OP_ADD;
    cbas.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    cbas.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    cbas.alphaBlendOp        = VK_BLEND_OP_ADD;
    cbas.colorWriteMask      =
        VK_COLOR_COMPONENT_R_BIT
        | VK_COLOR_COMPONENT_G_BIT
        | VK_COLOR_COMPONENT_B_BIT
        | VK_COLOR_COMPONENT_A_BIT;

    std::vector<VkPipelineColorBlendAttachmentState> blendAttStates (
        attachmentCount, cbas
    );

    if (alpha) {
        blendAttStates[0].blendEnable = alpha;
    }

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
        blendAttStates,
        config.isWireframeEnabled,
        testDepth,
        writeDepth
    );

    ASSERT_VULKAN(result)
}

void JojoPipeline::rebuild(Config &config) {

}
