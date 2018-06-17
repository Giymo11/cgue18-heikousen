#include <fstream>
#include <string>
#include <unordered_map>

#include <LinearMath/btVector3.h>
#include <LinearMath/btAlignedObjectArray.h>
#include <LinearMath/btGeometryUtil.h>

#include "jojo_level.hpp"

namespace BSP {

const float GEOMSCALE = 0.03f;

size_t vertexCount (
    const Header *bspHeader
) {
    return bspHeader->direntries[Vertices].length / sizeof (Vertex);
}

static uint32_t indexCount (
    const Header   *bspHeader,
    const Leaf     *leafs,
    const LeafFace *leafFaces,
    const Face     *faces
) {
    uint32_t indexCount = 0;

    const auto leafBytes = (const uint8_t *)leafs + bspHeader->direntries[Leafs].length;
    const auto leafEnd = (const Leaf *)leafBytes;

    for (auto leaf = &leafs[0]; leaf != leafEnd; ++leaf) {
        const auto leafFaceBegin = leafFaces + leaf->leafface;
        const auto leafFaceEnd = leafFaceBegin + leaf->n_leaffaces;

        for (auto lface = leafFaceBegin; lface != leafFaceEnd; ++lface)
            indexCount += faces[lface->face].n_meshverts;
    }

    return indexCount;
}

void fillVertexBuffer (
    const Header     *header,
    const Leaf       *leafs,
    const LeafFace   *leafFaces,
    const Face       *faces,
    const MeshVertex *meshverts,
    const Vertex     *bspVertices,
    const int32_t    *lightmapLookup,
    Level::Vertex    *vertices
) {
    const auto leafBytes = (const uint8_t *)leafs + header->direntries[BSP::Leafs].length;
    const auto leafEnd   = (const BSP::Leaf *)leafBytes;

    for (auto leaf = &leafs[0]; leaf != leafEnd; ++leaf) {
        const auto leafFaceBegin = leafFaces + leaf->leafface;
        const auto leafFaceEnd   = leafFaceBegin + leaf->n_leaffaces;

        for (auto lface = leafFaceBegin; lface != leafFaceEnd; ++lface) {
            const auto &face         = faces[lface->face];
            const auto texture       = static_cast<float>(face.texture);
            const auto lightmap      = static_cast<float>(lightmapLookup[face.texture]);
            const auto baseVert      = face.vertex;
            const auto meshvertBegin = meshverts + face.meshvert;
            const auto meshvertEnd   = meshvertBegin + face.n_meshverts;

            for (auto mvert = meshvertBegin; mvert != meshvertEnd; ++mvert) {
                const auto index = mvert->vertex + baseVert;
                const auto &v    = bspVertices[index];
                auto &vOut       = vertices[index];
                
                vOut.pos.x      = v.position[0]  * GEOMSCALE;
                vOut.pos.y      = v.position[2]  * GEOMSCALE;
                vOut.pos.z      = -v.position[1] * GEOMSCALE;
                vOut.normal.x   = v.normal[0];
                vOut.normal.y   = v.normal[2];
                vOut.normal.z   = -v.normal[1];
                vOut.uv.x       = v.texcoord[0][0];
                vOut.uv.y       = v.texcoord[0][1];
                vOut.uv.z       = texture;
                vOut.light_uv.x = v.texcoord[1][0];
                vOut.light_uv.y = v.texcoord[1][1];
                vOut.light_uv.z = lightmap;
            }
        }
    }
}

static void buildIndicesNaiveLeaf (
    const Leaf *leaf,
    const LeafFace *leafFaces,
    const Face *faces,
    const MeshVertex *meshVerts,
    uint32_t *indices,
    size_t *nextIndex
) {
    // Only deal with visible leafs
    if (leaf->cluster < 0)
        return;

    const auto baseFace = leaf->leafface;
    const auto maxFace = baseFace + leaf->n_leaffaces;
    auto index = *nextIndex;

    for (auto lface = baseFace; lface < maxFace; lface++) {
        const auto &face = faces[leafFaces[lface].face];
        const auto baseVertex = face.vertex;
        const auto meshVertex = face.meshvert;
        const auto maxVertex = meshVertex + face.n_meshverts;

        for (auto mvert = meshVertex; mvert < maxVertex; mvert++) {
            indices[index] = baseVertex + meshVerts[mvert].vertex;
            index += 1;
        }
    }

    // Store next index
    *nextIndex = index;
}

static void buildIndicesNaiveRecursive (
    const Node *nodes,
    const Leaf *leafs,
    const LeafFace *leafFaces,
    const Face *faces,
    const MeshVertex *meshVerts,
    int32_t nodeIndex,
    int32_t depth,
    uint32_t *indices,
    size_t *nextIndex
) {
    if (nodeIndex < 0) {
        buildIndicesNaiveLeaf (
            &leafs[-(nodeIndex + 1)], leafFaces, faces,
            meshVerts, indices, nextIndex
        );
        return;
    }

    const auto &node = nodes[nodeIndex];
    const auto newDepth = depth + 1;

    buildIndicesNaiveRecursive (
        nodes, leafs, leafFaces, faces, meshVerts,
        node.children[0], newDepth, indices, nextIndex
    );
    buildIndicesNaiveRecursive (
        nodes, leafs, leafFaces, faces, meshVerts,
        node.children[1], newDepth, indices, nextIndex
    );
}

void buildIndicesNaive (
    const Node *nodes,
    const Leaf *leafs,
    const LeafFace *leafFaces,
    const Face *faces,
    const MeshVertex *meshVerts,
    uint32_t *indices
) {
    size_t nextIndex = 0;

    buildIndicesNaiveRecursive (
        nodes, leafs, leafFaces, faces, meshVerts,
        0, 0, indices, &nextIndex
    );
}

void buildColliders (
    const Header    *header,
    const Leaf      *leafs,
    const LeafBrush *leafBrushes,
    const Brush     *brushes,
    const BrushSide *brushSides,
    const Plane     *planes,
    const Texture   *textures,
    btAlignedObjectArray<btCollisionShape *>     *collisionShapes,
    btAlignedObjectArray<btDefaultMotionState *> *motionStates,
    btAlignedObjectArray<btRigidBody *>          *rigidBodies
) {
    const auto BSPCONTENTS_SOLID = 1;
    const auto leafBytes = (uint8_t *)leafs + header->direntries[Leafs].length;
    const auto leafsEnd  = (const Leaf *)leafBytes;
    const auto maxBrush  = header->direntries[Brushes].length / (int)sizeof (Brush);

    btAlignedObjectArray<bool> visited;
    visited.resize (maxBrush, false);
    collisionShapes->reserve (maxBrush);
    motionStates->reserve (maxBrush);
    rigidBodies->reserve (maxBrush);

    for (auto leaf = &leafs[0]; leaf != leafsEnd; ++leaf) {
        const auto leafBrushBegin = leafBrushes + leaf->leafbrush;
        const auto leafBrushEnd   = leafBrushBegin + leaf->n_leafbrushes;
        
        for (auto lbrush = leafBrushBegin; lbrush != leafBrushEnd; ++lbrush) {
            const auto brushIndex = lbrush->brush;
            const auto &brush     = brushes[brushIndex];

            if (brush.texture < 0 || visited[brushIndex])
                continue;
            if ((textures[brush.texture].contents & BSPCONTENTS_SOLID) == 0)
                continue;
            visited[brushIndex] = true;

            btAlignedObjectArray<btVector3> planeEquations;
            planeEquations.reserve (brush.n_brushsides);

            const auto brushSideBegin = brushSides + brush.brushside;
            const auto brushSideEnd   = brushSideBegin + brush.n_brushsides;
            for (auto bside = brushSideBegin; bside != brushSideEnd; ++bside) {
                const auto &plane = planes[bside->plane];

                btVector3 planeEq;
                planeEq.setValue (plane.normal[0], plane.normal[2], -plane.normal[1]);
                planeEq[3] = GEOMSCALE * -plane.dist;
                planeEquations.push_back (planeEq);
            }

            if (brush.n_brushsides > 0) {
                btAlignedObjectArray<btVector3> vertices;
                btGeometryUtil::getVerticesFromPlaneEquations (planeEquations, vertices);

                if (vertices.size () > 0) {
                    auto shape = new btConvexHullShape (&(vertices[0].getX ()), vertices.size ());
                    collisionShapes->push_back (shape);

                    btTransform startTransform;
                    startTransform.setIdentity ();
                    startTransform.setOrigin (btVector3 (0, 0, 0));
                    auto motionState = new btDefaultMotionState (startTransform);
                    motionStates->push_back (motionState);

                    auto info = btRigidBody::btRigidBodyConstructionInfo (0.f, motionState, shape);
                    auto body = new btRigidBody (info);
                    rigidBodies->push_back (body);
                }
            }
        }
    }
}

BSPData::BSPData (
    std::vector<uint8_t>     &&data,
    std::vector<std::string> &&texturesIn,
    std::vector<std::string> &&normalsIn,
    std::vector<std::string> &&lightmapsIn,
    std::vector<int32_t>     &&lightmapLookupIn,
    uint32_t                   indexCount
) :
    raw            (std::move (data)),
    textures       (std::move (texturesIn)),
    normals        (std::move (normalsIn)),
    lightmaps      (std::move (lightmapsIn)),
    lightmapLookup (std::move (lightmapLookupIn)),
    header         ((const Header *)     (raw.data () + 0)),
    nodes          ((const Node *)       (raw.data () + header->direntries[Nodes].offset)),
    leafs          ((const Leaf *)       (raw.data () + header->direntries[Leafs].offset)),
    leafFaces      ((const LeafFace *)   (raw.data () + header->direntries[Leaffaces].offset)),
    faces          ((const Face *)       (raw.data () + header->direntries[Faces].offset)),
    meshVertices   ((const MeshVertex *) (raw.data () + header->direntries[Meshverts].offset)),
    vertices       ((const Vertex *)     (raw.data () + header->direntries[Vertices].offset)),
    leafBrushes    ((const LeafBrush *)  (raw.data () + header->direntries[Leafbrushes].offset)),
    brushes        ((const Brush *)      (raw.data () + header->direntries[Brushes].offset)),
    brushSides     ((const BrushSide *)  (raw.data () + header->direntries[Brushsides].offset)),
    planes         ((const Plane *)      (raw.data () + header->direntries[Planes].offset)),
    textureData    ((const Texture *)    (raw.data () + header->direntries[Textures].offset)),
    indexCount     (indexCount)
{
}

std::unique_ptr<BSPData> loadBSP (
    const std::string &name
) {
    std::ifstream file (
        name + ".bsp",
        std::ios_base::in | std::ios_base::binary | std::ios_base::ate
    );
    if (!file.is_open ())
        return nullptr;

    // Load whole file into memory
    auto size = file.tellg ();
    file.seekg (0, std::ios::beg);
    std::vector<uint8_t> buffer ((size_t)size);
    file.read ((char *)buffer.data (), size);
    file.close ();

    auto header    = (Header *)   (buffer.data () + 0);
    auto leafs     = (Leaf *)     (buffer.data () + header->direntries[Leafs].offset);
    auto leafFaces = (LeafFace *) (buffer.data () + header->direntries[Leaffaces].offset);
    auto faces     = (Face *)     (buffer.data () + header->direntries[Faces].offset);
    auto vertices  = (Vertex *)   (buffer.data () + header->direntries[Vertices].offset);
    auto indexNum  = indexCount (header, leafs, leafFaces, faces);

    // --------------------------------------------------------------
    // TEXTURE FIX BEGIN
    // --------------------------------------------------------------

    int32_t newTextureCount = 0;
    std::unordered_map<std::string, int32_t> nameLookup;

    {
        const auto textures    = (const Texture *) (buffer.data () + header->direntries[Textures].offset);
        const auto numTextures = header->direntries[Textures].length / sizeof (Texture);
        std::vector<int32_t> lookup (numTextures, 0);

        // Create lookup array
        for (auto i = 0; i < numTextures; ++i) {
            const auto name = (const char *)textures[i].name;
            if (name[0] != 'h')
                continue;

            newTextureCount += 1;
            lookup[i] = newTextureCount;
            nameLookup.emplace (name + 10, newTextureCount);
        }

        // Fix texture of every face
        auto faceEndBytes = (uint8_t *)faces + header->direntries[Faces].length;
        auto facesEnd     = (Face *)faceEndBytes;
        for (auto face = &faces[0]; face != facesEnd; ++face)
            face->texture = lookup[face->texture];
    }

    std::vector<std::string> textures       (newTextureCount);
    std::vector<std::string> normals        (newTextureCount);
    std::vector<int32_t>     lightmapLookup (newTextureCount, 0);

    int32_t lightmapCount = 0;
    std::vector<std::string> lightmaps;
    std::unordered_map<std::string, int32_t> lightmapNames;
    lightmaps.reserve (newTextureCount);

    {
        std::ifstream tex (name + ".tex");
        if (!tex.is_open ())
            return nullptr;

        std::string hash;
        std::string texture;
        std::string lightmap;
        std::string normal;

        while (tex >> hash >> texture >> normal >> lightmap) {
            const auto it = nameLookup.find (hash);
            if (it == nameLookup.end ())
                continue;

            const auto index = it->second - 1;
            textures[index] = texture;
            normals[index] = normal;

            auto lm = lightmapNames.find (lightmap);
            if (lm == lightmapNames.end ()) {
                lightmapCount += 1;
                lightmapLookup[index] = lightmapCount;
                lightmapNames.emplace (lightmap, lightmapCount);
                lightmaps.emplace_back (lightmap);
            } else {
                lightmapLookup[index] = lm->second;
            }
        }
    }

    // --------------------------------------------------------------
    // TEXTURE FIX END
    // --------------------------------------------------------------

    return std::make_unique<BSPData>(
        std::move(buffer),
        std::move(textures),
        std::move(normals),
        std::move(lightmaps),
        std::move(lightmapLookup),
        indexNum
    );
}

}