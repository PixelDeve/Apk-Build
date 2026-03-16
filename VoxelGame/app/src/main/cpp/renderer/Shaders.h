#pragma once

// ─────────────────────────────────────────────────────────────────
//  SOLID / TRANSPARENT BLOCK SHADER
// ─────────────────────────────────────────────────────────────────
static const char* SOLID_VERT = R"GLSL(
#version 300 es
precision highp float;

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aUV;
layout(location = 3) in vec3 aColor;   // vertex AO

uniform mat4 uMVP;
uniform mat4 uModel;
uniform vec3 uSunDir;
uniform float uAmbient;

out vec2  vUV;
out float vLight;
out float vFogDepth;

void main() {
    vec4 worldPos = uModel * vec4(aPos, 1.0);
    gl_Position   = uMVP * vec4(aPos, 1.0);

    // Diffuse lighting
    vec3 N = normalize(mat3(uModel) * aNormal);
    float diff = max(dot(N, normalize(uSunDir)), 0.0);

    // Vertex AO stored in aColor.r (0.5 – 1.0)
    vLight    = (uAmbient + diff * (1.0 - uAmbient)) * aColor.r;
    vUV       = aUV;
    vFogDepth = gl_Position.w;
}
)GLSL";

static const char* SOLID_FRAG = R"GLSL(
#version 300 es
precision highp float;

in vec2  vUV;
in float vLight;
in float vFogDepth;

uniform sampler2D uAtlas;
uniform vec3      uFogColor;
uniform float     uFogDensity;
uniform float     uAlpha;     // 1.0 solid, <1.0 transparent

out vec4 FragColor;

void main() {
    vec4 col = texture(uAtlas, vUV);
    if (col.a < 0.1) discard;

    col.rgb *= vLight;
    col.a   *= uAlpha;

    // Exponential fog matching Three.js FogExp2
    float fogFactor = exp(-uFogDensity * vFogDepth * uFogDensity * vFogDepth);
    fogFactor = clamp(fogFactor, 0.0, 1.0);
    col.rgb = mix(uFogColor, col.rgb, fogFactor);

    FragColor = col;
}
)GLSL";

// ─────────────────────────────────────────────────────────────────
//  FOLIAGE (GRASS PLANT) BILLBOARD SHADER
//  Uses alpha testing, double-sided, no backface culling
// ─────────────────────────────────────────────────────────────────
static const char* FOLIAGE_VERT = R"GLSL(
#version 300 es
precision highp float;

layout(location = 0) in vec3  aPos;
layout(location = 1) in vec2  aUV;
layout(location = 2) in float aAO;

uniform mat4  uMVP;
uniform float uAmbient;
uniform vec3  uSunDir;

out vec2  vUV;
out float vLight;
out float vFogDepth;

void main() {
    gl_Position = uMVP * vec4(aPos, 1.0);
    // Foliage gets a fixed soft light — no normal
    vLight    = uAmbient + 0.35 * aAO;
    vUV       = aUV;
    vFogDepth = gl_Position.w;
}
)GLSL";

static const char* FOLIAGE_FRAG = R"GLSL(
#version 300 es
precision highp float;

in vec2  vUV;
in float vLight;
in float vFogDepth;

uniform sampler2D uTex;
uniform vec3      uFogColor;
uniform float     uFogDensity;

out vec4 FragColor;

void main() {
    vec4 col = texture(uTex, vUV);
    if (col.a < 0.5) discard;   // alpha test

    col.rgb *= vLight;

    float fogFactor = exp(-uFogDensity * vFogDepth * uFogDensity * vFogDepth);
    fogFactor = clamp(fogFactor, 0.0, 1.0);
    col.rgb = mix(uFogColor, col.rgb, fogFactor);

    FragColor = col;
}
)GLSL";

// ─────────────────────────────────────────────────────────────────
//  SKY SHADER  (full-screen quad, sky colour blended toward horizon)
// ─────────────────────────────────────────────────────────────────
static const char* SKY_VERT = R"GLSL(
#version 300 es
precision highp float;
layout(location=0) in vec2 aPos;
out vec2 vPos;
void main() {
    gl_Position = vec4(aPos, 0.999, 1.0);
    vPos = aPos;
}
)GLSL";

static const char* SKY_FRAG = R"GLSL(
#version 300 es
precision highp float;
in vec2 vPos;
uniform vec3 uSkyTop;
uniform vec3 uSkyHorizon;
out vec4 FragColor;
void main() {
    float t = clamp(vPos.y * 0.5 + 0.5, 0.0, 1.0);
    FragColor = vec4(mix(uSkyHorizon, uSkyTop, t * t), 1.0);
}
)GLSL";
