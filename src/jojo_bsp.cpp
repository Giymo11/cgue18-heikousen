#include <fstream>
#include <string>
#include <unordered_map>

#include "jojo_level.hpp"

namespace BSP {

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
    const Vertex *bspVertices,
    size_t count,
    Level::Vertex *vertices
) {
    for (int32_t i = 0; i < count; i++)
        vertices[i] = *(Level::Vertex *)(&bspVertices[i]);
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

BSPData::BSPData (
    std::vector<uint8_t> &&data,
    std::vector<std::string> &&texturesIn,
    std::vector<std::string> &&normalsIn,
    std::vector<std::string> &&lightmapsIn,
    std::vector<int32_t> &&lightmapLookupIn,
    uint32_t indexCount
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
    // COORDINATE SYSTEM FIX BEGIN
    // --------------------------------------------------------------

    const auto vertexBytes = (uint8_t *)vertices + header->direntries[Vertices].length;
    const auto endVertex = (Vertex *)vertexBytes;
    const auto scale = 0.03f;
    for (auto v = &vertices[0]; v != endVertex; ++v) {
        float tmp = v->position[1];
        v->position[1] = v->position[2];
        v->position[2] = -tmp;

        tmp = v->normal[1];
        v->normal[1] = v->normal[2];
        v->normal[2] = -tmp;

        v->position[0] *= scale;
        v->position[1] *= scale;
        v->position[2] *= scale;
    }

    // --------------------------------------------------------------
    // COORDINATE SYSTEM FIX END
    // --------------------------------------------------------------

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

        while (tex >> hash >> texture >> lightmap >> normal) {
            const auto it = nameLookup.find (hash);
            if (it == nameLookup.end ())
                continue;

            const auto index = it->second;
            textures[index] = std::move(texture);
            normals[index] = std::move (normal);

            auto lm = lightmapNames.find (lightmap);
            if (lm == lightmapNames.end ()) {
                lightmapCount += 1;
                lightmapLookup[index] = lightmapCount;
                lightmapNames.emplace (lightmap, lightmapCount);
                lightmaps.emplace_back (std::move(lightmap));
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