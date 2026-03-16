#include "Game.h"
#include <cmath>
#include <algorithm>
#include <android/log.h>

#define GTAG "VoxelGame"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, GTAG, __VA_ARGS__)

void Game::init(JNIEnv* env, jobject activity, int w, int h) {
    screenW_ = w; screenH_ = h;

    world_.init();
    renderer_.init(w, h);
    firebase_.init(env, activity);
    auth_.init(env, activity);

    state_ = State::SignIn;
    auth_.signIn([this](const AuthResult& r){ onAuthResult(r); });
}

void Game::resize(int w, int h) {
    screenW_ = w; screenH_ = h;
    renderer_.resize(w, h);
}

void Game::onAuthResult(const AuthResult& r) {
    if (!r.success) {
        LOGD("Auth failed: %s", r.errorMsg.c_str());
        return;
    }
    player_.uid         = r.uid;
    player_.displayName = r.displayName.empty() ? r.email : r.displayName;
    firebase_.setAuthToken(r.idToken);
    startGame();
}

void Game::startGame() {
    // Load world clock epoch from Firebase
    firebase_.getDoc("world_clock", "epoch", [this](bool ok, const std::string& body){
        if (ok && body.find("epochMs") != std::string::npos) {
            // Parse epochMs from JSON — simplified
            auto pos = body.find("\"epochMs\"");
            if (pos != std::string::npos) {
                // find the integerValue or doubleValue after it
                auto vPos = body.find("Value\":\"", pos);
                if (vPos != std::string::npos) {
                    epochMs_ = std::stoll(body.substr(vPos+8, 20));
                    gameTime_ = (float)((double)(std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::system_clock::now().time_since_epoch()).count() - epochMs_) / 1000.0);
                }
            }
        }
    });

    initFirebaseListeners();
    state_ = State::Playing;
    player_.position = {0, World::terrainHeight(0,0) + 2.0f, 0};
}

void Game::initFirebaseListeners() {
    // ── World changes ─────────────────────────────────────────────
    firebase_.listenCollection("world_changes",
        [this](const std::vector<FirebaseDoc>& docs){
            for (auto& d : docs) {
                // Parse action, x, y, z, id from d.json
                // Simplified — production uses full JSON parser
                auto extract = [&](const std::string& key) -> int {
                    auto p = d.json.find("\""+key+"\"");
                    if (p == std::string::npos) return 0;
                    auto v = d.json.find("Value\":\"", p);
                    if (v == std::string::npos) return 0;
                    return std::stoi(d.json.substr(v+8, 10));
                };
                bool isBreak = d.json.find("\"BREAK\"") != std::string::npos;
                int x = extract("x"), y = extract("y"), z = extract("z"), id = extract("id");
                world_.setBlock(x, y, z, isBreak ? 0 : id);
                int cx = (int)std::floor((float)x / CHUNK_SIZE);
                int cz = (int)std::floor((float)z / CHUNK_SIZE);
                rebuildChunk(cx, cz);
            }
        });

    // ── Other players ─────────────────────────────────────────────
    firebase_.listenCollection("players",
        [this](const std::vector<FirebaseDoc>& docs){
            for (auto& d : docs) {
                if (d.path.find(player_.uid) != std::string::npos) continue;
                // Store remote player position (simplified parse)
                std::string pid = d.path.substr(d.path.rfind('/')+1);
                auto& rp = remotePlayers_[pid];
                rp.uid = pid;
                // Production: parse full position from d.json
            }
        });
}

