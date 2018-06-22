//
// Created by benja on 4/28/2018.
//
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
    VkBufferCreateInfo binfo              = {};
    VmaAllocationCreateInfo allocInfo     = {};
    VkPhysicalDeviceProperties properties = {};
    auto allocator                        = engine->allocator;

    // Assign descriptor
    auto descriptors = engine->descriptors;
    descriptorSet = descriptors->set (set);

    // Prepare buffer creation
    binfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    
    // Find UBO alignment
    vkGetPhysicalDeviceProperties (engine->chosenDevice, &properties);
    uint32_t minUboAlignment = (uint32_t)properties.limits.minUniformBufferOffsetAlignment;
    alignModelTrans          = sizeof (ModelTransformations);
    alignMaterialInfo        = sizeof (Object::Material);
    if (minUboAlignment > 0) {
        alignModelTrans = (alignModelTrans + minUboAlignment - 1)
            & ~(minUboAlignment - 1);
        alignMaterialInfo = (alignMaterialInfo + minUboAlignment - 1)
            & ~(minUboAlignment - 1);
    }

    // --------------------------------------------------------------
    // BUFFER: VERTEX BEGIN
    // --------------------------------------------------------------

    {
        const auto size = scene->vertices.size () * sizeof (Object::Vertex);
        allocInfo       = {};
        
        binfo.size = size;
        binfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT
            | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        binfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
        allocInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

        ASSERT_VULKAN (vmaCreateBuffer (
            allocator, &binfo, &allocInfo,
            &vertex, &mem_vertex, nullptr
        ));

        binfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        binfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        allocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
        allocInfo.requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;

        ASSERT_VULKAN (vmaCreateBuffer (
            allocator, &binfo, &allocInfo,
            &stage_vertex, &mems_vertex, nullptr
        ));

        Object::Vertex *vertexData;
        vmaMapMemory (allocator, mems_vertex, (void **)&vertexData);
        std::copy (
            scene->vertices.begin (),
            scene->vertices.end (),
            vertexData
        );
        vmaUnmapMemory (allocator, mems_vertex);
    }

    // --------------------------------------------------------------
    // BUFFER: VERTEX END
    // --------------------------------------------------------------

    // --------------------------------------------------------------
    // BUFFER: INDEX BEGIN
    // --------------------------------------------------------------

    {
        const auto size = scene->indices.size () * sizeof (uint32_t);
        allocInfo = {};

        binfo.size = size;
        binfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT
            | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        binfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
        allocInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

        ASSERT_VULKAN (vmaCreateBuffer (
            allocator, &binfo, &allocInfo,
            &index, &mem_index, nullptr
        ));

        binfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        binfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        allocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
        allocInfo.requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;

        ASSERT_VULKAN (vmaCreateBuffer (
            allocator, &binfo, &allocInfo,
            &stage_index, &mems_index, nullptr
        ));

        uint32_t *indexData;
        vmaMapMemory (allocator, mems_index, (void **)&indexData);
        std::copy (
            scene->indices.begin (),
            scene->indices.end (),
            indexData
        );
        vmaUnmapMemory (allocator, mems_index);
    }

    // --------------------------------------------------------------
    // BUFFER: INDEX END
    // --------------------------------------------------------------

    // --------------------------------------------------------------
    // BUFFER: MATERIAL INFO BEGIN
    // --------------------------------------------------------------

    {
        const auto size = alignMaterialInfo * sizeof (Object::Material);
        allocInfo = {};

        binfo.size = size;
        binfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT
            | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        binfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
        allocInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

        ASSERT_VULKAN (vmaCreateBuffer (
            allocator, &binfo, &allocInfo,
            &materialInfo, &mem_materialInfo,
            nullptr
        ));

        binfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        binfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        allocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
        allocInfo.requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;

        ASSERT_VULKAN (vmaCreateBuffer (
            allocator, &binfo, &allocInfo,
            &stage_materialInfo, &mems_materialInfo, nullptr
        ));

        const auto &materials  = scene->materials;
        const uint32_t numMats = (uint32_t)materials.size ();
        uint8_t *materialData;
        vmaMapMemory (allocator, mems_materialInfo, (void **)&materialData);
        for (uint32_t i = 0; i < numMats; i++) {
            auto m = (Object::Material *)(
                materialData + alignMaterialInfo * i
            );

            *m = materials[i];
        }
        vmaUnmapMemory (allocator, mems_materialInfo);
    }

    // --------------------------------------------------------------
    // BUFFER: MATERIAL INFO END
    // --------------------------------------------------------------

    // --------------------------------------------------------------
    // BUFFER: LIGHT INFO BEGIN
    // --------------------------------------------------------------

    {
        const auto size = sizeof (LightBlock);
        allocInfo = {};

        binfo.size = size;
        binfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT
            | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        binfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
        allocInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

        ASSERT_VULKAN (vmaCreateBuffer (
            allocator, &binfo, &allocInfo,
            &lightInfo, &mem_lightInfo,
            nullptr
        ));

        binfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        binfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        allocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
        allocInfo.requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
        allocInfo.preferredFlags = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
            | VK_MEMORY_PROPERTY_HOST_CACHED_BIT;

        ASSERT_VULKAN (vmaCreateBuffer (
            allocator, &binfo, &allocInfo,
            &stage_lightInfo, &mems_lightInfo,
            &alli_lightInfo
        ));
    }

    // --------------------------------------------------------------
    // BUFFER: LIGHT INFO END
    // --------------------------------------------------------------

    // --------------------------------------------------------------
    // BUFFER: MODEL TRANS BEGIN
    // --------------------------------------------------------------

    {
        const auto size = alignModelTrans * scene->nextDynTrans;
        allocInfo = {};

        binfo.size = size;
        binfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT
            | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        binfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
        allocInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

        ASSERT_VULKAN (vmaCreateBuffer (
            allocator, &binfo, &allocInfo,
            &modelTrans, &mem_modelTrans,
            nullptr
        ));

        binfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        binfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        allocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
        allocInfo.requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
        allocInfo.preferredFlags = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
            | VK_MEMORY_PROPERTY_HOST_CACHED_BIT;

        ASSERT_VULKAN (vmaCreateBuffer (
            allocator, &binfo, &allocInfo,
            &stage_modelTrans, &mems_modelTrans,
            &alli_modelTrans
        ));
    }

    // --------------------------------------------------------------
    // BUFFER: MODEL TRANS END
    // --------------------------------------------------------------

    // --------------------------------------------------------------
    // BUFFER: GLOBAL TRANS BEGIN
    // --------------------------------------------------------------

    {
        const auto size = sizeof (GlobalTransformations);
        allocInfo = {};

        binfo.size = size;
        binfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT
            | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        binfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
        allocInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

        ASSERT_VULKAN (vmaCreateBuffer (
            allocator, &binfo, &allocInfo,
            &globalTrans, &mem_globalTrans,
            nullptr
        ));

        binfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        binfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        allocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
        allocInfo.requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
        allocInfo.preferredFlags = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
            | VK_MEMORY_PROPERTY_HOST_CACHED_BIT;

        ASSERT_VULKAN (vmaCreateBuffer (
            allocator, &binfo, &allocInfo,
            &stage_globalTrans, &mems_globalTrans,
            &alli_globalTrans
        ));
    }

    // --------------------------------------------------------------
    // BUFFER: GLOBAL TRANS END
    // --------------------------------------------------------------

    // --------------------------------------------------------------
    // BUFFER: DOF INFO BEGIN
    // --------------------------------------------------------------

    {
        const auto size = sizeof (DepthOfField);
        allocInfo = {};

        binfo.size = size;
        binfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT
            | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        binfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
        allocInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

        ASSERT_VULKAN (vmaCreateBuffer (
            allocator, &binfo, &allocInfo,
            &dofInfo, &mem_dofInfo,
            nullptr
        ));

        binfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        binfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        allocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
        allocInfo.requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
        allocInfo.preferredFlags = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
            | VK_MEMORY_PROPERTY_HOST_CACHED_BIT;

        ASSERT_VULKAN (vmaCreateBuffer (
            allocator, &binfo, &allocInfo,
            &stage_dofInfo, &mems_dofInfo,
            &alli_dofInfo
        ));
    }

    // --------------------------------------------------------------
    // BUFFER: DOF INFO END
    // --------------------------------------------------------------

    VkDescriptorBufferInfo info = {};
    info.buffer = globalTrans;
    info.range = sizeof (GlobalTransformations);
    descriptors->update (set, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, info);
    descriptors->update (Rendering::Set::Level, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, info);

    info = {};
    info.buffer = modelTrans;
    info.range = sizeof (ModelTransformations);
    descriptors->update (set, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, info);

    info = {};
    info.buffer = materialInfo;
    info.range = sizeof (Object::Material);
    descriptors->update (set, 2, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, info);
}

