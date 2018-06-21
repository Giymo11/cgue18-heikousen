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
    const uint32_t                materialOffset,
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
        
        primitive.dynamicMaterial = materialOffset + p.material;
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
    const uint32_t                materialOffset,
    const int32_t                 currentNode,
    std::vector<Vertex>          *vertices,
    std::vector<uint32_t>        *indices,
    uint32_t                     *nextDynamicMVP,
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
            currentDynamicMVP, materialOffset, vertices, indices,
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
                *nextDynamicMVP, materialOffset,
                node.children[n], vertices, indices,
                nextDynamicMVP, minExtent,
                maxExtent, &sceneNode->children[n]
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
    std::vector<Vertex>          *vertices,
    std::vector<uint32_t>        *indices,
    uint32_t                     *nextDynamicMVP,
    Scene::Template              *templ,
    Scene::Scene        *scene
) {
    const auto dynMatBase = (uint32_t)scene->materials.size ();
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
                currentDynamicMVP, dynMatBase,
                scene.nodes[n], vertices, indices,
                nextDynamicMVP, &templ->minExtent,
                &templ->maxExtent, &templ->nodes[n]
            );
        }
    }

    // --------------------------------------------------------------
    // PARSE SCENE GRAPH END
    // --------------------------------------------------------------

    // --------------------------------------------------------------
    // PARSE TEXTURES START
    // --------------------------------------------------------------

    const auto textureOffset = scene->textureCount;

    {
        const auto &images   = model.images;
        const auto numImages = images.size ();
        auto &sceneTextures  = scene->textures;
        scene->textureCount += numImages;

        for (uint32_t i = 0; i < numImages; i++) {
            const auto &texture      = images[i];
            const auto width         = texture.width;
            const auto height        = texture.height;
            auto       &sceneTexture = sceneTextures[textureOffset + i];

            // No other texture sizes allowed
            if (width != 512 ||height != 512) {
                CHECK (false);
            }

            if (texture.component == 3) {
                for (uint32_t i = 0; i < width * height; ++i) {
                    sceneTexture[i * 4 + 0] = texture.image[i * 3 + 0];
                    sceneTexture[i * 4 + 1] = texture.image[i * 3 + 1];
                    sceneTexture[i * 4 + 2] = texture.image[i * 3 + 2];
                    sceneTexture[i * 4 + 3] = 0xFF;
                }
            } else {
                std::copy (
                    texture.image.begin (),
                    texture.image.end (),
                    sceneTexture.begin()
                );
            }
        }
    }

    // --------------------------------------------------------------
    // PARSE TEXTURES END
    // --------------------------------------------------------------

    // --------------------------------------------------------------
    // PARSE MATERIALS START
    // --------------------------------------------------------------

    {
        const auto &materials    = model.materials;
        const auto &textures     = model.textures;
        const auto numMaterials  = (uint32_t)materials.size ();
        auto &sceneMaterials     = scene->materials;
        sceneMaterials.resize (dynMatBase + numMaterials);
      
        for (uint32_t i = 0; i < numMaterials; ++i) {
            const auto &m  = materials[i];
            const auto &v  = m.values;
            auto       &sm = sceneMaterials[dynMatBase + i];

            sm.ambient  = 0.01f;
            sm.diffuse  = 0.6f;
            sm.specular = 0.2f;
            sm.alpha    = 2.0f;
            sm.texture  = 0.0f;
            sm.normal   = 1.0f;

            const auto baseColorTexture = v.find ("baseColorTexture");
            const auto normalTexture    = v.find ("normalTexture");
            const auto roughness        = v.find ("roughnessFactor");
            const auto metallic         = v.find ("metallicFactor");

            if (baseColorTexture != v.end ()) {
                sm.texture = textureOffset + textures[
                    baseColorTexture->second.TextureIndex ()
                ].source;
            }

            if (normalTexture != v.end ()) {
                sm.texture = textureOffset + textures[
                    baseColorTexture->second.TextureIndex ()
                ].source;
            }

            if (scene->templates.data () == templ) {
                sm.diffuse  = 0.2;
                sm.specular = 0.3;
            } else if (scene->templates.data() + 3 == templ) {
                sm.ambient  = 1.5f;
                sm.diffuse  = 0.0;
                sm.specular = 0.0;
            } else {
                sm.ambient  = 0.08;
                sm.alpha    = 40;
                sm.diffuse  = 0.6;
                sm.specular = 0.3;
            }


           /* if (roughness != v.end ())
                sm.alpha = roughness->second.Factor () * 42.f;
            if (metallic != v.end ())
                sm.specular = metallic->second.Factor ();  */
        }
    }

    // --------------------------------------------------------------
    // PARSE MATERIALS END
    // --------------------------------------------------------------
}