// ── Physics: matches JS movement + gravity + collision ────────────
void Game::updatePhysics(float dt) {
    if (state_ != State::Playing) return;

    glm::vec3& pos = player_.position;
    glm::vec3& vel = player_.velocity;

    // ── Movement input ────────────────────────────────────────────
    float speed = player_.flying ? FLY_SPEED : WALK_SPEED;
    float fwd = -input_.moveZ * speed;
    float rgt =  input_.moveX * speed;

    float sinY = sinf(player_.yaw), cosY = cosf(player_.yaw);
    float moveX = cosY * rgt + sinY * fwd;
    float moveZ = -sinY * rgt + cosY * fwd;

    vel.x = moveX;
    vel.z = moveZ;

    // ── Fly mode ──────────────────────────────────────────────────
    if (player_.flying) {
        if (input_.flyUp)   vel.y =  FLY_SPEED;
        else if (input_.flyDown) vel.y = -FLY_SPEED;
        else vel.y = 0;
    } else {
        // ── Gravity ──────────────────────────────────────────────
        vel.y += GRAVITY * dt;

        // ── Jump ─────────────────────────────────────────────────
        if (input_.jumpPressed && player_.onGround) {
            vel.y = JUMP_VEL;
            player_.onGround = false;
        }
    }

    // ── Collision (AABB sweep) ────────────────────────────────────
    auto isSolid = [&](float x, float y, float z) -> bool {
        int bi = world_.getBlock((int)std::floor(x),(int)std::floor(y),(int)std::floor(z));
        if (bi == 0) return false;
        auto it = world_.blocks().find(bi);
        return (it != world_.blocks().end() && !it->second.isFluid);
    };

    // X axis
    pos.x += vel.x * dt;
    for (int dy = 0; dy < 2; dy++) {
        float foot = pos.y - PLAYER_HEIGHT + dy;
        if (vel.x > 0 && (isSolid(pos.x+PLAYER_RADIUS, foot, pos.z-PLAYER_RADIUS) ||
                           isSolid(pos.x+PLAYER_RADIUS, foot, pos.z+PLAYER_RADIUS))) {
            pos.x = std::floor(pos.x+PLAYER_RADIUS) - PLAYER_RADIUS - 0.001f;
            vel.x = 0;
        }
        if (vel.x < 0 && (isSolid(pos.x-PLAYER_RADIUS, foot, pos.z-PLAYER_RADIUS) ||
                           isSolid(pos.x-PLAYER_RADIUS, foot, pos.z+PLAYER_RADIUS))) {
            pos.x = std::floor(pos.x-PLAYER_RADIUS) + 1.0f + PLAYER_RADIUS + 0.001f;
            vel.x = 0;
        }
    }

    // Z axis
    pos.z += vel.z * dt;
    for (int dy = 0; dy < 2; dy++) {
        float foot = pos.y - PLAYER_HEIGHT + dy;
        if (vel.z > 0 && (isSolid(pos.x-PLAYER_RADIUS, foot, pos.z+PLAYER_RADIUS) ||
                           isSolid(pos.x+PLAYER_RADIUS, foot, pos.z+PLAYER_RADIUS))) {
            pos.z = std::floor(pos.z+PLAYER_RADIUS) - PLAYER_RADIUS - 0.001f;
            vel.z = 0;
        }
        if (vel.z < 0 && (isSolid(pos.x-PLAYER_RADIUS, foot, pos.z-PLAYER_RADIUS) ||
                           isSolid(pos.x+PLAYER_RADIUS, foot, pos.z-PLAYER_RADIUS))) {
            pos.z = std::floor(pos.z-PLAYER_RADIUS) + 1.0f + PLAYER_RADIUS + 0.001f;
            vel.z = 0;
        }
    }

    // Y axis
    pos.y += vel.y * dt;
    player_.onGround = false;
    // Feet collision
    float foot = pos.y - PLAYER_HEIGHT;
    if (vel.y <= 0) {
        for (float ox : {-PLAYER_RADIUS+0.05f, PLAYER_RADIUS-0.05f}) {
            for (float oz : {-PLAYER_RADIUS+0.05f, PLAYER_RADIUS-0.05f}) {
                if (isSolid(pos.x+ox, foot, pos.z+oz)) {
                    pos.y = std::floor(foot) + 1.0f + PLAYER_HEIGHT;
                    vel.y = 0;
                    player_.onGround = true;
                    break;
                }
            }
        }
    }
    // Head collision
    if (vel.y > 0) {
        for (float ox : {-PLAYER_RADIUS+0.05f, PLAYER_RADIUS-0.05f}) {
            for (float oz : {-PLAYER_RADIUS+0.05f, PLAYER_RADIUS-0.05f}) {
                if (isSolid(pos.x+ox, pos.y+0.1f, pos.z+oz)) {
                    pos.y = std::floor(pos.y) - 0.1f;
                    vel.y = 0;
                    break;
                }
            }
        }
    }

    // Clamp to world bottom
    if (pos.y < 2.0f) { pos.y = 2.0f; vel.y = 0; }
}

void Game::updateCamera() {
    // Look rotation from touch
    player_.yaw   += input_.lookDeltaX;
    player_.pitch -= input_.lookDeltaY;
    player_.pitch  = glm::clamp(player_.pitch, -1.48f, 1.48f);

    camera_.position = player_.position;
    camera_.yaw      = player_.yaw;
    camera_.pitch    = player_.pitch;
}

