cmake_minimum_required(VERSION 3.22.1)
project(voxelpocket)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ── GLM (header-only) ─────────────────────────────────────────────
# Place glm/ inside app/src/main/cpp/third_party/
include_directories(
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/third_party
    ${CMAKE_SOURCE_DIR}/third_party/glm
)

# ── Source files ──────────────────────────────────────────────────
add_library(
    voxelpocket SHARED

    # Entry point
    main.cpp

    # Game
    game/Game.cpp

    # World
    world/World.cpp
    world/Chunk.cpp

    # Renderer
    renderer/Renderer.cpp

    # Network
    network/FirebaseClient.cpp

    # Auth
    auth/GoogleAuth.cpp

    # Input
    input/TouchInput.cpp
)

# ── Android system libraries ──────────────────────────────────────
find_library(log-lib    log)
find_library(android-lib android)
find_library(egl-lib    EGL)
find_library(gles3-lib  GLESv3)

target_link_libraries(
    voxelpocket
    ${log-lib}
    ${android-lib}
    ${egl-lib}
    ${gles3-lib}
)

# ── Compiler flags ────────────────────────────────────────────────
target_compile_options(voxelpocket PRIVATE
    -O2
    -ffast-math
    -Wall
    -Wno-unused-variable
)
