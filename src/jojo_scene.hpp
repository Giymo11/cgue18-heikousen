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
struct Node;
struct Model;
struct Primitive;
}

// TODO: make sure to create these classes with proper constructors and constant member fields in place

class JojoVertex {
public:
    glm::vec3 pos;
    glm::vec3 color;

    JojoVertex(glm::vec3 pos, glm::vec3 color);


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
    uint32_t firstIndex;
    uint32_t indexCount;
    JojoMaterial *material;

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
    // TODO: vulkan data
public:
    std::vector<JojoNode> children;

    std::vector<uint32_t> indexBuffer;
    std::vector<JojoVertex> vertexBuffer;

    std::vector<JojoMaterial> materials;

};

/**
 * Representing a transformation
 */
class JojoNode {
public:
    //Node* parent;

    JojoScene *root;
    std::vector<JojoNode> children;
    std::string name;

    std::vector<JojoPrimitive> primitives;
    std::vector<JojoLight> lights;
    // TODO: make sure you pass through twice, the first time to get all the lights

    glm::mat4 matrix;
    // position?
    // bullet bounding box info


private:
    void loadMatrix(const tinygltf::Node &gltfNode);

    void loadVertices(const tinygltf::Model &model, const tinygltf::Primitive &primitive);

    uint32_t loadIndices(const tinygltf::Model &model, const tinygltf::Primitive &primitive, const uint32_t vertexStart);

    void loadNode(const tinygltf::Node &gltfNode, const tinygltf::Model &model, std::vector<JojoMaterial> &materials);

public:
    void loadFromGltf(const tinygltf::Model &gltfModel, JojoScene *root);

};




