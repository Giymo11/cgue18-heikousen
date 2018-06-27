#include <iostream>
#include <unordered_map>

#include "jojo_vulkan_utils.hpp"
#include "jojo_engine.hpp"
#include "jojo_level.hpp"
#include "Rendering/DescriptorSets.h"

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
    attrib[2].format = VK_FORMAT_R32G32B32_SFLOAT;
    attrib[2].offset = offsetof (Vertex, uv);

    attrib[3].location = 3;
    attrib[3].binding = 0;
    attrib[3].format = VK_FORMAT_R32G32B32_SFLOAT;
    attrib[3].offset = offsetof (Vertex, light_uv);

    return attrib;
}

JojoLevel *alloc (
    const VmaAllocator     allocator,
    const std::string     &bspName
) {
    VkBufferCreateInfo binfo = {};
    VmaAllocationCreateInfo allocInfo = {};
    auto level = new JojoLevel;

    // Load bsp and count vertices/indices
    level->bsp = BSP::loadBSP ("maps/" + bspName);
    const auto bsp = level->bsp.get ();
    const auto vertexCount = BSP::vertexCount (bsp->header);
    const auto indexCount = bsp->indexCount;
    const auto vertexDataSize = (uint32_t)(sizeof (Vertex) * vertexCount);
    const auto indexDataSize = (uint32_t)(sizeof (uint32_t) * indexCount);

    level->drawQueue.resize (bsp->leafCount);
    level->transparent.resize (bsp->leafCount);

    binfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    binfo.size = vertexDataSize;
    binfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT
        | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    binfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    allocInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    ASSERT_VULKAN (vmaCreateBuffer (
        allocator, &binfo, &allocInfo,
        &level->vertex, &level->vertexMemory, nullptr
    ));

    binfo.size = indexDataSize;
    binfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT
        | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    binfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    allocInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    ASSERT_VULKAN (vmaCreateBuffer (
        allocator, &binfo, &allocInfo,
        &level->index, &level->indexMemory, nullptr
    ));

    binfo.size = indexDataSize;
    binfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    binfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    allocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
    allocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
    allocInfo.requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
    allocInfo.preferredFlags = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        | VK_MEMORY_PROPERTY_HOST_CACHED_BIT;

    ASSERT_VULKAN (vmaCreateBuffer (
        allocator, &binfo, &allocInfo,
        &level->indexStaging, &level->indexStagingMemory,
        &level->indexInfo
    ));

    return level;
}

void free (
    const VmaAllocator     allocator,
    JojoLevel             *level
) {
    auto &shapes = level->collisionShapes;
    auto numShapes = shapes.size ();
    for (int i = 0; i < numShapes; i++)
        delete shapes[i];

    auto &motionStates = level->motionStates;
    auto numMotionStates = motionStates.size ();
    for (int i = 0; i < numMotionStates; i++)
        delete motionStates[i];

    auto &bodies = level->rigidBodies;
    auto numBodies = bodies.size ();
    for (int i = 0; i < numBodies; i++)
        delete bodies[i];

    vmaDestroyBuffer (allocator, level->indexStaging, level->indexStagingMemory);
    vmaDestroyBuffer (allocator, level->index, level->indexMemory);
    vmaDestroyBuffer (allocator, level->vertex, level->vertexMemory);

    delete level;
}

