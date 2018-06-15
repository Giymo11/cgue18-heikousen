#include <iostream>
#include <unordered_map>

#include "jojo_vulkan_utils.hpp"
#include "jojo_engine.hpp"
#include "jojo_level.hpp"

namespace Level {

VkVertexInputBindingDescription vertexInputBinding () {
    VkVertexInputBindingDescription binding;

    binding.binding = 0;
    binding.stride = sizeof (Vertex);
    binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    return binding;
}

std::vector<VkVertexInputAttributeDescription> vertexAttributes () {
    std::vector<VkVertexInputAttributeDescription> attrib (4);
    attrib[0].location = 0;
    attrib[0].binding = 0;
    attrib[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    attrib[0].offset = offsetof (Vertex, pos);

    attrib[1].location = 1;
    attrib[1].binding = 0;
    attrib[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    attrib[1].offset = offsetof (Vertex, normal);

    attrib[2].location = 2;
    attrib[2].binding = 0;
    attrib[2].format = VK_FORMAT_R32G32_SFLOAT;
    attrib[2].offset = offsetof (Vertex, uv);

    attrib[3].location = 3;
    attrib[3].binding = 0;
    attrib[3].format = VK_FORMAT_R32G32_SFLOAT;
    attrib[3].offset = offsetof (Vertex, light_uv);

    return attrib;
}

JojoLevel *alloc (
    const JojoEngine  *engine,
    const std::string &bspName
) {
    const auto device = engine->device;
    const auto chosenDevice = engine->chosenDevice;
    auto level = new JojoLevel;

    // Load bsp and count vertices/indices
    level->bsp = BSP::loadBSP (bspName);
    const auto bsp = level->bsp.get ();
    const auto vertexCount = BSP::vertexCount (bsp->header);
    const auto indexCount = bsp->indexCount;
    const auto vertexDataSize = (uint32_t)(sizeof (Vertex) * vertexCount);
    const auto indexDataSize = (uint32_t)(sizeof (uint32_t) * indexCount);

    createBuffer (
        device, chosenDevice, vertexDataSize,
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        &level->vertex, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        &level->vertexMemory
    );
    createBuffer (
        device, chosenDevice, indexDataSize,
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        &level->index, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        &level->indexMemory
    );
    createBuffer (
        device, chosenDevice, indexDataSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT, &level->indexStaging,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        &level->indexStagingMemory
    );

    return level;
}

void free (
    const JojoEngine *engine,
    JojoLevel        *level
) {
    const auto device = engine->device;

    vkFreeMemory (device, level->indexStagingMemory, nullptr);
    vkFreeMemory (device, level->indexMemory, nullptr);
    vkFreeMemory (device, level->vertexMemory, nullptr);

    vkDestroyBuffer (device, level->indexStaging, nullptr);
    vkDestroyBuffer (device, level->index, nullptr);
    vkDestroyBuffer (device, level->vertex, nullptr);

    delete level;
}

void stageVertexData (
    const JojoEngine      *engine,
    const JojoLevel       *level,
    const VkCommandBuffer  transferCmd
) {
    const auto device = engine->device;
    const auto chosenDevice = engine->chosenDevice;
    const auto bsp = level->bsp.get ();
    const auto vertexCount = BSP::vertexCount (bsp->header);
    const auto vertexDataSize = (uint32_t)(sizeof (Vertex) * vertexCount);

    // TODO: actually use a transfer queue
    const auto transferQueue = engine->queue;

    VkBuffer staging;
    VkDeviceMemory stagingMemory;
    createBuffer (
        device, chosenDevice, vertexDataSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT, &staging,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        &stagingMemory
    );

    Vertex *vertexData = nullptr;
    VkResult result = vkMapMemory (
        device, stagingMemory, 0, vertexDataSize, 0,
        (void **)(&vertexData)
    );
    ASSERT_VULKAN (result);

    // Generate vertex data and write it directly to staging buffer
    BSP::fillVertexBuffer (bsp->vertices, vertexCount, vertexData);
    vkUnmapMemory (device, stagingMemory);

    VkBufferCopy bufferCopy = {};
    bufferCopy.size = vertexDataSize;
    VkCommandBufferBeginInfo cmdBegin = {};
    cmdBegin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cmdBegin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    result = vkBeginCommandBuffer (transferCmd, &cmdBegin);
    ASSERT_VULKAN (result);
    vkCmdCopyBuffer (transferCmd, staging, level->vertex, 1, &bufferCopy);
    result = vkEndCommandBuffer (transferCmd);
    ASSERT_VULKAN (result);

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &transferCmd;

    // TODO: Maybe batch all of this together
    result = vkQueueSubmit (transferQueue, 1, &submitInfo, VK_NULL_HANDLE);
    ASSERT_VULKAN (result)
    result = vkQueueWaitIdle (transferQueue);
    ASSERT_VULKAN (result)

    vkFreeMemory    (device, stagingMemory, nullptr);
    vkDestroyBuffer (device, staging, nullptr);
}

void cmdBuildAndStageIndicesNaively (
    const VkDevice         device,
    const VkQueue          transferQueue,
    const JojoLevel       *level,
    const VkCommandBuffer  transferCmd
) {
    const auto bsp = level->bsp.get ();
    const auto indexCount = bsp->indexCount;
    const auto indexDataSize = (uint32_t)(sizeof (uint32_t) * indexCount);

    uint32_t *indexData = nullptr;
    VkResult result = vkMapMemory (
        device, level->indexStagingMemory, 0, indexDataSize, 0,
        (void **)(&indexData)
    );
    ASSERT_VULKAN (result);

    // Traverse BSP, generate indices and write them directly to staging buffer
    BSP::buildIndicesNaive (
        bsp->nodes, bsp->leafs, bsp->leafFaces, 
        bsp->faces, bsp->meshVertices, indexData
    );
    vkUnmapMemory (device, level->indexStagingMemory);

    VkBufferCopy bufferCopy = {};
    bufferCopy.size = indexDataSize;
    vkCmdCopyBuffer (transferCmd, level->indexStaging, level->index, 1, &bufferCopy);
}

void loadAndStageTextures (
    const JojoLevel       *level
) {
    const auto bsp = level->bsp.get ();
    const auto textures = bsp->textures;
    
    std::cout << "=========================================\n";
    std::cout << "= LEVEL TEXTURES BEGIN                  =\n";
    std::cout << "=========================================\n";

    for (auto i = 0; i < textures.size(); i++)
        std::cout << i << " " << textures[i] << "\n";

    std::cout << "=========================================\n";
    std::cout << "= LEVEL TEXTURES END                    =\n";
    std::cout << "=========================================\n";

    std::cout << "=========================================\n";
    std::cout << "= FACE TEXTURES BEGIN                   =\n";
    std::cout << "=========================================\n";

    const auto header = bsp->header;
    const auto leafs = bsp->leafs;
    const auto leafFaces = bsp->leafFaces;
    const auto faces = bsp->faces;
    const auto meshverts = bsp->meshVertices;
    const auto leafBytes = (const uint8_t *)leafs + header->direntries[BSP::Leafs].length;
    const auto leafEnd = (const BSP::Leaf *)leafBytes;
    std::unordered_map<int, int> vertexTextures;

    for (auto leaf = &leafs[0]; leaf != leafEnd; ++leaf) {
        const auto leafFaceBegin = leafFaces + leaf->leafface;
        const auto leafFaceEnd = leafFaceBegin + leaf->n_leaffaces;

        for (auto lface = leafFaceBegin; lface != leafFaceEnd; ++lface) {
            const auto face = faces[lface->face];
            /*std::cout << "FACE " << lface->face << " TEXTURE " << face.texture << " LIGHTMAP " << face.lm_index << "\n";*/

            for (auto v = 0; v < face.n_meshverts; v++) {
                const auto index = meshverts[face.meshvert + v].vertex + face.vertex;
                const auto vtex = vertexTextures.find (index);
                if (vtex != vertexTextures.end () && vtex->second != face.texture)
                    std::cout << "TEXTURE CONFLICT VERTEX " << vtex->first << "\n";
                else
                    vertexTextures[index] = face.texture;
            }
        }
    }

    std::cout << "=========================================\n";
    std::cout << "= FACE TEXTURES END                     =\n";
    std::cout << "=========================================\n";
}

}