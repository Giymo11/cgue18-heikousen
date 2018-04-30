//
// Created by benja on 4/27/2018.
//

#pragma once


#include <string>
#include <vector>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#include <glm/glm.hpp>

// tinygltf forward declarations, makes header handling easier
namespace tinygltf {
    class Node;

    class Model;

    struct Primitive;
}

// TODO: make sure to create these classes with proper constructors and constant member fields in place

class JojoVertex {
public:
    glm::vec3 pos;
    glm::vec3 normal;
    glm::vec2 uv;

    JojoVertex(glm::vec3 pos, glm::vec3 normal, glm::vec2 uv);


};


class JojoMaterial {

};

class JojoLight {
public:
    glm::vec3 color;
    // etc
};

/**
 * representing a mesh with only one material
 */
class JojoPrimitive {
public:
    uint32_t indexOffset;
    uint32_t indexCount;
    uint32_t dynamicOffset;
    JojoMaterial *material;
    // TODO: make the material a offset too

    // material info
    // index info
    // vertex info
    // normal map info
    // texture info
    // TODO: alpha blending
};


class JojoNode;


/**
 * Representing the whole scene
 */
class JojoScene {
public:
    std::vector<JojoNode*> children;

    std::vector<uint32_t> indices;
    std::vector<JojoVertex> vertices;
    std::vector<glm::mat4> mvps;

    std::vector<JojoMaterial> materials;

};

/**
 * Representing a transformation
 */
class JojoNode {

private:
    glm::mat4 matrix;

public:
    JojoNode *parent = nullptr;
    JojoScene *root = nullptr;

    std::vector<JojoNode*> children;
    std::string name;

    std::vector<JojoPrimitive> primitives;
    std::vector<JojoLight> lights;
    // TODO: make sure you pass through twice, the first time to get all the lights


    // position?
    // bullet bounding box info

private:
    void loadMatrix(const tinygltf::Node &gltfNode);

    void loadVertices(const tinygltf::Model &model, const tinygltf::Primitive &primitive);

    uint32_t
    loadIndices(const tinygltf::Model &model, const tinygltf::Primitive &primitive, const uint32_t vertexStart);

    void loadNode(const tinygltf::Node &gltfNode,
                  const tinygltf::Model &model,
                  std::vector<JojoMaterial> &materials);

public:
    const glm::mat4 &getRelativeMatrix() const;

    void setRelativeMatrix(const glm::mat4 &newMatrix);

    void setParent(JojoNode *newParent);

    void loadFromGltf(const tinygltf::Model &gltfModel, JojoScene *root);

    glm::mat4 calculateAbsoluteMatrix();

    glm::mat4 startTransform;
};




