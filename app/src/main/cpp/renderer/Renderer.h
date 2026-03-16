#pragma once
#include <GLES3/gl3.h>
#include <string>
#include <unordered_map>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "Chunk.h"
#include "World.h"

// ── Render constants ──────────────────────────────────────────────
static constexpr int RENDER_DIST_DEFAULT = 3;   // chunks
static constexpr float FOG_DENSITY       = 0.009f;
static constexpr float SKY_COLOR_R       = 0.471f;
static constexpr float SKY_COLOR_G       = 0.655f;
static constexpr float SKY_COLOR_B       = 1.0f;

// ── Camera ────────────────────────────────────────────────────────
struct Camera {
    glm::vec3 position = {0, 15, 0};
    float yaw   = 0.0f;   // radians
    float pitch = 0.0f;
    float fovDeg = 75.0f;

    glm::mat4 viewMatrix() const;
    glm::mat4 projMatrix(float aspect) const;
    glm::vec3 forward() const;
};

// ── Per-chunk GPU data ─────────────────────────────────────────────
struct ChunkRender {
    ChunkGPU gpu;
    int cx, cz;
    bool needsRebuild = true;
};

// ── Renderer ──────────────────────────────────────────────────────
class Renderer {
public:
    bool init(int screenW, int screenH);
    void resize(int w, int h);
    void destroy();

    // Upload CPU mesh to GPU
    void uploadChunk(ChunkRender& cr,
                     const MeshData& solid,
                     const MeshData& transp,
                     const FoliageMesh& foliage);

    void freeChunk(ChunkRender& cr);

    // Draw a full frame
    void drawFrame(
        const Camera& cam,
        std::unordered_map<std::string, ChunkRender*>& chunks,
        float gameTime,
        float aspectRatio
    );

    // Load texture from raw RGBA pixels (for atlas)
    GLuint loadTexture(const unsigned char* pixels, int w, int h);
    GLuint loadTextureFromFile(const std::string& path);

    GLuint atlasTexture    = 0;
    GLuint grassPlantTex   = 0;

    int screenW = 0, screenH = 0;

private:
    GLuint solidShader_    = 0;
    GLuint foliageShader_  = 0;
    GLuint skyShader_      = 0;

    GLuint skyVAO_ = 0, skyVBO_ = 0;

    void buildSkyGeometry();
    void compileSkyShader();
    void compileSolidShader();
    void compileFoliageShader();

    GLuint compileShader(const char* vert, const char* frag);
    void updateSunMoon(float gameTime, const Camera& cam);

    glm::vec3 sunDir_  = {0.5f, 1.0f, 0.3f};
    glm::vec3 skyColorCurrent_ = {SKY_COLOR_R, SKY_COLOR_G, SKY_COLOR_B};
};
