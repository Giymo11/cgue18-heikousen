#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_NO_STB_IMAGE_WRITE
#define STB_IMAGE_IMPLEMENTATION
#define _SILENCE_CXX17_OLD_ALLOCATOR_MEMBERS_DEPRECATION_WARNING
#include <tiny_gltf.h>
#include <LinearMath/btVector3.h>
#include <LinearMath/btAlignedObjectArray.h>

#include "jojo_scene.hpp"

namespace Object {

using namespace glm;

static void loadMesh (
    const tinygltf::Mesh         &mesh,
    const tinygltf::Accessor     *accessors,
    const tinygltf::BufferView   *views,
    const tinygltf::Buffer       *buffers,
    const uint32_t                dynamicMVP,
    std::vector<Vertex>          *vertices,
    std::vector<uint32_t>        *indices,
    std::vector<Primitive>       *primitives,
    vec3                         *minExtent,
    vec3                         *maxExtent
) {
    primitives->reserve (primitives->size () + mesh.primitives.size ());

    for (const auto &p : mesh.primitives) {
        Primitive primitive = {};

        if (p.indices < 0)
            continue;

        // --------------------------------------------------------------
        // LOAD VERTICES START
        // --------------------------------------------------------------
        {
            const uint32_t currentNum = (uint32_t)vertices->size ();
            const vec3    *pos        = nullptr;
            const vec3    *nml        = nullptr;
            const vec2    *tex        = nullptr;
            uint32_t       num        = 0;

            {
                const auto &posIt = p.attributes.find ("POSITION");
                const auto &nmlIt = p.attributes.find ("NORMAL");
                const auto &texIt = p.attributes.find ("TEXCOORD_0");
                const auto &end = p.attributes.end ();
                if (posIt == end || nmlIt == end || texIt == end)
                    continue;

                const auto &posAccessor = accessors[posIt->second];
                const auto &posView = views[posAccessor.bufferView];
                const auto &nmlAccessor = accessors[nmlIt->second];
                const auto &nmlView = views[nmlAccessor.bufferView];
                const auto &texAccessor = accessors[texIt->second];
                const auto &texView = views[texAccessor.bufferView];

                pos = (const vec3 *)(
                    buffers[posView.buffer].data.data ()
                    + posAccessor.byteOffset
                    + posView.byteOffset
                    );
                nml = (const vec3 *)(
                    buffers[nmlView.buffer].data.data ()
                    + nmlAccessor.byteOffset
                    + nmlView.byteOffset
                    );
                tex = (const vec2 *)(
                    buffers[texView.buffer].data.data ()
                    + texAccessor.byteOffset
                    + texView.byteOffset
                    );

                num = (uint32_t)posAccessor.count;
            }

            vertices->resize (currentNum + num);
            for (uint32_t i = 0; i < num; ++i) {
                auto &vert = vertices->data()[i + currentNum];

                vert.pos = pos[i];
                vert.nml = nml[i];
                vert.tex = tex[i];

                vert.pos.y = -vert.pos.y;
                vert.nml.y = -vert.nml.y;

                *minExtent = min (vert.pos, *minExtent);
                *maxExtent = max (vert.pos, *maxExtent);
            }

            primitive.vertexOffset = currentNum;
        }

        // --------------------------------------------------------------
        // LOAD VERTICES END
        // --------------------------------------------------------------

        // --------------------------------------------------------------
        // LOAD INDICES START
        // --------------------------------------------------------------

        {
            const uint32_t  currentNum = (uint32_t)indices->size ();
            const void     *ind        = nullptr;
            int             type       = 0;
            uint32_t        num        = 0;

            {
                const auto &acc  = accessors[p.indices];
                const auto &view = views[acc.bufferView];

                ind = (const void *)(
                    buffers[view.buffer].data.data ()
                    + acc.byteOffset
                    + view.byteOffset
                );

                type = acc.componentType;
                num  = (uint32_t)acc.count;
            }

            indices->resize (currentNum + num);
            switch (type) {
            case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT:
            {
                const uint32_t *buff = (const uint32_t *)ind;
                std::copy (buff, buff + num, indices->data () + currentNum);
            }
                break;
            case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT:
            {
                const auto buff = (const uint16_t *)ind;
                auto outBuff = indices->data () + currentNum;

                for (uint32_t i = 0; i < num; ++i)
                    outBuff[i] = buff[i];
            }
                break;
            case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE:
            {
                const auto buff = (const uint8_t *)ind;
                auto outBuff = indices->data () + currentNum;

                for (uint32_t i = 0; i < num; ++i)
                    outBuff[i] = buff[i];
            }
                break;
            default:
                CHECK (false);
            }

            primitive.indexCount  = num;
            primitive.indexOffset = currentNum;
        }

        // --------------------------------------------------------------
        // LOAD INDICES END
        // --------------------------------------------------------------
        
        primitive.dynamicMaterial = 0;
        primitive.dynamicMVP      = dynamicMVP;
        primitives->emplace_back (std::move (primitive));
    }
}

static void loadNode (
    const tinygltf::Node         *nodes,
    const tinygltf::Mesh         *meshes,
    const tinygltf::Accessor     *accessors,
    const tinygltf::BufferView   *views,
    const tinygltf::Buffer       *buffers,
    const uint32_t                currentDynamicMVP,
    const uint32_t                currentDynamicMaterial,
    const int32_t                 currentNode,
    std::vector<Vertex>          *vertices,
    std::vector<uint32_t>        *indices,
    uint32_t                     *nextDynamicMVP,
    uint32_t                     *nextDynamicMaterial,
    vec3                         *minExtent,
    vec3                         *maxExtent,
    Scene::Node                  *sceneNode
) {
    const auto &node = nodes[currentNode];

    // --------------------------------------------------------------
    // INITIALIZE MATRIX START
    // --------------------------------------------------------------

    vec3 translation;
    vec3 scale (1.0f);
    mat4 rotation;
    mat4 local;

    if (node.translation.size () == 3)
        translation = make_vec3 (node.translation.data ());
    if (node.scale.size () == 3)
        scale = make_vec3 (node.scale.data ());
    if (node.rotation.size () == 4) {
        quat q = make_quat (node.rotation.data ());
        rotation = mat4 (q);
    }
    if (node.matrix.size () == 16) {
        sceneNode->relative = make_mat4 (node.matrix.data ());
    } else {
        sceneNode->relative = glm::translate (translation)
            * rotation
            * glm::scale (scale);
    }

    // --------------------------------------------------------------
    // INITIALIZE MATRIX END
    // --------------------------------------------------------------

    if (node.mesh > -1) {
        loadMesh (
            meshes[node.mesh], accessors, views, buffers,
            currentDynamicMVP, vertices, indices,
            &sceneNode->primitives, minExtent, maxExtent
        );
  
        *nextDynamicMVP += 1;
        sceneNode->dynamicTrans = currentDynamicMVP;
    } else {
        sceneNode->dynamicTrans = -1;
    }

    // --------------------------------------------------------------
    // PARSE SCENE GRAPH START
    // --------------------------------------------------------------

    {
        const auto nodeCount = node.children.size ();

        sceneNode->children.resize (nodeCount);
        for (auto n = 0; n < nodeCount; ++n) {
            loadNode (
                nodes, meshes, accessors, views, buffers,
                *nextDynamicMVP, currentDynamicMaterial,
                node.children[n], vertices, indices,
                nextDynamicMVP, nextDynamicMaterial,
                minExtent, maxExtent,
                &sceneNode->children[n]
            );
        }
    }

    // --------------------------------------------------------------
    // PARSE SCENE GRAPH END
    // --------------------------------------------------------------
}

static void loadTemplateFromGLB (
    const std::string            &modelName,
    const uint32_t                currentDynamicMVP,
    const uint32_t                currentDynamicMaterial,
    std::vector<Vertex>          *vertices,
    std::vector<uint32_t>        *indices,
    uint32_t                     *nextDynamicMVP,
    uint32_t                     *nextDynamicMaterial,
    Scene::Template              *templ
) {
    tinygltf::Model model;
    tinygltf::TinyGLTF loader;

    templ->nextInstance = 0;

    // --------------------------------------------------------------
    // LOAD BINARY START
    // --------------------------------------------------------------

    {
        auto loaded = loader.LoadBinaryFromFile (
            &model, nullptr,
            "models/" + modelName + ".glb"
        );
        CHECK (loaded);
    }

    // --------------------------------------------------------------
    // LOAD BINARY END
    // --------------------------------------------------------------

    // --------------------------------------------------------------
    // PARSE SCENE GRAPH START
    // --------------------------------------------------------------

    {
        const auto  nodes     = model.nodes.data ();
        const auto  meshes    = model.meshes.data ();
        const auto  accessors = model.accessors.data ();
        const auto  views     = model.bufferViews.data ();
        const auto  buffers   = model.buffers.data ();
        const auto &scene     = model.scenes[model.defaultScene];
        const auto  nodeCount = scene.nodes.size ();

        templ->nodes.resize (nodeCount);
        for (auto n = 0; n < nodeCount; ++n) {
            loadNode (
                nodes, meshes, accessors, views, buffers,
                currentDynamicMVP, currentDynamicMaterial,
                scene.nodes[n], vertices, indices,
                nextDynamicMVP, nextDynamicMaterial,
                &templ->minExtent, &templ->maxExtent,
                &templ->nodes[n]
            );
        }
    }

    // --------------------------------------------------------------
    // PARSE SCENE GRAPH END
    // --------------------------------------------------------------
}

}

