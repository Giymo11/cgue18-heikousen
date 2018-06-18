//
// Created by benja on 4/28/2018.
//
#include "jojo_physics.hpp"
#include "jojo_vulkan_data.hpp"

namespace Scene {

void instantiate (
    const mat4           &transform,
    uint32_t              templateIndex,
    InstanceType          type,
    SceneTemplates       *scene,
    Instance             *instance
) {
    const auto &templ = scene->templates[templateIndex];

    // Calculate start transform
    JojoVulkanMesh::ModelTransformations modelTrans;
    updateMatrices (
        scene->templates.data (),
        instance, 1, 0, false,
        (uint8_t *)&modelTrans
    );
    modelTrans.model = transform * modelTrans.model;

    btTransform startTransform;
    startTransform.setFromOpenGLMatrix (value_ptr (modelTrans.model));
    instance->motionState = new btDefaultMotionState (startTransform);

    btVector3 localInertia (0, 1, 0);
    btScalar mass (1.0f);
    templ.shape->calculateLocalInertia (mass, localInertia);

    auto info = btRigidBody::btRigidBodyConstructionInfo (
        mass,
        instance->motionState,
        scene->templates[templateIndex].shape,
        localInertia
    );
    instance->body = new btRigidBody (info);
    instance->body->setRestitution (Physics::objectRestitution);
    instance->body->forceActivationState (DISABLE_DEACTIVATION);
    instance->body->setUserPointer ((void *)instance);

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
        cmd, 0, 1, &data->vertex,
        &offsets
    );
    vkCmdBindIndexBuffer (
        cmd, data->index, 0,
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

static void updateNodeMatrices (
    const Node     &node,
    const mat4     &matrix,
    const uint32_t  instanceId,
    const uint32_t  transAlignment,
    uint8_t        *transBuffer
) {
    const auto abs = node.relative * matrix;

    // Only update one instance for now
    if (node.dynamicTrans >= 0 && instanceId == 0) {
        auto trans = (JojoVulkanMesh::ModelTransformations *)(
            transBuffer + transAlignment * node.dynamicTrans
        );

        trans->model = abs;
    }

    for (const auto &child : node.children) {
        updateNodeMatrices (
            child, abs, instanceId,
            transAlignment, transBuffer
        );
    }
}

void updateMatrices (
    const Template *templates,
    const Instance *instances,
    const uint32_t  instanceCount,
    const uint32_t  transAlignment,
    const bool      withPhysics,
    uint8_t        *transBuffer
) {
    for (uint32_t i = 0; i < instanceCount; i++) {
        const auto &inst = instances[i];
        const auto &temp = templates[inst.templateId];

        // --------------------------------------------------------------
        // PHYSICS TRANSFORMATION BEGIN
        // --------------------------------------------------------------

        mat4 physicsMatrix;

        if (withPhysics) {
            btTransform trans;
            inst.body->getMotionState ()->getWorldTransform (trans);
            trans.getOpenGLMatrix (glm::value_ptr (physicsMatrix));
        }

        // --------------------------------------------------------------
        // PHYSICS TRANSFORMATION END
        // --------------------------------------------------------------

        for (const auto &node : templates->nodes) {
            updateNodeMatrices (
                node, physicsMatrix, inst.instanceId,
                transAlignment, transBuffer
            );
        }
    }
}

static void updateNodeNormalMatrices (
    const Node     &node,
    const mat4     &view,
    const uint32_t  instanceId,
    const uint32_t  transAlignment,
    uint8_t        *transBuffer
) {
    // Only update one instance for now
    if (node.dynamicTrans >= 0 && instanceId == 0) {
        auto trans = (JojoVulkanMesh::ModelTransformations *)(
            transBuffer + transAlignment * node.dynamicTrans
            );

        trans->normalMatrix = mat4 (inverseTranspose (mat3 (view * trans->model)));
    }

    for (const auto &child : node.children) {
        updateNodeMatrices (
            child, view, instanceId,
            transAlignment, transBuffer
        );
    }
}

void updateNormalMatrices (
    const Template *templates,
    const Instance *instances,
    const uint32_t  instanceCount,
    const mat4      view,
    const uint32_t  transAlignment,
    uint8_t        *transBuffer
) {
    for (uint32_t i = 0; i < instanceCount; i++) {
        const auto &inst = instances[i];
        const auto &temp = templates[inst.templateId];

        for (const auto &node : templates->nodes) {
            updateNodeNormalMatrices (
                node, view, inst.instanceId,
                transAlignment, transBuffer
            );
        }
    }
}

}
