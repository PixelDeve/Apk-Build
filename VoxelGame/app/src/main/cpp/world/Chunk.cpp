#include "Chunk.h"
#include <cmath>

// ── Face AO values matching JS emitBox ────────────────────────────
static constexpr float AO[6] = { 0.8f, 0.8f, 1.0f, 0.5f, 0.9f, 0.9f };

// Face normal directions: +X,-X,+Y,-Y,+Z,-Z
static constexpr float NORMALS[6][3] = {
    { 1,0,0},{-1,0,0},{0,1,0},{0,-1,0},{0,0,1},{0,0,-1}
};

// Face neighbor offsets for face-culling
static constexpr int NEIGH[6][3] = {
    {1,0,0},{-1,0,0},{0,1,0},{0,-1,0},{0,0,1},{0,0,-1}
};

// ── Vertex positions for each face (local block space) ───────────
// Order: BL,TL,TR,BR  (matches UV winding in JS)
static const float FACE_VERTS[6][4][3] = {
    // +X
    {{1,0,0},{1,1,0},{1,1,1},{1,0,1}},
    // -X
    {{0,0,1},{0,1,1},{0,1,0},{0,0,0}},
    // +Y
    {{0,1,1},{1,1,1},{1,1,0},{0,1,0}},
    // -Y
    {{0,0,0},{1,0,0},{1,0,1},{0,0,1}},
    // +Z
    {{1,0,1},{1,1,1},{0,1,1},{0,0,1}},
    // -Z
    {{0,0,0},{0,1,0},{1,1,0},{1,0,0}},
};

// UV fracs per face axis (matches JS vFrac/uFrac for sub-block shapes)
// For a full unit cube all fracs are 1.0
static constexpr float UV_FRAC_V[6] = {1,1,1,1,1,1};
static constexpr float UV_FRAC_U[6] = {1,1,1,1,1,1};

void ChunkBuilder::emitBox(
    MeshData& mesh,
    int wx, int wy, int wz,
    float x0, float x1, float y0, float y1, float z0, float z1,
    const std::array<int,6>& tiles,
    const AtlasConfig& atlas
) {
    const float _HP   = 0.5f;
    const float invW  = 1.0f / (atlas.cols * 32.0f);
    const float invH  = 1.0f / (atlas.rows * 32.0f);

    // Build scaled face vertices
    const float mins[3] = {x0,y0,z0};
    const float maxs[3] = {x1,y1,z1};

    for (int fi = 0; fi < 6; fi++) {
        int ti = tiles[fi];
        float ao = AO[fi];

        float u  = (float)(ti % atlas.cols) / (float)atlas.cols + _HP * invW;
        float v  = 1.0f - (float)(ti / atlas.cols + 1) / (float)atlas.rows + _HP * invH;
        float uS = (1.0f / atlas.cols - 2 * _HP * invW);
        float vS = (1.0f / atlas.rows - 2 * _HP * invH);

        int base = mesh.vertexCount;
        for (int vi = 0; vi < 4; vi++) {
            // Scale from unit-cube coords to sub-block coords
            float vx = (FACE_VERTS[fi][vi][0] < 0.5f) ? x0 : x1;
            float vy = (FACE_VERTS[fi][vi][1] < 0.5f) ? y0 : y1;
            float vz = (FACE_VERTS[fi][vi][2] < 0.5f) ? z0 : z1;

            mesh.positions.push_back((float)wx + vx);
            mesh.positions.push_back((float)wy + vy);
            mesh.positions.push_back((float)wz + vz);
            mesh.normals.push_back(NORMALS[fi][0]);
            mesh.normals.push_back(NORMALS[fi][1]);
            mesh.normals.push_back(NORMALS[fi][2]);
            mesh.colors.push_back(ao);
            mesh.colors.push_back(ao);
            mesh.colors.push_back(ao);
        }
        // UV quad: BL,TL,TR,BR → (u,v),(u,v+vS),(u+uS,v+vS),(u+uS,v)
        mesh.uvs.push_back(u);    mesh.uvs.push_back(v);
        mesh.uvs.push_back(u);    mesh.uvs.push_back(v+vS);
        mesh.uvs.push_back(u+uS); mesh.uvs.push_back(v+vS);
        mesh.uvs.push_back(u+uS); mesh.uvs.push_back(v);
        // Two triangles
        mesh.indices.push_back(base);
        mesh.indices.push_back(base+1);
        mesh.indices.push_back(base+2);
        mesh.indices.push_back(base);
        mesh.indices.push_back(base+2);
        mesh.indices.push_back(base+3);
        mesh.vertexCount += 4;
    }
}

