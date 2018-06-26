#pragma once
#include <cstdint>
#include <memory>
#include <vector>
#include <glm/glm.hpp>

namespace Level {
struct Vertex;
struct Leaf;
}

namespace BSP {

using namespace glm;

enum LumpType : int32_t {
    Entities,
    Textures,
    Planes,
    Nodes,
    Leafs,
    Leaffaces,
    Leafbrushes,
    Models,
    Brushes,
    Brushsides,
    Vertices,
    Meshverts,
    Effects,
    Faces,
    Lightmaps,
    Lightvols,
    Visdata
};

enum FaceType : int32_t {
    Invalid,
    Polygon,
    Patch,
    Mesh,
    Billboard
};

struct DirEntry {
    int32_t    offset;
    int32_t    length;
};

struct Header {
    uint8_t    magic[4];
    int32_t    version;
    DirEntry   direntries[17];
};

struct Texture {
    uint8_t    name[64];
    int32_t    flags;
    int32_t    contents;
};

struct Node {
    int32_t    plane;
    int32_t    children[2];
    int32_t    mins[3];
    int32_t    maxs[3];
};

struct Leaf {
    int32_t    cluster;
    int32_t    area;
    int32_t    mins[3];
    int32_t    maxs[3];
    int32_t    leafface;
    int32_t    n_leaffaces;
    int32_t    leafbrush;
    int32_t    n_leafbrushes;
};

struct LeafFace {
    int32_t    face;
};

struct LeafBrush {
    int32_t    brush;
};

struct Brush {
    int32_t    brushside;
    int32_t    n_brushsides;
    int32_t    texture;
};

struct BrushSide {
    int32_t    plane;
    int32_t    texture;
};

struct Plane {
    float      normal[3];
    float      dist;
};

struct Face {
    int32_t    texture;
    int32_t    effect;
    FaceType   type;
    int32_t    vertex;
    int32_t    n_vertices;
    int32_t    meshvert;
    int32_t    n_meshverts;
    int32_t    lm_index;
    int32_t    lm_start[2];
    int32_t    lm_size[2];
    float      lm_origin[3];
    float      lm_vecs[2][3];
    float      normal[3];
    int32_t    size[2];
};

struct Vertex {
    float      position[3];
    float      texcoord[2][2];
    float      normal[3];
    uint8_t    color[4];
};

struct MeshVertex {
    int32_t    vertex;
};

struct BSPData {
    BSPData (
        std::vector<uint8_t>     &&raw,
        std::vector<std::string> &&textures,
        std::vector<std::string> &&normals,
        std::vector<std::string> &&lightmaps,
        std::vector<int32_t>     &&lightmapLookup,
        std::vector<vec3>        &&lightPos,
        uint32_t                   indexCount,
        uint32_t                   leafCount
    );

    const std::vector<uint8_t>     raw;
    const std::vector<std::string> textures;
    const std::vector<std::string> normals;
    const std::vector<std::string> lightmaps;
    const std::vector<int32_t>     lightmapLookup;
    const std::vector<glm::vec3>   lightPos;

    const Header     *header;
    const Node       *nodes;
    const Leaf       *leafs;
    const LeafFace   *leafFaces;
    const Brush      *brushes;
    const LeafBrush  *leafBrushes;
    const BrushSide  *brushSides;
    const Plane      *planes;
    const Texture    *textureData;
    const Face       *faces;
    const MeshVertex *meshVertices;
    const Vertex     *vertices;
    const uint32_t    indexCount;
    const uint32_t    leafCount;
};

std::unique_ptr<BSPData> loadBSP (
    const std::string &name
);

size_t vertexCount (
    const Header     *bspHeader
);

void fillVertexBuffer (
    const Header     *header,
    const Leaf       *leafs,
    const LeafFace   *leafFaces,
    const Face       *faces,
    const MeshVertex *meshverts,
    const Vertex     *bspVertices,
    const int32_t    *lightmapLookup,
    Level::Vertex    *vertices
);

void buildIndicesNaive (
    const Node               *nodes,
    const Leaf               *leafs,
    const LeafFace           *leafFaces,
    const Face               *faces,
    const MeshVertex         *meshVerts,
    std::vector<Level::Leaf> &drawLeafs,
    uint32_t                 *indices
);

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
);

}