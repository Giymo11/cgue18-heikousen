#pragma once
#include <cstdint>
#include <memory>
#include <vector>

namespace Level {
struct Vertex;
}

namespace BSP {

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
    BSPData (const std::vector<uint8_t> raw);

    const std::vector<uint8_t>  raw;
    const Header     *header;
    const Node       *nodes;
    const Leaf       *leafs;
    const LeafFace   *leafFaces;
    const Face       *faces;
    const MeshVertex *meshVertices;
    const Vertex     *vertices;
};

std::unique_ptr<BSPData> loadBSP (
    const std::string &name
);

size_t vertexCount (
    const Header     *bspHeader
);

size_t indexCount (
    const Header     *bspHeader
);

void fillVertexBuffer (
    const Vertex     *bspVertices,
    size_t            count,
    Level::Vertex    *vertices
);

void buildIndicesNaive (
    const Node       *nodes,
    const Leaf       *leafs,
    const LeafFace   *leafFaces,
    const Face       *faces,
    const MeshVertex *meshVerts,
    uint32_t         *indices
);

}