//
// Created by benja on 4/28/2018.
//

#include <iostream>

#include <tiny_gltf.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "jojo_scene.hpp"
#include "jojo_vulkan_utils.hpp"


JojoVertex::JojoVertex(glm::vec3 pos, glm::vec3 color)
        : pos(pos), color(color) {}

void JojoNode::loadFromGltf(const tinygltf::Model &gltfModel, JojoScene *root) {
    // loadImages(gltfModel, device, transferQueue);
    // loadMaterials(gltfModel, device, transferQueue);
    std::vector<JojoMaterial> materials;

    const tinygltf::Scene &scene = gltfModel.scenes[gltfModel.defaultScene];

    // TODO: load materials and textures into root

    this->root = root;
    this->name = scene.name;

    for (auto &node : scene.nodes) {
        this->loadNode(gltfModel.nodes[node], gltfModel, materials);
    }

    std::cout << "loaded " << this->children.size() << " own nodes" << std::endl;
}


void
JojoNode::loadNode(const tinygltf::Node &gltfNode, const tinygltf::Model &model, std::vector<JojoMaterial> &materials) {

    JojoNode jojoNode;
    jojoNode.root = this->root;
    jojoNode.name = gltfNode.name;
    jojoNode.loadMatrix(gltfNode);


    // Node contains mesh data
    if (gltfNode.mesh > -1) {
        const tinygltf::Mesh mesh = model.meshes[gltfNode.mesh];
        jojoNode.name += " - " + mesh.name;

        for (const auto &primitive : mesh.primitives) {
            if (primitive.indices < 0) {
                continue;
            }
            const uint32_t indexStart = static_cast<uint32_t>(this->root->indexBuffer.size());
            const uint32_t vertexStart = static_cast<uint32_t>(this->root->vertexBuffer.size());
            // Vertices
            jojoNode.loadVertices(model, primitive);
            // Indices
            const uint32_t indexCount = jojoNode.loadIndices(model, primitive, vertexStart);
            if (indexCount <= 0) {
                std::cout << "Index count was 0" << std::endl;
                continue;
            }

            JojoPrimitive jojoPrimitive;
            jojoPrimitive.firstIndex = indexStart;
            jojoPrimitive.indexCount = indexCount;
            jojoPrimitive.material = nullptr;    // TODO from materials[primitive.material]}

            jojoNode.primitives.push_back(jojoPrimitive);
        }
    }


    if (!gltfNode.children.empty()) {
        for (auto i = 0; i < gltfNode.children.size(); i++) {
            jojoNode.loadNode(model.nodes[gltfNode.children[i]], model, materials);
        }
    }

    this->children.push_back(jojoNode);
}

uint32_t JojoNode::loadIndices(const tinygltf::Model &model,
                               const tinygltf::Primitive &primitive,
                               const uint32_t vertexStart) {

    const tinygltf::Accessor &accessor = model.accessors[primitive.indices];
    const tinygltf::BufferView &bufferView = model.bufferViews[accessor.bufferView];
    const tinygltf::Buffer &buffer = model.buffers[bufferView.buffer];

    const uint32_t indexCount = static_cast<uint32_t>(accessor.count);

    switch (accessor.componentType) {
        case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT: {
            uint32_t *buf = new uint32_t[accessor.count];
            memcpy(buf, &buffer.data[accessor.byteOffset + bufferView.byteOffset],
                   accessor.count * sizeof(uint32_t));
            for (size_t index = 0; index < accessor.count; index++) {
                this->root->indexBuffer.push_back(buf[index] + vertexStart);
            }
            break;
        }
        case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT: {
            uint16_t *buf = new uint16_t[accessor.count];
            memcpy(buf, &buffer.data[accessor.byteOffset + bufferView.byteOffset],
                   accessor.count * sizeof(uint16_t));
            for (size_t index = 0; index < accessor.count; index++) {
                this->root->indexBuffer.push_back(buf[index] + vertexStart);
            }
            break;
        }
        case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE: {
            uint8_t *buf = new uint8_t[accessor.count];
            memcpy(buf, &buffer.data[accessor.byteOffset + bufferView.byteOffset],
                   accessor.count * sizeof(uint8_t));
            for (size_t index = 0; index < accessor.count; index++) {
                this->root->indexBuffer.push_back(buf[index] + vertexStart);
            }
            break;
        }
        default:
            std::cerr << "Index component type " << accessor.componentType << " not supported!" << std::endl;
            psnip_trap();
            return 0;
    }

    return indexCount;
}


