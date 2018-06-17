#pragma once
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <btBulletDynamicsCommon.h>

#include "jojo_bsp.hpp"
#include "jojo_vulkan_textures.hpp"

class JojoEngine;
namespace Rendering {
class DescriptorSets;
}

namespace Level {

using namespace glm;

VkVertexInputBindingDescription                vertexInputBinding ();
std::vector<VkVertexInputAttributeDescription> vertexAttributes ();

typedef std::vector<std::pair<VkBuffer, VmaAllocation>> CleanupQueue;

struct Vertex {
    vec3 pos;
    vec3 uv;
    vec3 light_uv;
    vec3 normal;
};

struct JojoLevel {
    std::unique_ptr<BSP::BSPData> bsp;

    VkBuffer          vertex;
    VkBuffer          index;
    VkBuffer          indexStaging;
    VmaAllocation     vertexMemory;
    VmaAllocation     indexMemory;
    VmaAllocation     indexStagingMemory;
    VmaAllocationInfo indexInfo;

    Textures::Texture texDiffuse;
    Textures::Texture texNormal;
    Textures::Texture texLightmap;

    VkDescriptorSet   descriptorSet;

    btAlignedObjectArray<btCollisionShape *>     collisionShapes;
    btAlignedObjectArray<btDefaultMotionState *> motionStates;
    btAlignedObjectArray<btRigidBody *>          rigidBodies;
};

JojoLevel *alloc (
    const VmaAllocator     allocator,
    const std::string     &bsp
);

void free (
    const VmaAllocator     allocator,
    JojoLevel             *level
);

void cmdStageVertexData (
    const VmaAllocator     allocator,
    const JojoLevel       *level,
    const VkCommandBuffer  transferCmd,
    CleanupQueue          *cleanupQueue
);

void cmdBuildAndStageIndicesNaively (
    const VmaAllocator     allocator,
    const VkDevice         device,
    const JojoLevel       *level,
    const VkCommandBuffer  transferCmd
);

void cmdLoadAndStageTextures (
    const VmaAllocator     allocator,
    const VkDevice         device,
    const VkCommandBuffer  transferCmd,
    JojoLevel             *level,
    CleanupQueue          *cleanupQueue
);

void updateDescriptors (
    const Rendering::DescriptorSets *descriptors,
    const JojoLevel                 *level
);

void loadRigidBodies (
    JojoLevel               *level
);

void addRigidBodies (
    JojoLevel               *level,
    btDiscreteDynamicsWorld *world
);

void removeRigidBodies (
    JojoLevel               *level,
    btDiscreteDynamicsWorld *world
);

}