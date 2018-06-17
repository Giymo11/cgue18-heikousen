//
// Created by benja on 4/28/2018.
//

#include <glm/gtc/matrix_inverse.hpp>

#include "jojo_vulkan_data.hpp"

#include "jojo_vulkan_utils.hpp"
#include "jojo_vulkan_textures.hpp"
#include "jojo_vulkan.hpp"

#include "Rendering/DescriptorSets.h"

JojoVulkanMesh::JojoVulkanMesh() {}

VkVertexInputBindingDescription JojoVulkanMesh::getVertexInputBindingDescription() {
    VkVertexInputBindingDescription vertexInputBindingDescription;

    vertexInputBindingDescription.binding = 0;
    vertexInputBindingDescription.stride = sizeof(Object::Vertex);
    vertexInputBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    return vertexInputBindingDescription;
}

std::vector<VkVertexInputAttributeDescription> JojoVulkanMesh::getVertexInputAttributeDescriptions() {
    std::vector<VkVertexInputAttributeDescription> attr(3);
    attr[0].location = 0;
    attr[0].binding = 0;
    attr[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    attr[0].offset = offsetof(Object::Vertex, pos);

    attr[1].location = 1;
    attr[1].binding = 0;
    attr[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    attr[1].offset = offsetof(Object::Vertex, nml);

    attr[2].location = 2;
    attr[2].binding = 0; 
    attr[2].format = VK_FORMAT_R32G32_SFLOAT;
    attr[2].offset = offsetof (Object::Vertex, tex);

    return attr;
}

void JojoVulkanMesh::initializeBuffers(JojoEngine *engine, Rendering::Set set) {

    assert(scene != nullptr);

    auto descriptors = engine->descriptors;
    descriptorSet = descriptors->set (set);

    createAndUploadBuffer(engine->device, engine->chosenDevice, engine->commandPool, engine->queue, scene->vertices,
                          VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                          &(vertexBuffer), &(vertexBufferDeviceMemory));
    createAndUploadBuffer(engine->device, engine->chosenDevice, engine->commandPool, engine->queue, scene->indices,
                          VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                          &(indexBuffer), &(indexBufferDeviceMemory));


    // Calculate required alignment based on minimum device offset alignment
    VkPhysicalDeviceProperties properties;
    vkGetPhysicalDeviceProperties(engine->chosenDevice, &properties);
    VkPhysicalDeviceMemoryProperties memoryProperties;
    vkGetPhysicalDeviceMemoryProperties (engine->chosenDevice, &memoryProperties);

    size_t minUboAlignment = properties.limits.minUniformBufferOffsetAlignment;
    dynamicAlignment = sizeof(ModelTransformations);
    if (minUboAlignment > 0) {
        dynamicAlignment = (dynamicAlignment + minUboAlignment - 1) & ~(minUboAlignment - 1);
    }

    size_t bufferSize = scene->mvps.size() * dynamicAlignment; 

    alignedTransforms = (ModelTransformations *) alignedAlloc(bufferSize, dynamicAlignment);
    assert(alignedTransforms);

    {
        materialInfoAlignment = sizeof (MaterialInfo);
        if (minUboAlignment > 0) {
            materialInfoAlignment = (materialInfoAlignment + minUboAlignment - 1) & ~(minUboAlignment - 1);
        }

        size_t materialInfoSize = scene->mvps.size () * materialInfoAlignment;
        
        createBuffer (
            engine->device,
            engine->chosenDevice,
            materialInfoSize,
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            &materialInfoBuffer,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            &materialInfoMemory
        );
    }

    {
        createBuffer (
            engine->device,
            engine->chosenDevice,
            sizeof (LightBlock),
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            &lightInfoBuffer,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            &lightInfoMemory
        );
    }

    createBuffer(engine->device, engine->chosenDevice, bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                 &(uniformBuffer),
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 &(uniformBufferDeviceMemory));

    createBuffer (
        engine->device,
        engine->chosenDevice,
        sizeof(GlobalTransformations),
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        &globalTransformationBuffer,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        &globalTransformationMemory
    );

    Textures::generateTextureArray (
        engine->allocator, engine->device,
        engine->commandPool, engine->queue,
        &diffuse256
    );

    VkDescriptorBufferInfo info = {};
    info.buffer = globalTransformationBuffer;
    info.range = sizeof (GlobalTransformations);
    descriptors->update (set, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, info);
    descriptors->update (Rendering::Set::Level, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, info);

    info = {};
    info.buffer = uniformBuffer;
    info.range = sizeof (ModelTransformations);
    descriptors->update (set, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, info);

    info = {};
    info.buffer = materialInfoBuffer;
    info.range = sizeof (MaterialInfo);
    descriptors->update (set, 2, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, info);

    info = {};
    info.buffer = lightInfoBuffer;
    info.range = sizeof (LightBlock);
    descriptors->update (set, 3, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, info);
    descriptors->update (Rendering::Set::Level, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, info);

    auto diffuse256desc = Textures::descriptor (&diffuse256);
    descriptors->update (set, 4, diffuse256desc);
}

void JojoVulkanMesh::destroyBuffers(JojoEngine *engine) {
    Textures::freeTexture (engine->allocator, engine->device, &diffuse256);

    vkFreeMemory(engine->device, uniformBufferDeviceMemory, nullptr);
    vkDestroyBuffer(engine->device, uniformBuffer, nullptr);

    vkFreeMemory(engine->device, indexBufferDeviceMemory, nullptr);
    vkDestroyBuffer(engine->device, indexBuffer, nullptr);

    vkFreeMemory(engine->device, vertexBufferDeviceMemory, nullptr);
    vkDestroyBuffer(engine->device, vertexBuffer, nullptr);
}

void JojoVulkanMesh::updateAlignedUniforms(const glm::mat4 &view) {
    assert(scene != nullptr);

    for (int i = 0; i < scene->mvps.size(); ++i) {
        // cast pointer to number to circumvent the step size of glm::mat4
        ModelTransformations *alignedMatrix = (ModelTransformations *) (((uint64_t) alignedTransforms + (i * dynamicAlignment)));
        alignedMatrix->normalMatrix = glm::mat4(glm::inverseTranspose (glm::mat3 (view * scene->mvps[i])));
        alignedMatrix->model = scene->mvps[i];
    }

}

void JojoVulkanMesh::drawNode (
    const VkCommandBuffer   cmd,
    const VkPipelineLayout  pipelineLayout,
    const Scene::Node      *node
) const {
    for (const auto &primitive : node->primitives) {
        uint32_t offsets[] = {
            primitive.dynamicMVP      * static_cast<uint32_t>(dynamicAlignment),
            primitive.dynamicMaterial * static_cast<uint32_t>(materialInfoAlignment)
        };

        vkCmdBindDescriptorSets (
            cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
            pipelineLayout, 0, 1, &descriptorSet,
            2, offsets
        );

        vkCmdDrawIndexed (
            cmd, primitive.indexCount, 1,
            primitive.indexOffset,
            primitive.vertexOffset, 0
        );
    }

    for (const auto &child : node->children) {
        drawNode(cmd, pipelineLayout, &child);
    }
}