// ── Minecraft-style cross billboard for grass plants ──────────────
void ChunkBuilder::emitBillboard(
    FoliageMesh& mesh,
    float wx, float wy, float wz,
    float u0, float u1, float v0, float v1
) {
    // Two quads crossing at center, each offset ±0.35 on the perpendicular axis
    const float off = 0.35f;
    const float h   = 0.8f; // billboard height

    struct Quad { float dx0,dz0, dx1,dz1; };
    Quad quads[2] = {
        { -off, -off,  off,  off },   // diagonal /
        { -off,  off,  off, -off },   // diagonal
    };

    for (auto& q : quads) {
        int base = mesh.vertexCount;
        // BL, TL, TR, BR
        FoliageVertex vs[4] = {
            { wx+0.5f+q.dx0, wy+0.0f, wz+0.5f+q.dz0, u0, v0, 0.85f },
            { wx+0.5f+q.dx0, wy+  h,  wz+0.5f+q.dz0, u0, v1, 0.85f },
            { wx+0.5f+q.dx1, wy+  h,  wz+0.5f+q.dz1, u1, v1, 0.85f },
            { wx+0.5f+q.dx1, wy+0.0f, wz+0.5f+q.dz1, u1, v0, 0.85f },
        };
        for (auto& v : vs) mesh.verts.push_back(v);
        mesh.indices.push_back(base);   mesh.indices.push_back(base+1); mesh.indices.push_back(base+2);
        mesh.indices.push_back(base);   mesh.indices.push_back(base+2); mesh.indices.push_back(base+3);
        // back-face (alpha test = no culling needed but add anyway for solid look)
        mesh.indices.push_back(base+2); mesh.indices.push_back(base+1); mesh.indices.push_back(base);
        mesh.indices.push_back(base+3); mesh.indices.push_back(base+2); mesh.indices.push_back(base);
        mesh.vertexCount += 4;
    }
}