void JojoNode::loadVertices(const tinygltf::Model &model,
                            const tinygltf::Primitive &primitive) {
    const float *bufferPos = nullptr;
    const float *bufferNormals = nullptr;
    const float *bufferTexCoords = nullptr;

    // Position attribute is required
    assert(primitive.attributes.find("POSITION") != primitive.attributes.end());

    const tinygltf::Accessor &posAccessor = model.accessors[primitive.attributes.find("POSITION")->second];
    const tinygltf::BufferView &posView = model.bufferViews[posAccessor.bufferView];
    bufferPos = reinterpret_cast<const float *>(&(model.buffers[posView.buffer].data[posAccessor.byteOffset +
                                                                                     posView.byteOffset]));

    if (primitive.attributes.find("NORMAL") != primitive.attributes.end()) {
        const tinygltf::Accessor &normAccessor = model.accessors[primitive.attributes.find("NORMAL")->second];
        const tinygltf::BufferView &normView = model.bufferViews[normAccessor.bufferView];
        bufferNormals = reinterpret_cast<const float *>(&(model.buffers[normView.buffer].data[
                normAccessor.byteOffset + normView.byteOffset]));
    }

    if (primitive.attributes.find("TEXCOORD_0") != primitive.attributes.end()) {
        const tinygltf::Accessor &uvAccessor = model.accessors[primitive.attributes.find("TEXCOORD_0")->second];
        const tinygltf::BufferView &uvView = model.bufferViews[uvAccessor.bufferView];
        bufferTexCoords = reinterpret_cast<const float *>(&(model.buffers[uvView.buffer].data[
                uvAccessor.byteOffset + uvView.byteOffset]));
    }

    for (size_t v = 0; v < posAccessor.count; v++) {

        //auto pos = localNodeMatrix * glm::vec4(glm::make_vec3(&bufferPos[v * 3]), 1.0f);
        auto pos = glm::make_vec3(&bufferPos[v * 3]);

        //vert.normal = glm::normalize(glm::mat3(localNodeMatrix) * glm::vec3(bufferNormals ? glm::make_vec3(&bufferNormals[v * 3]) : glm::vec3(0.0f)));
        //vert.uv = bufferTexCoords ? glm::make_vec2(&bufferTexCoords[v * 2]) : glm::vec3(0.0f);
        // Vulkan coordinate system
        pos.y *= -1.0f;
        //vert.normal.y *= -1.0f;

        glm::vec3 color(1.0, 1.0, 1.0);

        JojoVertex vert{pos, color};
        this->root->vertexBuffer.push_back(vert);
    }
}

void JojoNode::loadMatrix(const tinygltf::Node &gltfNode) {
    // Generate local node matrix
    glm::vec3 translation = glm::vec3(0.0f);
    if (gltfNode.translation.size() == 3) {
        translation = glm::make_vec3(gltfNode.translation.data());
    }
    glm::mat4 rotation = glm::mat4(1.0f);
    if (gltfNode.rotation.size() == 4) {
        glm::quat q = glm::make_quat(gltfNode.rotation.data());
        rotation = glm::mat4(q);
    }
    glm::vec3 scale = glm::vec3(1.0f);
    if (gltfNode.scale.size() == 3) {
        scale = glm::make_vec3(gltfNode.scale.data());
    }
    glm::mat4 localNodeMatrix = glm::mat4(1.0f);
    if (gltfNode.matrix.size() == 16) {
        localNodeMatrix = glm::make_mat4x4(gltfNode.matrix.data());
    } else {
        // T * R * S
        localNodeMatrix =
                glm::translate(glm::mat4(1.0f), translation) * rotation * glm::scale(glm::mat4(1.0f), scale);
    }
    // localNodeMatrix = parentMatrix * localNodeMatrix;
    this->matrix = localNodeMatrix;
}