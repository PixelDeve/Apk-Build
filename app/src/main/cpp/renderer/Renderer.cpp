#include "Renderer.h"
#include "Shaders.h"
#include <android/log.h>
#include <cmath>
#include <vector>

#define LOG_TAG "VoxelRenderer"
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// ── Camera helpers ────────────────────────────────────────────────
glm::mat4 Camera::viewMatrix() const {
    glm::vec3 fwd(
        cosf(pitch) * sinf(yaw),
        sinf(pitch),
        cosf(pitch) * cosf(yaw)
    );
    return glm::lookAt(position, position + fwd, glm::vec3(0,1,0));
}
glm::mat4 Camera::projMatrix(float aspect) const {
    return glm::perspective(glm::radians(fovDeg), aspect, 0.1f, 200.0f);
}
glm::vec3 Camera::forward() const {
    return glm::vec3(
        cosf(pitch)*sinf(yaw), sinf(pitch), cosf(pitch)*cosf(yaw)
    );
}

// ── Shader compilation ────────────────────────────────────────────
GLuint Renderer::compileShader(const char* vert, const char* frag) {
    auto compile = [](GLenum type, const char* src) -> GLuint {
        GLuint s = glCreateShader(type);
        glShaderSource(s, 1, &src, nullptr);
        glCompileShader(s);
        GLint ok; glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
        if (!ok) {
            char buf[512]; glGetShaderInfoLog(s, 512, nullptr, buf);
            LOGE("Shader error: %s", buf);
        }
        return s;
    };
    GLuint v = compile(GL_VERTEX_SHADER,   vert);
    GLuint f = compile(GL_FRAGMENT_SHADER, frag);
    GLuint prog = glCreateProgram();
    glAttachShader(prog, v); glAttachShader(prog, f);
    glLinkProgram(prog);
    glDeleteShader(v); glDeleteShader(f);
    return prog;
}

void Renderer::compileSolidShader()   { solidShader_   = compileShader(SOLID_VERT,   SOLID_FRAG);   }
void Renderer::compileFoliageShader() { foliageShader_ = compileShader(FOLIAGE_VERT, FOLIAGE_FRAG); }
void Renderer::compileSkyShader()     { skyShader_      = compileShader(SKY_VERT,     SKY_FRAG);     }

void Renderer::buildSkyGeometry() {
    // Fullscreen quad
    float verts[] = {-1,-1, 1,-1, -1,1, 1,-1, 1,1, -1,1};
    glGenVertexArrays(1, &skyVAO_); glGenBuffers(1, &skyVBO_);
    glBindVertexArray(skyVAO_);
    glBindBuffer(GL_ARRAY_BUFFER, skyVBO_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2*sizeof(float), nullptr);
    glBindVertexArray(0);
}

bool Renderer::init(int w, int h) {
    screenW = w; screenH = h;
    compileSolidShader();
    compileFoliageShader();
    compileSkyShader();
    buildSkyGeometry();
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    return true;
}

void Renderer::resize(int w, int h) {
    screenW = w; screenH = h;
    glViewport(0, 0, w, h);
}

void Renderer::destroy() {
    glDeleteProgram(solidShader_);
    glDeleteProgram(foliageShader_);
    glDeleteProgram(skyShader_);
    glDeleteVertexArrays(1, &skyVAO_);
    glDeleteBuffers(1, &skyVBO_);
}

// ── Texture loading from raw RGBA ─────────────────────────────────
GLuint Renderer::loadTexture(const unsigned char* pixels, int w, int h) {
    GLuint tex; glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    return tex;
}

// ── Upload CPU mesh data to GPU buffers ───────────────────────────
static void uploadMesh(
    GLuint& vao, GLuint& vbo, GLuint& ebo, int& count,
    const MeshData& mesh
) {
    if (mesh.indices.empty()) { count = 0; return; }
    if (!vao) { glGenVertexArrays(1,&vao); glGenBuffers(1,&vbo); glGenBuffers(1,&ebo); }

    // Interleave: pos(3) + norm(3) + uv(2) + color(3) = 11 floats
    int nv = mesh.vertexCount;
    std::vector<float> interleaved;
    interleaved.reserve(nv * 11);
    for (int i = 0; i < nv; i++) {
        interleaved.push_back(mesh.positions[i*3]);
        interleaved.push_back(mesh.positions[i*3+1]);
        interleaved.push_back(mesh.positions[i*3+2]);
        interleaved.push_back(mesh.normals[i*3]);
        interleaved.push_back(mesh.normals[i*3+1]);
        interleaved.push_back(mesh.normals[i*3+2]);
        interleaved.push_back(mesh.uvs[i*2]);
        interleaved.push_back(mesh.uvs[i*2+1]);
        interleaved.push_back(mesh.colors[i*3]);
        interleaved.push_back(mesh.colors[i*3+1]);
        interleaved.push_back(mesh.colors[i*3+2]);
    }

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, interleaved.size()*sizeof(float), interleaved.data(), GL_DYNAMIC_DRAW);

    const int stride = 11 * sizeof(float);
    glEnableVertexAttribArray(0); glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,stride,(void*)0);
    glEnableVertexAttribArray(1); glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,stride,(void*)(3*sizeof(float)));
    glEnableVertexAttribArray(2); glVertexAttribPointer(2,2,GL_FLOAT,GL_FALSE,stride,(void*)(6*sizeof(float)));
    glEnableVertexAttribArray(3); glVertexAttribPointer(3,3,GL_FLOAT,GL_FALSE,stride,(void*)(8*sizeof(float)));

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh.indices.size()*sizeof(int), mesh.indices.data(), GL_DYNAMIC_DRAW);
    count = (int)mesh.indices.size();
    glBindVertexArray(0);
}

