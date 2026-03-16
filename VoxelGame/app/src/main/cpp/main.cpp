#include <jni.h>
#include <android/log.h>
#include <android/native_window_jni.h>
#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <android/input.h>
#include <chrono>
#include <memory>
#include "game/Game.h"

#define TAG "VoxelMain"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)

// ── Global game instance ──────────────────────────────────────────
static std::unique_ptr<Game> g_game;
static JavaVM* g_jvm = nullptr;

// ── EGL context ──────────────────────────────────────────────────
static EGLDisplay g_display = EGL_NO_DISPLAY;
static EGLSurface g_surface = EGL_NO_SURFACE;
static EGLContext g_context = EGL_NO_CONTEXT;
static ANativeWindow* g_window = nullptr;

static bool eglInit(ANativeWindow* window) {
    g_display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    eglInitialize(g_display, nullptr, nullptr);

    const EGLint cfgAttribs[] = {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
        EGL_RED_SIZE, 8, EGL_GREEN_SIZE, 8, EGL_BLUE_SIZE, 8,
        EGL_DEPTH_SIZE, 24,
        EGL_NONE
    };
    EGLConfig config; EGLint numCfg;
    eglChooseConfig(g_display, cfgAttribs, &config, 1, &numCfg);

    const EGLint ctxAttribs[] = { EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE };
    g_context = eglCreateContext(g_display, config, EGL_NO_CONTEXT, ctxAttribs);
    g_surface = eglCreateWindowSurface(g_display, config, window, nullptr);
    eglMakeCurrent(g_display, g_surface, g_surface, g_context);
    return true;
}

static void eglTeardown() {
    if (g_display != EGL_NO_DISPLAY) {
        eglMakeCurrent(g_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        if (g_context != EGL_NO_CONTEXT) eglDestroyContext(g_display, g_context);
        if (g_surface != EGL_NO_SURFACE) eglDestroySurface(g_display, g_surface);
        eglTerminate(g_display);
    }
    g_display = EGL_NO_DISPLAY;
    g_surface = EGL_NO_SURFACE;
    g_context = EGL_NO_CONTEXT;
}

// ────────────────────────────────────────────────────────────────────
//  JNI API called from MainActivity.java
// ────────────────────────────────────────────────────────────────────

extern "C" JNIEXPORT jint JNI_OnLoad(JavaVM* vm, void*) {
    g_jvm = vm;
    return JNI_VERSION_1_6;
}

extern "C" JNIEXPORT void JNICALL
Java_com_voxelpocket_MainActivity_nativeInit(
    JNIEnv* env, jobject activity,
    jobject surface, jint w, jint h
) {
    g_window = ANativeWindow_fromSurface(env, surface);
    eglInit(g_window);

    g_game = std::make_unique<Game>();
    g_game->init(env, activity, w, h);
    LOGD("Game initialised %dx%d", w, h);
}

extern "C" JNIEXPORT void JNICALL
Java_com_voxelpocket_MainActivity_nativeResize(
    JNIEnv*, jobject, jint w, jint h
) {
    if (g_game) g_game->resize(w, h);
}

extern "C" JNIEXPORT void JNICALL
Java_com_voxelpocket_MainActivity_nativeFrame(JNIEnv*, jobject) {
    static auto last = std::chrono::steady_clock::now();
    auto now = std::chrono::steady_clock::now();
    float dt = std::chrono::duration<float>(now - last).count();
    last = now;
    dt = std::min(dt, 0.05f); // cap at 50ms

    if (g_game) {
        g_game->tick(dt);
        eglSwapBuffers(g_display, g_surface);
    }
}

extern "C" JNIEXPORT void JNICALL
Java_com_voxelpocket_MainActivity_nativeTouchEvent(
    JNIEnv* env, jobject, jobject motionEvent
) {
    // In production: wrap the MotionEvent into an AInputEvent
    // and pass to g_game->onTouchEvent()
    // This requires NDK AInputEvent bridge
}

extern "C" JNIEXPORT void JNICALL
Java_com_voxelpocket_MainActivity_nativeDestroy(JNIEnv*, jobject) {
    if (g_game) { g_game->destroy(); g_game.reset(); }
    eglTeardown();
    if (g_window) { ANativeWindow_release(g_window); g_window = nullptr; }
}
