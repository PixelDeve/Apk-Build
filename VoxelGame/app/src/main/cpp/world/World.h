#pragma once
#include <unordered_map>
#include <string>
#include <cmath>
#include <functional>
#include "Blocks.h"

// ────────────────────────────────────────────────────────────────────
//  World constants  (mirror JS source exactly)
// ────────────────────────────────────────────────────────────────────
static constexpr int   CHUNK_SIZE         = 16;
static constexpr int   WORLD_HEIGHT       = 32;
static constexpr float PLAYER_HEIGHT      = 1.62f;
static constexpr float PLAYER_RADIUS      = 0.3f;
static constexpr float DAY_LENGTH_SECONDS = 300.0f;

// ────────────────────────────────────────────────────────────────────
//  A block change recorded from Firebase (used for replay on load)
// ────────────────────────────────────────────────────────────────────
struct BlockChange {
    int   x, y, z;
    int   id;       // 0 = BREAK
    int   prevId;
    int   facing;
    bool  isBreak;
};

// ────────────────────────────────────────────────────────────────────
//  World  – stores block overrides over procedural terrain
// ────────────────────────────────────────────────────────────────────
class World {
public:
    World();

    // Initialise block definitions
    void init();

    // ── Block access ──────────────────────────────────────────────
    int  getBlock(int x, int y, int z) const;
    void setBlock(int x, int y, int z, int id);

    // ── Terrain (procedural, matches JS exactly) ──────────────────
    static int  terrainHeight(int x, int z);
    static bool isTreeLog   (int x, int y, int z);
    static bool isTreeLeaves(int x, int y, int z);

    // ── Override map (Firebase world_changes) ─────────────────────
    std::unordered_map<std::string, int>& overrides() { return worldOverrides_; }

    // ── Block definitions ─────────────────────────────────────────
    const std::unordered_map<int, BlockDef>& blocks() const { return blocks_; }
    std::unordered_map<int, BlockDef>&       blocks()       { return blocks_; }

    // ── Door/stair/chest state ────────────────────────────────────
    struct BlockState { bool open=false; int facing=0; };
    std::unordered_map<std::string, BlockState> blockStates;

    // ── Broken grass plants (Firebase broken_grass) ───────────────
    std::unordered_map<std::string, bool> brokenGrassPlants;

    // ── Helper ────────────────────────────────────────────────────
    static std::string key(int x, int y, int z) {
        return std::to_string(x)+","+std::to_string(y)+","+std::to_string(z);
    }

private:
    // ── Deterministic hash matching JS source ─────────────────────
    static int hash(int x, int z);

    std::unordered_map<std::string, int>     worldOverrides_;
    std::unordered_map<int, BlockDef>        blocks_;
};
