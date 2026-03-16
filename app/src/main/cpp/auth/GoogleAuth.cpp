#include "GoogleAuth.h"
#include <android/log.h>

#define AUTHTAG "GoogleAuth"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, AUTHTAG, __VA_ARGS__)

GoogleAuth* g_googleAuth = nullptr;

void GoogleAuth::init(JNIEnv* env, jobject activity) {
    env_      = env;
    activity_ = env->NewGlobalRef(activity);
    g_googleAuth = this;
}

void GoogleAuth::signIn(AuthCallback cb) {
    pendingCb_ = cb;

    // Trigger Java-side Google Sign-In
    // Java method: void startGoogleSignIn()
    jclass cls = env_->GetObjectClass(activity_);
    jmethodID mid = env_->GetMethodID(cls, "startGoogleSignIn", "()V");
    if (mid) {
        env_->CallVoidMethod(activity_, mid);
    } else {
        LOGD("startGoogleSignIn method not found — check MainActivity.java");
        AuthResult r;
        r.success  = false;
        r.errorMsg = "startGoogleSignIn not found in Activity";
        if (pendingCb_) pendingCb_(r);
    }
}

void GoogleAuth::signOut(std::function<void()> cb) {
    jclass cls = env_->GetObjectClass(activity_);
    jmethodID mid = env_->GetMethodID(cls, "signOut", "()V");
    if (mid) env_->CallVoidMethod(activity_, mid);
    signedIn_ = false;
    currentUser_ = {};
    if (cb) cb();
}

void GoogleAuth::_onAuthResult(const AuthResult& result) {
    signedIn_    = result.success;
    currentUser_ = result;
    if (pendingCb_) {
        pendingCb_(result);
        pendingCb_ = {};
    }
}

// ────────────────────────────────────────────────────────────────────
//  JNI entry point called from Java after Google Sign-In completes
//  Java signature:
//    native void onAuthResult(String uid, String email, String name,
//                              String photo, String token, boolean ok, String err);
// ────────────────────────────────────────────────────────────────────
extern "C" JNIEXPORT void JNICALL
Java_com_voxelpocket_MainActivity_onAuthResult(
    JNIEnv* env, jobject /*thiz*/,
    jstring uid, jstring email, jstring name,
    jstring photo, jstring token,
    jboolean ok, jstring err
) {
    auto jstr = [&](jstring j) -> std::string {
        if (!j) return "";
        const char* c = env->GetStringUTFChars(j, nullptr);
        std::string s(c);
        env->ReleaseStringUTFChars(j, c);
        return s;
    };

    AuthResult r;
    r.success     = (bool)ok;
    r.uid         = jstr(uid);
    r.email       = jstr(email);
    r.displayName = jstr(name);
    r.photoUrl    = jstr(photo);
    r.idToken     = jstr(token);
    r.errorMsg    = jstr(err);

    if (g_googleAuth) g_googleAuth->_onAuthResult(r);
}