static void loadConvexNode (
    const tinygltf::Node            *nodes,
    const tinygltf::Mesh            *meshes,
    const tinygltf::Accessor        *accessors,
    const tinygltf::BufferView      *views,
    const tinygltf::Buffer          *buffers,
    const int32_t                    currentNode,
    btAlignedObjectArray<btVector3> &vertices
) {
    const auto &node = nodes[currentNode];

    if (node.mesh > -1) {
        const auto &mesh = meshes[node.mesh];

        for (const auto &p : mesh.primitives) {
            if (p.indices < 0)
                continue;

            // --------------------------------------------------------------
            // LOAD VERTICES START
            // --------------------------------------------------------------
            {
                const uint32_t currentNum = (uint32_t)vertices.size();
                const vec3    *pos = nullptr;
                uint32_t       num = 0;

                {
                    const auto &posIt = p.attributes.find ("POSITION");
                    const auto &end = p.attributes.end ();
                    if (posIt == end)
                        continue;

                    const auto &posAccessor = accessors[posIt->second];
                    const auto &posView = views[posAccessor.bufferView];

                    pos = (const vec3 *)(
                        buffers[posView.buffer].data.data ()
                        + posAccessor.byteOffset
                        + posView.byteOffset
                        );

                    num = (uint32_t)posAccessor.count;
                }

                vertices.resize (currentNum + num);
                for (uint32_t i = 0; i < num; ++i) {
                    const auto &curPos = pos[i];
                    auto       &vert   = vertices[i + currentNum];

                    vert.setX (curPos.x);
                    vert.setY (-curPos.y);
                    vert.setZ (curPos.z);
                }
            }

            // --------------------------------------------------------------
            // LOAD VERTICES END
            // --------------------------------------------------------------
        }
    }

    // --------------------------------------------------------------
    // PARSE SCENE GRAPH START
    // --------------------------------------------------------------

    {
        const auto nodeCount = node.children.size ();
        for (auto n = 0; n < nodeCount; ++n) {
            loadConvexNode (
                nodes, meshes, accessors, views, buffers,
                currentNode, vertices
            );
        }
    }

    // --------------------------------------------------------------
    // PARSE SCENE GRAPH END
    // --------------------------------------------------------------
}

static void loadConvexMeshFromGLB (
    const std::string               &modelName,
    btAlignedObjectArray<btVector3> &vertices
) {
    tinygltf::Model model;
    tinygltf::TinyGLTF loader;

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
        const auto  nodes = model.nodes.data ();
        const auto  meshes = model.meshes.data ();
        const auto  accessors = model.accessors.data ();
        const auto  views = model.bufferViews.data ();
        const auto  buffers = model.buffers.data ();
        const auto &scene = model.scenes[model.defaultScene];
        const auto  nodeCount = scene.nodes.size ();

        for (auto n = 0; n < nodeCount; ++n) {
            loadConvexNode (
                model.nodes.data(), model.meshes.data(),
                model.accessors.data(), model.bufferViews.data(),
                model.buffers.data(),
                n, vertices
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
    Scene::Scene            *templates
) {
    // --------------------------------------------------------------
    // LOAD MODEL START
    // --------------------------------------------------------------

    loadTemplateFromGLB (
        modelName, templates->nextDynTrans,
        &templates->vertices, &templates->indices,
        &templates->nextDynTrans,
        &templates->templates[templateIndex],
        templates
    );

    // --------------------------------------------------------------
    // LOAD MODEL END
    // --------------------------------------------------------------

    // --------------------------------------------------------------
    // INITIALIZE COLLISION SHAPE START
    // --------------------------------------------------------------

    auto &temp = templates->templates[templateIndex];

    switch (collisionInfo.type) {
    case Object::Box:
    {
       vec3 extent = (temp.maxExtent - temp.minExtent) * 0.5f;
       temp.shape = new btBoxShape (btVector3(
           extent.x,
           extent.y,
           extent.z
       ));
    }
        break;
    case Object::Player:
        temp.shape = new btBoxShape (btVector3 (
            1.5f,
            0.8f,
            1.8f
        ));
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