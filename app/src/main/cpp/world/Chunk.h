#pragma once
#include <vector>
#include <array>
#include <memory>
#include "World.h"

// One renderable mesh bucket (solid or transparent)
struct MeshData {
    std::vector<float> positions;   // xyz
    std::vector<float> normals;     // xyz
    std::vector<float> uvs;         // uv
    std::vector<float> colors;      // rgb (vertex AO)
    std::vector<int>   indices;
    int vertexCount = 0;

    void clear() {
        positions.clear(); normals.clear();
        uvs.clear(); colors.clear(); indices.clear();
        vertexCount = 0;
    }
};

// ── Foliage billboard: cross-shaped grass ────────────────────────
struct FoliageVertex {
    float x, y, z;
    float u, v;
    float ao;
};

struct FoliageMesh {
    std::vector<FoliageVertex> verts;
    std::vector<int>           indices;
    int vertexCount = 0;

    void clear() { verts.clear(); indices.clear(); vertexCount = 0; }
};

// ── GPU handles for one chunk ─────────────────────────────────────
struct ChunkGPU {
    unsigned int solidVAO   = 0, solidVBO   = 0, solidEBO   = 0;
    unsigned int transpVAO  = 0, transpVBO  = 0, transpEBO  = 0;
    unsigned int foliageVAO = 0, foliageVBO = 0, foliageEBO = 0;
    int solidCount   = 0;
    int transpCount  = 0;
    int foliageCount = 0;
    bool dirty = true;
};

// ────────────────────────────────────────────────────────────────────
//  ChunkBuilder – CPU-side mesh generation
//  Mirrors buildChunk() in the JS source
// ────────────────────────────────────────────────────────────────────
class ChunkBuilder {
public:
    // atlasConfig: { cols, rows } from the loaded texture atlas
    struct AtlasConfig { int cols=8; int rows=8; };

    static void build(
        int chunkX, int chunkZ,
        const World& world,
        const AtlasConfig& atlas,
        MeshData& solidOut,
        MeshData& transpOut,
        FoliageMesh& foliageOut
    );

private:
    // ── Emits all 6 faces of a sub-block box ────────────────────
    static void emitBox(
        MeshData& mesh,
        int wx, int wy, int wz,
        float x0, float x1, float y0, float y1, float z0, float z1,
        const std::array<int,6>& tiles,
        const AtlasConfig& atlas
    );

    // ── Emits one billboard quad ─────────────────────────────────
    static void emitBillboard(
        FoliageMesh& mesh,
        float wx, float wy, float wz,
        float u0, float u1, float v0, float v1
    );
};