void JojoVulkanMesh::destroyBuffers (
    JojoEngine *engine
) {
    auto allocator = engine->allocator;

    Textures::freeTexture (allocator, engine->device, &textures);
    vmaDestroyBuffer (allocator, stage_globalTrans, mems_globalTrans);
    vmaDestroyBuffer (allocator, globalTrans, mem_globalTrans);
    vmaDestroyBuffer (allocator, stage_modelTrans, mems_modelTrans);
    vmaDestroyBuffer (allocator, modelTrans, mem_modelTrans);
    vmaDestroyBuffer (allocator, stage_lightInfo, mems_lightInfo);
    vmaDestroyBuffer (allocator, lightInfo, mem_lightInfo);
    vmaDestroyBuffer (allocator, stage_materialInfo, mems_materialInfo);
    vmaDestroyBuffer (allocator, materialInfo, mem_materialInfo);
    vmaDestroyBuffer (allocator, stage_index, mems_index);
    vmaDestroyBuffer (allocator, index, mem_index);
    vmaDestroyBuffer (allocator, stage_vertex, mems_vertex);
    vmaDestroyBuffer (allocator, vertex, mem_vertex);
}

void JojoVulkanMesh::drawNode (
    const VkCommandBuffer   cmd,
    const VkPipelineLayout  pipelineLayout,
    const Scene::Node      *node
) const {
    for (const auto &primitive : node->primitives) {
        uint32_t offsets[] = {
            primitive.dynamicMVP      * alignModelTrans,
            primitive.dynamicMaterial * alignMaterialInfo
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