static void uploadFoliage(
    GLuint& vao, GLuint& vbo, GLuint& ebo, int& count,
    const FoliageMesh& mesh
) {
    if (mesh.indices.empty()) { count = 0; return; }
    if (!vao) { glGenVertexArrays(1,&vao); glGenBuffers(1,&vbo); glGenBuffers(1,&ebo); }

    // layout: pos(3) + uv(2) + ao(1) = 6 floats
    std::vector<float> flat;
    flat.reserve(mesh.verts.size() * 6);
    for (auto& v : mesh.verts) {
        flat.push_back(v.x); flat.push_back(v.y); flat.push_back(v.z);
        flat.push_back(v.u); flat.push_back(v.v);
        flat.push_back(v.ao);
    }

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, flat.size()*sizeof(float), flat.data(), GL_DYNAMIC_DRAW);
    const int stride = 6*sizeof(float);
    glEnableVertexAttribArray(0); glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,stride,(void*)0);
    glEnableVertexAttribArray(1); glVertexAttribPointer(1,2,GL_FLOAT,GL_FALSE,stride,(void*)(3*sizeof(float)));
    glEnableVertexAttribArray(2); glVertexAttribPointer(2,1,GL_FLOAT,GL_FALSE,stride,(void*)(5*sizeof(float)));

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh.indices.size()*sizeof(int), mesh.indices.data(), GL_DYNAMIC_DRAW);
    count = (int)mesh.indices.size();
    glBindVertexArray(0);
}

void Renderer::uploadChunk(ChunkRender& cr,
                            const MeshData& solid,
                            const MeshData& transp,
                            const FoliageMesh& foliage) {
    uploadMesh(cr.gpu.solidVAO, cr.gpu.solidVBO, cr.gpu.solidEBO, cr.gpu.solidCount, solid);
    uploadMesh(cr.gpu.transpVAO, cr.gpu.transpVBO, cr.gpu.transpEBO, cr.gpu.transpCount, transp);
    uploadFoliage(cr.gpu.foliageVAO, cr.gpu.foliageVBO, cr.gpu.foliageEBO, cr.gpu.foliageCount, foliage);
    cr.gpu.dirty = false;
}

void Renderer::freeChunk(ChunkRender& cr) {
    glDeleteVertexArrays(1,&cr.gpu.solidVAO);  glDeleteBuffers(1,&cr.gpu.solidVBO);  glDeleteBuffers(1,&cr.gpu.solidEBO);
    glDeleteVertexArrays(1,&cr.gpu.transpVAO); glDeleteBuffers(1,&cr.gpu.transpVBO); glDeleteBuffers(1,&cr.gpu.transpEBO);
    glDeleteVertexArrays(1,&cr.gpu.foliageVAO);glDeleteBuffers(1,&cr.gpu.foliageVBO);glDeleteBuffers(1,&cr.gpu.foliageEBO);
    cr.gpu = ChunkGPU{};
}

// ── Sky colour logic (day/night cycle) ────────────────────────────
static void calcSkyColors(float gt,
                           glm::vec3& skyTop, glm::vec3& skyHorizon,
                           glm::vec3& sunDir, float& ambient) {
    float angle = (gt / DAY_LENGTH_SECONDS) * 2.0f * 3.14159f;
    float sunY  = sinf(angle);
    float dayT  = glm::clamp((sunY + 0.2f) / 0.4f, 0.0f, 1.0f);

    // Day: #78A7FF  Night: #050510
    skyTop     = glm::mix(glm::vec3(0.02f,0.02f,0.06f), glm::vec3(0.47f,0.65f,1.0f), dayT);
    skyHorizon = glm::mix(glm::vec3(0.04f,0.04f,0.08f), glm::vec3(0.78f,0.85f,1.0f), dayT);
    ambient    = glm::mix(0.1f, 0.6f, dayT);

    // Sun direction rotates around X axis
    float sx = cosf(angle), sy = sunY, sz = 0.3f;
    sunDir = glm::normalize(glm::vec3(sx, sy, sz));
}

