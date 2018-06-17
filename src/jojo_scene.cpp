//
// Created by benja on 4/28/2018.
//
#include "jojo_vulkan_data.hpp"

namespace Scene {

void instantiate (
    const btVector3      &position,
    uint32_t              templateIndex,
    InstanceType          type,
    SceneTemplates       *scene,
    Instance             *instance
) {
    btTransform startTransform;
    startTransform.setIdentity ();
    startTransform.setOrigin (position);
    instance->motionState = new btDefaultMotionState (startTransform);

    auto info = btRigidBody::btRigidBodyConstructionInfo (
        0.f,
        instance->motionState,
        scene->templates[templateIndex].shape
    );
    instance->body = new btRigidBody (info);

    auto &nextInstance   = scene->templates[templateIndex].nextInstance;
    instance->templateId = templateIndex;
    instance->instanceId = nextInstance;
    instance->type       = type;
    nextInstance += 1;
}

void cmdDrawInstances (
    const VkCommandBuffer   cmd,
    const VkPipelineLayout  pipelineLayout,
    const JojoVulkanMesh   *data,
    const Template         *templates,
    const Instance         *instances,
    const uint32_t          instanceCount
) {
    VkDeviceSize offsets = 0;
    vkCmdBindVertexBuffers (
        cmd, 0, 1, &data->vertexBuffer,
        &offsets
    );
    vkCmdBindIndexBuffer (
        cmd, data->indexBuffer, 0,
        VK_INDEX_TYPE_UINT32
    );

    for (uint32_t i = 0; i < instanceCount; i++) {
        const auto &inst = instances[i];
        const auto &temp = templates[inst.templateId];

        // Only one instance for now
        if (inst.instanceId > 0)
            continue;

        for (const auto &node : temp.nodes)
            data->drawNode (cmd, pipelineLayout, &node);
    }
}

}

//
//glm::mat4 JojoNode::calculateAbsoluteMatrix() {
//    if (parent != nullptr) {
//        return parent->calculateAbsoluteMatrix() * matrix;
//    }
//    return matrix;
//}
//
//void JojoNode::setRelativeMatrix(const glm::mat4 &newMatrix) {
//    matrix = newMatrix;
//
//    if (!primitives.empty()) {
//        auto &primitive = primitives[0];
//        auto dynOffset = primitive.dynamicOffset;
//
//        if (parent) {
//            root->mvps[dynOffset] = calculateAbsoluteMatrix();
//        }
//    }
//    for (auto *child : children) {
//        child->setParent(this);
//    }
//}
//
//void JojoNode::setParent(JojoNode *newParent) {
//    parent = newParent;
//    setRelativeMatrix(matrix);
//}
//
//

