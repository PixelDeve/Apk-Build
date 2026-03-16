#include "World.h"
#include <cmath>
#include <cstring>

// ── Matching the JS hash function exactly ──────────────────────────
int World::hash(int x, int z) {
    // JS: (Math.abs(x * 73856093 ^ z * 19349663) % 1000000) | 0
    int h = (int)(std::abs((long long)x * 73856093 ^ (long long)z * 19349663) % 1000000);
    return h;
}

// ── Terrain height: exactly mirrors JS getBlock height formula ─────
int World::terrainHeight(int x, int z) {
    return (int)std::floor(
        10.0 + std::sin(x * 0.05 + z * 0.03) * 4.0
             + std::cos(x * 0.03 - z * 0.05) * 3.0
    );
}

// ── Tree detection (matching JS inner-loop logic) ──────────────────
bool World::isTreeLog(int x, int y, int z) {
    // For each candidate tree root in [-2,+2] radius
    for (int tx = -2; tx <= 2; tx++) {
        for (int tz = -2; tz <= 2; tz++) {
            int wx = x - tx, wz = z - tz;
            int th = terrainHeight(wx, wz);
            if (th >= 7 && hash(wx, wz) % 100 < 2) {
                int relX = x - wx, relZ = z - wz, relY = y - th;
                if (relX == 0 && relZ == 0 && relY > 0 && relY <= 5) return true;
            }
        }
    }
    return false;
}

bool World::isTreeLeaves(int x, int y, int z) {
    for (int tx = -2; tx <= 2; tx++) {
        for (int tz = -2; tz <= 2; tz++) {
            int wx = x - tx, wz = z - tz;
            int th = terrainHeight(wx, wz);
            if (th >= 7 && hash(wx, wz) % 100 < 2) {
                int relX = x - wx, relZ = z - wz, relY = y - th;
                if (relY >= 4 && relY <= 6) {
                    int dist = std::abs(relX) + std::abs(relZ) + std::abs(relY - 5);
                    if (dist <= 2 && !(relX == 0 && relZ == 0 && relY <= 5)) return true;
                }
            }
        }
    }
    return false;
}

World::World() {}

void World::init() {
    blocks_ = makeBlockDefs();
}

int World::getBlock(int x, int y, int z) const {
    // ── Check override map first (Firebase world_changes) ──────────
    auto it = worldOverrides_.find(key(x, y, z));
    if (it != worldOverrides_.end()) return it->second;

    if (y < 0) return 0;
    if (y == 0) return 9; // Bedrock

    // ── Tree structure ────────────────────────────────────────────
    if (isTreeLog(x, y, z))    return 4; // Log
    if (isTreeLeaves(x, y, z)) return 5; // Leaves

    int h = terrainHeight(x, z);
    if (y > h) return 0; // Air
    if (y == h) return (h < 7) ? 6 : 1; // Sand under y=7, else Grass

    // ── Underground ───────────────────────────────────────────────
    if (y < h - 3) {
        // Ore distribution exactly matching JS
        int rand = World::hash(x + y * 1000003, z) % 10000;
        if (rand <    1) return 54; // Diamond Block (very rare)
        if (rand <    5) return 15; // Diamond Ore
        if (rand <   20) return 14; // Gold Ore
        if (rand <   60) return 13; // Iron Ore
        if (rand <  150) return 12; // Coal Ore
        return 3; // Stone
    }

    // ── Near surface: dirt above h-3 ─────────────────────────────
    return (y > h - 3) ? 2 : 3;
}

void World::setBlock(int x, int y, int z, int id) {
    worldOverrides_[key(x, y, z)] = id;
}