// ── Full frame render ─────────────────────────────────────────────
void Renderer::drawFrame(
    const Camera& cam,
    std::unordered_map<std::string, ChunkRender*>& chunks,
    float gameTime,
    float aspectRatio
) {
    glm::vec3 skyTop, skyHorizon, sunDir;
    float ambient;
    calcSkyColors(gameTime, skyTop, skyHorizon, sunDir, ambient);
    skyColorCurrent_ = skyHorizon;

    // ── Sky ────────────────────────────────────────────────────
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glUseProgram(skyShader_);
    glUniform3fv(glGetUniformLocation(skyShader_,"uSkyTop"),    1, glm::value_ptr(skyTop));
    glUniform3fv(glGetUniformLocation(skyShader_,"uSkyHorizon"),1, glm::value_ptr(skyHorizon));
    glBindVertexArray(skyVAO_);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    // ── MVP ────────────────────────────────────────────────────
    glm::mat4 proj = cam.projMatrix(aspectRatio);
    glm::mat4 view = cam.viewMatrix();
    glm::mat4 mvp  = proj * view;
    glm::mat4 model = glm::mat4(1.0f);

    // ── Solid chunks ───────────────────────────────────────────
    glUseProgram(solidShader_);
    glUniformMatrix4fv(glGetUniformLocation(solidShader_,"uMVP"),  1,GL_FALSE,glm::value_ptr(mvp));
    glUniformMatrix4fv(glGetUniformLocation(solidShader_,"uModel"),1,GL_FALSE,glm::value_ptr(model));
    glUniform3fv(glGetUniformLocation(solidShader_,"uSunDir"),     1,glm::value_ptr(sunDir));
    glUniform1f (glGetUniformLocation(solidShader_,"uAmbient"),    ambient);
    glUniform3fv(glGetUniformLocation(solidShader_,"uFogColor"),   1,glm::value_ptr(skyHorizon));
    glUniform1f (glGetUniformLocation(solidShader_,"uFogDensity"), FOG_DENSITY);
    glUniform1f (glGetUniformLocation(solidShader_,"uAlpha"),      1.0f);
    glUniform1i (glGetUniformLocation(solidShader_,"uAtlas"),      0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, atlasTexture);

    for (auto& [key, cr] : chunks) {
        if (!cr || cr->gpu.solidCount == 0) continue;
        glBindVertexArray(cr->gpu.solidVAO);
        glDrawElements(GL_TRIANGLES, cr->gpu.solidCount, GL_UNSIGNED_INT, nullptr);
    }

    // ── Transparent chunks ─────────────────────────────────────
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glUniform1f(glGetUniformLocation(solidShader_,"uAlpha"), 0.75f);
    for (auto& [key, cr] : chunks) {
        if (!cr || cr->gpu.transpCount == 0) continue;
        glBindVertexArray(cr->gpu.transpVAO);
        glDrawElements(GL_TRIANGLES, cr->gpu.transpCount, GL_UNSIGNED_INT, nullptr);
    }

    // ── Grass plant foliage ────────────────────────────────────
    glUseProgram(foliageShader_);
    glUniformMatrix4fv(glGetUniformLocation(foliageShader_,"uMVP"),      1,GL_FALSE,glm::value_ptr(mvp));
    glUniform1f (glGetUniformLocation(foliageShader_,"uAmbient"),        ambient);
    glUniform3fv(glGetUniformLocation(foliageShader_,"uSunDir"),         1,glm::value_ptr(sunDir));
    glUniform3fv(glGetUniformLocation(foliageShader_,"uFogColor"),       1,glm::value_ptr(skyHorizon));
    glUniform1f (glGetUniformLocation(foliageShader_,"uFogDensity"),     FOG_DENSITY);
    glUniform1i (glGetUniformLocation(foliageShader_,"uTex"),            0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, grassPlantTex ? grassPlantTex : atlasTexture);

    for (auto& [key, cr] : chunks) {
        if (!cr || cr->gpu.foliageCount == 0) continue;
        glBindVertexArray(cr->gpu.foliageVAO);
        glDrawElements(GL_TRIANGLES, cr->gpu.foliageCount, GL_UNSIGNED_INT, nullptr);
    }

    glDisable(GL_BLEND);
    glEnable(GL_CULL_FACE);
    glBindVertexArray(0);
}
