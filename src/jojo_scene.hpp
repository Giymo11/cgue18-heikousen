//
// Created by benja on 4/27/2018.
//
#pragma once
#include <string>
#include <vector>
#include <debug_trap.h>
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/transform.hpp>
#include <btBulletDynamicsCommon.h>

#define CHECK(x) { if (!x) psnip_trap(); }

class JojoVulkanMesh;

namespace Object {

using namespace glm;

struct Vertex {
    vec3 pos;
    vec3 nml;
    vec2 tex;
};

struct Primitive {
    uint32_t dynamicMVP;
    uint32_t dynamicMaterial;

    uint32_t indexCount;
    uint32_t indexOffset;
    uint32_t vertexOffset;
};

struct TransData {
    mat4 model;
    mat4 view;
    mat4 projection;
};

struct Material {
    uint32_t something;
};

enum CollisionShapeType : uint8_t {
    Box,
    Convex
};

struct CollisionShapeInfo {
    CollisionShapeType type;
    std::string        shapeModel;
};

};

namespace Scene {

using namespace glm;

typedef std::vector<std::pair<
    const std::string,
    const Object::CollisionShapeInfo
>> TemplateInfo;

struct Node {
    mat4                           baseMatrix;
    mat4                           matrix;

    std::vector<Node>              children;
    std::vector<Object::Primitive> primitives;
};

struct Template {
    std::vector<Node>  nodes;
    btCollisionShape  *shape;
    uint32_t           nextInstance;
};

enum InstanceType : uint8_t {
    PlayerInstance,
    NonLethalInstance,
    LethalInstance,
    PortalInstance
};

struct Instance {
    uint32_t              instanceId;
    uint32_t              templateId;

    InstanceType          type;
    btRigidBody          *body;
    btDefaultMotionState *motionState;
};

struct SceneTemplates {
    std::vector<Template>         templates;
    std::vector<Instance>         instances;

    uint32_t                      numInstances;
    uint32_t                      nextDynTrans;
    uint32_t                      nextDynMaterial;

    std::vector<uint32_t>         indices;
    std::vector<Object::Vertex>   vertices;
    std::vector<Object::Material> materials;
    std::vector<std::string>      textures;
};

void loadTemplate (
    const std::string                 &modelName,
    const Object::CollisionShapeInfo  &collisionInfo,
    uint32_t                           templateIndex,
    SceneTemplates                    *templates
);

void instantiate (
    const btVector3      &position,
    uint32_t              templateIndex,
    InstanceType          type,
    SceneTemplates       *scene,
    Instance             *instance
);

void cmdDrawInstances (
    const VkCommandBuffer   cmd,
    const VkPipelineLayout  pipelineLayout,
    const JojoVulkanMesh   *data,
    const Template         *templates,
    const Instance         *instances,
    uint32_t                instanceCount
);

}

