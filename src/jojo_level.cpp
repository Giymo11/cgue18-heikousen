#include "jojo_vulkan_utils.hpp"
#include "jojo_engine.hpp"
#include "jojo_level.hpp"

namespace Level {

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
    const auto indexCount = BSP::indexCount (bsp->header);
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
    const auto indexCount = BSP::indexCount (bsp->header);
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

}