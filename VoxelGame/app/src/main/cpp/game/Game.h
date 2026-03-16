#pragma once
#include <string>
#include <unordered_map>
#include <memory>
#include <vector>
#include "../world/World.h"
#include "../world/Chunk.h"
#include "../renderer/Renderer.h"
#include "../network/FirebaseClient.h"
#include "../auth/GoogleAuth.h"
#include "../input/TouchInput.h"

// ── Player state ──────────────────────────────────────────────────
struct PlayerState {
    glm::vec3 position  = {0, 20, 0};
    glm::vec3 velocity  = {0, 0, 0};
    float     yaw       = 0.0f;      // radians
    float     pitch     = 0.0f;
    bool      onGround  = false;
    bool      flying    = false;

    int    health    = 6;
    int    maxHealth = 6;
    int    coins     = 0;
    std::string displayName = "Player";
    std::string uid;
};

// ── Inventory item ────────────────────────────────────────────────
struct InvItem { int id=0; int count=0; };

// ── Remote player ─────────────────────────────────────────────────
struct RemotePlayer {
    std::string uid;
    std::string name;
    glm::vec3   position = {0,0,0};
    float       yaw      = 0.0f;
    bool        moving   = false;
    int         health   = 6;
};

// ────────────────────────────────────────────────────────────────────
//  Game  — the top-level game state machine
// ────────────────────────────────────────────────────────────────────
class Game {
public:
    enum class State { Loading, SignIn, Playing, Paused };

    void init(JNIEnv* env, jobject activity, int screenW, int screenH);
    void resize(int w, int h);
    void tick(float dt);                 // called every frame
    void onTouchEvent(AInputEvent* ev);
    void destroy();

    State state() const { return state_; }

    // JNI-accessible: called from Java after auth completes
    void onAuthResult(const AuthResult& r);

private:
    // ── Subsystems ───────────────────────────────────────────────
    World           world_;
    Renderer        renderer_;
    FirebaseClient  firebase_;
    GoogleAuth      auth_;
    TouchInput      touch_;

    // ── Game state ────────────────────────────────────────────────
    State          state_     = State::Loading;
    PlayerState    player_;
    Camera         camera_;
    InputState     input_;

    // ── Inventory: 30 slots matching JS ──────────────────────────
    InvItem        inventory_[30] = {};
    InvItem        hotbar_[9]     = {};
    int            hotbarIndex_   = 0;

    // ── World management ──────────────────────────────────────────
    int  renderDist_  = RENDER_DIST_DEFAULT;
    std::unordered_map<std::string, std::unique_ptr<ChunkRender>> chunkRenders_;
    std::unordered_map<std::string, ChunkRender*> visibleChunks_;
    // Re-usable CPU mesh buffers (avoid alloc every frame)
    MeshData    solidBuf_, transpBuf_;
    FoliageMesh foliageBuf_;

    // ── Network state ─────────────────────────────────────────────
    std::unordered_map<std::string, RemotePlayer> remotePlayers_;
    float       networkTimer_  = 0.0f;
    float       networkRate_   = 0.1f;  // 100ms, matching JS
    float       gameTime_      = 0.0f;  // seconds, synced from Firebase epoch
    long long   epochMs_       = 0;     // Firebase world_clock epoch

    // ── Physics constants (matching JS) ──────────────────────────
    static constexpr float GRAVITY     = -25.0f;
    static constexpr float JUMP_VEL    = 9.0f;
    static constexpr float WALK_SPEED  = 4.5f;
    static constexpr float FLY_SPEED   = 8.0f;
    static constexpr float SPRINT_MOD  = 1.6f;

    // ── Internals ─────────────────────────────────────────────────
    void updateChunks();
    void rebuildChunk(int cx, int cz);
    void unloadDistantChunks();
    void updatePhysics(float dt);
    void updateCamera();
    void syncToNetwork(float dt);
    void applyNetworkUpdate(const std::string& json);
    void initFirebaseListeners();
    void startGame();

    std::string chunkKey(int cx, int cz) const {
        return std::to_string(cx)+","+std::to_string(cz);
    }

    int screenW_ = 0, screenH_ = 0;
};