void cmdStageVertexData (
    const VmaAllocator     allocator,
    const JojoLevel       *level,
    const VkCommandBuffer  transferCmd,
    CleanupQueue          *cleanupQueue
) {
    const auto bsp = level->bsp.get ();
    const auto vertexCount = BSP::vertexCount (bsp->header);
    const auto vertexDataSize = (uint32_t)(sizeof (Vertex) * vertexCount);

    VkBuffer staging;
    VmaAllocation stagingMemory;

    VkBufferCreateInfo stagingInfo = {};
    stagingInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    stagingInfo.size = vertexDataSize;
    stagingInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    stagingInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
    allocInfo.requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
    allocInfo.preferredFlags = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    ASSERT_VULKAN (vmaCreateBuffer (
        allocator, &stagingInfo, &allocInfo,
        &staging, &stagingMemory, nullptr
    ));

    Vertex *vertexData = nullptr;
    ASSERT_VULKAN (vmaMapMemory (
        allocator, stagingMemory,
        (void **)&vertexData
    ));

    // Generate vertex data and write it directly to staging buffer
    BSP::fillVertexBuffer (
        bsp->header, bsp->leafs, bsp->leafFaces,
        bsp->faces, bsp->meshVertices, bsp->vertices,
        bsp->lightmapLookup.data (), vertexData
    );
    vmaUnmapMemory (allocator, stagingMemory);


    VkBufferCopy bufferCopy = {};
    bufferCopy.size = vertexDataSize;
    vkCmdCopyBuffer (transferCmd, staging, level->vertex, 1, &bufferCopy);

    // Add staging buffers to cleanup queue
    cleanupQueue->emplace_back (staging, stagingMemory);
}

