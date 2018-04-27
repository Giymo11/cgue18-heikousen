//
// Created by benja on 4/27/2018.
//

#pragma once

#include <glm/glm.hpp>
#include <string>
#include <vector>




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
    JojoMaterial &material;

    // material info
    // index info
    // vertex info
    // normal map info
    // texture info
    // TODO: alpha blending
};

/**
 * Representing a transformation
 */
class JojoNode {
public:
    //Node* parent;
    std::vector<JojoNode> children;
    std::string name;

    std::vector<JojoPrimitive> primitives;
    std::vector<JojoLight> lights;
    // TODO: make sure you pass through twice, the first time to get all the lights

    glm::mat4 matrix;
    // position?
    // bullet bounding box info
};

/**
 * Representing the whole scene
 */
class JojoScene {
    // TODO: vulkan data
public:
    std::vector<JojoNode> children;




};


