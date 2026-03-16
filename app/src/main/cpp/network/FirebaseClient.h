#pragma once
#include <string>
#include <functional>
#include <thread>
#include <mutex>
#include <queue>
#include <atomic>
#include <jni.h>

// ── Firebase project config (matches JS source) ───────────────────
static const char* FIREBASE_PROJECT  = "pixelbox-bca1b";
static const char* FIREBASE_API_KEY  = "AIzaSyBgQaeWlxecgunDZo8xHBorRVIrZ8K4YJ8";
static const char* APP_ID            = "pixelbox_v1";

// Base Firestore URL
#define FS_BASE "https://firestore.googleapis.com/v1/projects/" FIREBASE_PROJECT "/databases/(default)/documents/artifacts/" APP_ID "/public/data"
// Realtime DB (for presence ping)
#define RTDB_BASE "https://pixelbox-bca1b-default-rtdb.firebaseio.com"

// ────────────────────────────────────────────────────────────────────
//  FirebaseClient
//  Uses Android's java.net.HttpURLConnection via JNI.
//  All network calls are run on a background thread.
//  Callbacks are posted back to a queue, consumed on main thread.
// ────────────────────────────────────────────────────────────────────
struct FirebaseDoc {
    std::string path;   // e.g. "players/uid123"
    std::string json;   // serialised field map
};

using FirebaseCallback = std::function<void(bool ok, const std::string& body)>;

class FirebaseClient {
public:
    // Call once from the main thread after JNI_OnLoad
    void init(JNIEnv* env, jobject appContext);

    // Set the auth token (obtained from Google Sign-In via JNI)
    void setAuthToken(const std::string& idToken) {
        std::lock_guard<std::mutex> lk(mu_);
        authToken_ = idToken;
    }

    // ── Document operations (async) ──────────────────────────────
    void setDoc(const std::string& collectionPath,
                const std::string& docId,
                const std::string& fieldsJson,
                FirebaseCallback cb = {});

    void getDoc(const std::string& collectionPath,
                const std::string& docId,
                FirebaseCallback cb);

    void deleteDoc(const std::string& collectionPath,
                   const std::string& docId,
                   FirebaseCallback cb = {});

    void addDoc(const std::string& collectionPath,
                const std::string& fieldsJson,
                FirebaseCallback cb = {});

    // ── Collection listener (polling, 1s interval) ───────────────
    //  onSnapshot equivalent — fires cb each poll with full list
    void listenCollection(const std::string& collectionPath,
                          std::function<void(const std::vector<FirebaseDoc>&)> cb);

    void stopListening(const std::string& collectionPath);

    // ── Drain pending callbacks on the main thread ───────────────
    void tick();

    // ── Shutdown ─────────────────────────────────────────────────
    void shutdown();

private:
    // JNI refs
    JNIEnv*  env_    = nullptr;
    jobject  ctx_    = nullptr;

    std::string authToken_;
    std::mutex  mu_;

    // Thread pool
    struct Task {
        std::string method;  // GET POST PATCH DELETE
        std::string url;
        std::string body;
        FirebaseCallback cb;
    };
    std::queue<Task>             taskQueue_;
    std::queue<std::pair<bool,std::string>> resultQueue_;
    std::queue<FirebaseCallback> cbQueue_;

    std::thread workerThread_;
    std::atomic<bool> running_{false};

    // ── Polling listeners ────────────────────────────────────────
    struct Listener {
        std::string path;
        std::function<void(const std::vector<FirebaseDoc>&)> cb;
        std::string lastPageToken;
        std::thread thread;
        std::atomic<bool> active{false};
    };
    std::unordered_map<std::string, std::unique_ptr<Listener>> listeners_;

    // Worker functions
    std::string httpRequest(const std::string& method,
                            const std::string& url,
                            const std::string& body);
    std::string buildUrl(const std::string& path, const std::string& docId) const;
    void workerLoop();
    std::vector<FirebaseDoc> parseDocList(const std::string& json);
    std::string buildFieldsJson(const std::string& raw);
};

// ────────────────────────────────────────────────────────────────────
//  Helpers for constructing Firestore REST field maps
//  (Firestore REST uses {"fields":{"key":{"stringValue":"val"},...}})
// ────────────────────────────────────────────────────────────────────
namespace FS {
    inline std::string strField(const std::string& val) {
        return "{\"stringValue\":\"" + val + "\"}";
    }
    inline std::string numField(double val) {
        return "{\"doubleValue\":" + std::to_string(val) + "}";
    }
    inline std::string boolField(bool val) {
        return std::string("{\"booleanValue\":") + (val?"true":"false") + "}";
    }
    // Build a minimal fields object string
    inline std::string fields(const std::string& inner) {
        return "{\"fields\":{" + inner + "}}";
    }
}