namespace Scene {

void loadTemplate (
    const std::string                &modelName,
    const Object::CollisionShapeInfo &collisionInfo,
    const uint32_t                    templateIndex,
    Scene::SceneTemplates            *templates
) {
    // --------------------------------------------------------------
    // LOAD MODEL START
    // --------------------------------------------------------------

    loadTemplateFromGLB (
        modelName, templates->nextDynTrans,
        templates->nextDynMaterial, &templates->vertices,
        &templates->indices, &templates->nextDynTrans,
        &templates->nextDynMaterial,
        &templates->templates[templateIndex]
    );

    // --------------------------------------------------------------
    // LOAD MODEL END
    // --------------------------------------------------------------

    // --------------------------------------------------------------
    // INITIALIZE COLLISION SHAPE START
    // --------------------------------------------------------------

    switch (collisionInfo.type) {
    case Object::Box:
    {
       auto &temp = templates->templates[templateIndex];
       vec3 extent = (temp.maxExtent - temp.minExtent) * 0.5f;
        temp.shape = new btBoxShape (btVector3(
            extent.x,
            extent.y,
            extent.z
        ));
    }
        break;
    case Object::Convex:
    default:
        CHECK (false);
        break;
    }

    // --------------------------------------------------------------
    // INITIALIZE COLLISION SHAPE END
    // --------------------------------------------------------------
}

}