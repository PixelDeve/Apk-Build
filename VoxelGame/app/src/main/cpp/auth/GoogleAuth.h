#pragma once
#include <string>
#include <functional>
#include <jni.h>

// ── Result from a Google Sign-In attempt ─────────────────────────
struct AuthResult {
    bool        success    = false;
    std::string uid;          // Firebase UID
    std::string email;
    std::string displayName;
    std::string photoUrl;
    std::string idToken;      // Firebase ID token — passed to FirebaseClient
    std::string errorMsg;
};

using AuthCallback = std::function<void(const AuthResult&)>;

// ────────────────────────────────────────────────────────────────────
//  GoogleAuth
//  Triggers Google Sign-In via JNI → Activity.
//  The Java Activity calls back into C++ via JNI when auth completes.
// ────────────────────────────────────────────────────────────────────
class GoogleAuth {
public:
    // Must be called from the main thread after the Activity is created
    void init(JNIEnv* env, jobject activity);

    // Trigger the Google Sign-In popup (starts Activity result flow)
    void signIn(AuthCallback cb);

    // Sign out
    void signOut(std::function<void()> cb = {});

    // Called by Java Activity after sign-in completes (via JNI callback)
    // Do not call from C++ directly
    void _onAuthResult(const AuthResult& result);

    bool isSignedIn() const { return signedIn_; }
    const AuthResult& currentUser() const { return currentUser_; }

private:
    JNIEnv*  env_      = nullptr;
    jobject  activity_ = nullptr;  // Global ref

    bool       signedIn_   = false;
    AuthResult currentUser_;
    AuthCallback pendingCb_;
};

// ── Global instance accessible from JNI callbacks ─────────────────
extern GoogleAuth* g_googleAuth;