// ────────────────────────────────────────────────────────────────────
//  build() — mirrors JS buildChunk() exactly
// ────────────────────────────────────────────────────────────────────
void ChunkBuilder::build(
    int cx, int cz,
    const World& world,
    const AtlasConfig& atlas,
    MeshData& solidOut,
    MeshData& transpOut,
    FoliageMesh& foliageOut
) {
    solidOut.clear();
    transpOut.clear();
    foliageOut.clear();

    const auto& blocks = world.blocks();

    for (int lx = 0; lx < CHUNK_SIZE; lx++) {
        for (int lz = 0; lz < CHUNK_SIZE; lz++) {
            int wx = cx * CHUNK_SIZE + lx;
            int wz = cz * CHUNK_SIZE + lz;

            for (int y = 0; y < WORLD_HEIGHT; y++) {
                int type = world.getBlock(wx, y, wz);
                if (type == 0) continue;

                auto it = blocks.find(type);
                if (it == blocks.end()) continue;
                const BlockDef& blk = it->second;

                // ── Door: skip if open ──────────────────────────
                if (blk.isDoor) {
                    std::string dk = World::key(wx, y, wz);
                    auto si = world.blockStates.find(dk);
                    if (si != world.blockStates.end() && si->second.open) continue;
                }

                bool isTransp = blk.transparent && !blk.isTorch;
                MeshData& mesh = isTransp ? transpOut : solidOut;

                // ── Slab ─────────────────────────────────────────
                if (blk.isSlab) {
                    emitBox(mesh, wx, y, wz, 0,1, 0,0.5f, 0,1, blk.tiles, atlas);
                    continue;
                }

                // ── Stair ─────────────────────────────────────────
                if (blk.isStair) {
                    auto si = world.blockStates.find(World::key(wx,y,wz));
                    int facing = si != world.blockStates.end() ? si->second.facing : 0;
                    emitBox(mesh, wx, y, wz, 0,1, 0,0.5f, 0,1, blk.tiles, atlas);
                    float stepBoxes[4][6] = {
                        {0,1, 0.5f,1, 0.5f,1},
                        {0.5f,1, 0.5f,1, 0,1},
                        {0,1, 0.5f,1, 0,0.5f},
                        {0,0.5f, 0.5f,1, 0,1},
                    };
                    auto& sb = stepBoxes[facing % 4];
                    emitBox(mesh, wx, y, wz, sb[0],sb[1],sb[2],sb[3],sb[4],sb[5], blk.tiles, atlas);
                    continue;
                }

                // ── Torch ─────────────────────────────────────────
                if (blk.isTorch) {
                    // Tiny 2/16-wide post in center
                    emitBox(mesh, wx, y, wz,
                            6.0f/16, 10.0f/16,
                            0,       10.0f/16,
                            6.0f/16, 10.0f/16,
                            blk.tiles, atlas);
                    continue;
                }

                // ── Regular cube — face culling ───────────────────
                for (int fi = 0; fi < 6; fi++) {
                    int nx = wx + NEIGH[fi][0];
                    int ny = y  + NEIGH[fi][1];
                    int nz = wz + NEIGH[fi][2];
                    int neighbour = world.getBlock(nx, ny, nz);

                    // Cull if neighbour is opaque
                    bool neighbourOpaque = false;
                    if (neighbour != 0) {
                        auto nit = blocks.find(neighbour);
                        if (nit != blocks.end() && !nit->second.transparent)
                            neighbourOpaque = true;
                    }
                    if (neighbourOpaque) continue;

                    // Single-face emit
                    float x0 = (float)(FACE_VERTS[fi][0][0] < 0.5f ? 0 : 1);
                    // We always emit full-cube here; emitBox handles sub-box coords
                    // But for regular cubes just emit the face directly ──
                    float ao = AO[fi];
                    float _HP = 0.5f;
                    float invW = 1.0f / (atlas.cols * 32.0f);
                    float invH = 1.0f / (atlas.rows * 32.0f);
                    int ti = blk.tiles[fi];
                    float u  = (float)(ti % atlas.cols) / atlas.cols + _HP * invW;
                    float v  = 1.0f - (float)(ti / atlas.cols + 1) / atlas.rows + _HP * invH;
                    float uS = 1.0f / atlas.cols - 2*_HP*invW;
                    float vS = 1.0f / atlas.rows - 2*_HP*invH;

                    int base = mesh.vertexCount;
                    for (int vi = 0; vi < 4; vi++) {
                        mesh.positions.push_back((float)wx + FACE_VERTS[fi][vi][0]);
                        mesh.positions.push_back((float) y + FACE_VERTS[fi][vi][1]);
                        mesh.positions.push_back((float)wz + FACE_VERTS[fi][vi][2]);
                        mesh.normals.push_back(NORMALS[fi][0]);
                        mesh.normals.push_back(NORMALS[fi][1]);
                        mesh.normals.push_back(NORMALS[fi][2]);
                        mesh.colors.push_back(ao);
                        mesh.colors.push_back(ao);
                        mesh.colors.push_back(ao);
                    }
                    mesh.uvs.push_back(u);    mesh.uvs.push_back(v);
                    mesh.uvs.push_back(u);    mesh.uvs.push_back(v+vS);
                    mesh.uvs.push_back(u+uS); mesh.uvs.push_back(v+vS);
                    mesh.uvs.push_back(u+uS); mesh.uvs.push_back(v);
                    mesh.indices.push_back(base);   mesh.indices.push_back(base+1); mesh.indices.push_back(base+2);
                    mesh.indices.push_back(base);   mesh.indices.push_back(base+2); mesh.indices.push_back(base+3);
                    mesh.vertexCount += 4;
                }

                // ── Grass billboard foliage on top of grass blocks ─
                if (type == 1) {
                    int ay = y + 1;
                    if (world.getBlock(wx, ay, wz) == 0) {
                        std::string plantKey = World::key(wx, ay, wz);
                        bool broken = world.brokenGrassPlants.count(plantKey) > 0;
                        if (!broken) {
                            // UV will be 0,1,0,1 if no grass_plant texture loaded;
                            // Renderer will bind the grass_plant texture separately
                            emitBillboard(foliageOut,
                                (float)wx, (float)ay, (float)wz,
                                0.0f, 1.0f, 0.0f, 1.0f);
                        }
                    }
                }
            } // y
        } // lz
    } // lx
}