// ── Chunk management ──────────────────────────────────────────────
void Game::updateChunks() {
    int pcx = (int)std::floor(player_.position.x / CHUNK_SIZE);
    int pcz = (int)std::floor(player_.position.z / CHUNK_SIZE);

    ChunkBuilder::AtlasConfig aCfg; // cols/rows set from renderer
    visibleChunks_.clear();

    for (int dx = -renderDist_; dx <= renderDist_; dx++) {
        for (int dz = -renderDist_; dz <= renderDist_; dz++) {
            int cx = pcx + dx, cz = pcz + dz;
            std::string key = chunkKey(cx, cz);

            if (!chunkRenders_.count(key)) {
                chunkRenders_[key] = std::make_unique<ChunkRender>();
                chunkRenders_[key]->cx = cx;
                chunkRenders_[key]->cz = cz;
                chunkRenders_[key]->needsRebuild = true;
            }

            auto* cr = chunkRenders_[key].get();
            if (cr->needsRebuild) rebuildChunk(cx, cz);
            visibleChunks_[key] = cr;
        }
    }

    unloadDistantChunks();
}

void Game::rebuildChunk(int cx, int cz) {
    ChunkBuilder::AtlasConfig aCfg;
    ChunkBuilder::build(cx, cz, world_, aCfg, solidBuf_, transpBuf_, foliageBuf_);

    std::string key = chunkKey(cx, cz);
    if (!chunkRenders_.count(key)) {
        chunkRenders_[key] = std::make_unique<ChunkRender>();
        chunkRenders_[key]->cx = cx;
        chunkRenders_[key]->cz = cz;
    }
    renderer_.uploadChunk(*chunkRenders_[key], solidBuf_, transpBuf_, foliageBuf_);
    chunkRenders_[key]->needsRebuild = false;
}

void Game::unloadDistantChunks() {
    int pcx = (int)std::floor(player_.position.x / CHUNK_SIZE);
    int pcz = (int)std::floor(player_.position.z / CHUNK_SIZE);
    std::vector<std::string> toRemove;
    for (auto& [key, cr] : chunkRenders_) {
        if (std::abs(cr->cx - pcx) > renderDist_+2 ||
            std::abs(cr->cz - pcz) > renderDist_+2) {
            renderer_.freeChunk(*cr);
            toRemove.push_back(key);
        }
    }
    for (auto& k : toRemove) chunkRenders_.erase(k);
}

// ── Network sync (100ms, matching JS) ────────────────────────────
void Game::syncToNetwork(float dt) {
    networkTimer_ += dt;
    if (networkTimer_ < networkRate_) return;
    networkTimer_ = 0;
    if (state_ != State::Playing || player_.uid.empty()) return;

    auto mkNum = [](double v){ return FS::numField(v); };
    auto mkStr = [](const std::string& s){ return FS::strField(s); };
    auto mkBool= [](bool b){ return FS::boolField(b); };

    std::string fields =
        "\"x\":" + mkNum(player_.position.x) +
        ",\"y\":" + mkNum(player_.position.y) +
        ",\"z\":" + mkNum(player_.position.z) +
        ",\"ry\":" + mkNum(player_.yaw) +
        ",\"name\":" + mkStr(player_.displayName) +
        ",\"health\":" + mkNum(player_.health) +
        ",\"coins\":" + mkNum(player_.coins);

    firebase_.setDoc("players", player_.uid, FS::fields(fields));
}

void Game::onTouchEvent(AInputEvent* ev) {
    touch_.onTouchEvent(ev, screenW_, screenH_);
}

void Game::tick(float dt) {
    // Advance game time
    if (epochMs_ > 0) {
        auto nowMs = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        gameTime_ = (float)((double)(nowMs - epochMs_) / 1000.0);
    } else {
        gameTime_ += dt;
    }

    touch_.update(dt, input_);
    updateCamera();
    updatePhysics(dt);
    updateChunks();
    syncToNetwork(dt);
    firebase_.tick();

    // ── Render ───────────────────────────────────────────────────
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    float aspect = (float)screenW_ / (float)screenH_;
    renderer_.drawFrame(camera_, visibleChunks_, gameTime_, aspect);
}

void Game::destroy() {
    firebase_.shutdown();
    renderer_.destroy();
}