void cmdBuildAndStageIndicesNaively (
    const VmaAllocator     allocator,
    const VkDevice         device,
    const VkCommandBuffer  transferCmd,
    const vec3            &pos,
    JojoLevel             *level
) {
    const auto bsp = level->bsp.get ();
    const auto indexCount = bsp->indexCount;
    const auto indexDataSize = (uint32_t)(sizeof (uint32_t) * indexCount);
    const auto allocInfo = level->indexInfo;
    auto indexData = (uint32_t *)allocInfo.pMappedData;

    std::unordered_set<int32_t> drawnFaces;
    auto   &drawQueue = level->drawQueue;
    int     leafCount = 0;
    size_t  nextIndex = 0;
    level->indexCount = 0;

    // Find order of leafs to be drawn
    BSP::traverseTreeRecursiveFront (
        bsp->nodes, bsp->leafs, bsp->planes,
        pos, 0, drawQueue.data (), &leafCount
    );

    // Generate indices for picked leafs
    for (int i = 0; i < leafCount; i++) {
        BSP::buildIndicesLeafOpaque (
            &bsp->leafs[drawQueue[i]], bsp->leafFaces,
            bsp->faces, bsp->meshVertices, bsp->textureData,
            indexData, &nextIndex, drawnFaces
        );
    }
    level->indexCount = (uint32_t)nextIndex;

    leafCount = 0;
    level->transparentCount = 0;

    // Find order of leafs to be drawn
    BSP::traverseTreeRecursiveBack (
        bsp->nodes, bsp->leafs, bsp->planes,
        pos, 0, drawQueue.data (), &leafCount
    );

    // Generate indices for picked transparent leafs
    for (int i = 0; i < leafCount; i++) {
        const auto oldNumIndices = nextIndex;

        BSP::buildIndicesLeafTransparent (
            &bsp->leafs[drawQueue[i]], bsp->leafFaces,
            bsp->faces, bsp->meshVertices, bsp->textureData,
            indexData, &nextIndex, drawnFaces
        );

        if (nextIndex > oldNumIndices) {
            auto &trans = level->transparent[level->transparentCount];
            trans.indexCount  = nextIndex - oldNumIndices;
            trans.indexOffset = oldNumIndices;

            level->transparentCount += 1;
        }
    }
    
    // Flush memory range if neccessary
    VkMemoryPropertyFlags memFlags;
    vmaGetMemoryTypeProperties (allocator, allocInfo.memoryType, &memFlags);
    if ((memFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == 0) {
        VkMappedMemoryRange memRange = { VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE };
        memRange.memory = allocInfo.deviceMemory;
        memRange.offset = allocInfo.offset;
        memRange.size = (uint32_t)nextIndex * sizeof(uint32_t);
        vkFlushMappedMemoryRanges (device, 1, &memRange);
    }

    VkBufferCopy bufferCopy = {};
    bufferCopy.size = indexDataSize;
    vkCmdCopyBuffer (
        transferCmd, level->indexStaging,
        level->index, 1, &bufferCopy
    );
}

void cmdLoadAndStageTextures (
    const VmaAllocator     allocator,
    const VkDevice         device,
    const VkCommandBuffer  transferCmd,
    JojoLevel             *level,
    CleanupQueue          *cleanupQueue
) {
    const auto bsp = level->bsp.get ();
    const auto textures = bsp->textures;
    const auto normals = bsp->normals;
    const auto lightmaps = bsp->lightmaps;

    VkBuffer stagingDiffuse;
    VkBuffer stagingNormal;
    VkBuffer stagingLightmaps;

    VmaAllocation memDiffuse;
    VmaAllocation memNormal;
    VmaAllocation memLightmaps;

    Textures::cmdTextureArrayFromList (
        allocator, device, transferCmd, textures,
        512, 512, true, &level->texDiffuse,
        &stagingDiffuse, &memDiffuse
    );

    Textures::cmdTextureArrayFromList (
        allocator, device, transferCmd, normals,
        512, 512, true, &level->texNormal,
        &stagingNormal, &memNormal
    );

    Textures::cmdTextureArrayFromList (
        allocator, device, transferCmd, lightmaps,
        2048, 2048, false, &level->texLightmap,
        &stagingLightmaps, &memLightmaps
    );

    cleanupQueue->emplace_back (stagingDiffuse, memDiffuse);
    cleanupQueue->emplace_back (stagingNormal, memNormal);
    cleanupQueue->emplace_back (stagingLightmaps, memLightmaps);
}

void updateDescriptors (
    const Rendering::DescriptorSets *descriptors,
    const JojoLevel                 *level
) {
    auto diffuse512 = Textures::descriptor (&level->texDiffuse);
    descriptors->update (Rendering::Set::Level, 1, diffuse512);
    descriptors->update (Rendering::Set::Transparent, 1, diffuse512);
    auto lightmap = Textures::descriptor (&level->texLightmap);
    descriptors->update (Rendering::Set::Level, 2, lightmap);
    descriptors->update (Rendering::Set::Transparent, 2, lightmap);
}

void loadRigidBodies (
    JojoLevel               *level
) {
    const auto bsp = level->bsp.get ();

    BSP::buildColliders (
        bsp->header, bsp->leafs, bsp->leafBrushes, bsp->brushes,
        bsp->brushSides, bsp->planes, bsp->textureData,
        &level->collisionShapes, &level->motionStates,
        &level->rigidBodies
    );
}

void addRigidBodies (
    JojoLevel               *level,
    btDiscreteDynamicsWorld *world
) {
    const auto numBodies = level->rigidBodies.size ();
    auto &bodies = level->rigidBodies;
    for (int i = 0; i < numBodies; i++)
        world->addRigidBody (bodies[i]);
}

void removeRigidBodies (
    JojoLevel               *level,
    btDiscreteDynamicsWorld *world
) {
    const btVector3 zeroVector (0, 0, 0);
    const auto      numBodies = level->rigidBodies.size ();
    auto           &bodies = level->rigidBodies;

    btTransform startTransform;
    startTransform.setIdentity ();
    startTransform.setOrigin (btVector3 (0, 0, 0));

    for (int i = 0; i < numBodies; i++) {
        world->removeRigidBody (bodies[i]);

        bodies[i]->clearForces ();
        bodies[i]->setLinearVelocity (zeroVector);
        bodies[i]->setAngularVelocity (zeroVector);
        bodies[i]->setWorldTransform (startTransform);
    }

}

void cmdDraw (
    const VkCommandBuffer    drawCmd,
    const JojoLevel         *level
) {
    vkCmdDrawIndexed (
        drawCmd, level->indexCount,
        1, 0, 0, 0
    );
}

}