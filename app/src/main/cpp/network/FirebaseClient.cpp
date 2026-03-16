#include "FirebaseClient.h"
#include <android/log.h>
#include <sstream>
#include <chrono>

#define FBTAG "FirebaseClient"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, FBTAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, FBTAG, __VA_ARGS__)

// ── HTTP via JNI → Android HttpURLConnection ───────────────────────
// We call a Java helper class com.voxelpocket.HttpHelper
std::string FirebaseClient::httpRequest(
    const std::string& method,
    const std::string& url,
    const std::string& body
) {
    // Get JNI env for this thread
    JavaVM* vm = nullptr;
    // NOTE: In a real implementation you would attach the thread to the JVM
    // and call com.voxelpocket.HttpHelper.request(method, url, body, token)
    // This is the stub — filled in by the Java-side HttpHelper class.
    // See app/src/main/java/com/voxelpocket/HttpHelper.java
    return "";
}

void FirebaseClient::init(JNIEnv* env, jobject appContext) {
    env_ = env;
    ctx_ = env->NewGlobalRef(appContext);
    running_ = true;
    workerThread_ = std::thread([this]{ workerLoop(); });
}

void FirebaseClient::shutdown() {
    running_ = false;
    if (workerThread_.joinable()) workerThread_.join();
    for (auto& [k,l] : listeners_) { l->active = false; }
}

std::string FirebaseClient::buildUrl(const std::string& path, const std::string& docId) const {
    return std::string(FS_BASE) + "/" + path + "/" + docId + "?key=" + FIREBASE_API_KEY;
}

void FirebaseClient::setDoc(
    const std::string& col, const std::string& docId,
    const std::string& fieldsJson, FirebaseCallback cb
) {
    std::string url = buildUrl(col, docId);
    std::lock_guard<std::mutex> lk(mu_);
    taskQueue_.push({.method="PATCH", .url=url, .body=fieldsJson, .cb=cb});
}

void FirebaseClient::getDoc(
    const std::string& col, const std::string& docId,
    FirebaseCallback cb
) {
    std::string url = buildUrl(col, docId);
    std::lock_guard<std::mutex> lk(mu_);
    taskQueue_.push({.method="GET", .url=url, .body="", .cb=cb});
}

void FirebaseClient::deleteDoc(
    const std::string& col, const std::string& docId,
    FirebaseCallback cb
) {
    std::string url = buildUrl(col, docId);
    std::lock_guard<std::mutex> lk(mu_);
    taskQueue_.push({.method="DELETE", .url=url, .body="", .cb=cb});
}

void FirebaseClient::addDoc(
    const std::string& col, const std::string& fieldsJson,
    FirebaseCallback cb
) {
    std::string url = std::string(FS_BASE) + "/" + col + "?key=" + FIREBASE_API_KEY;
    std::lock_guard<std::mutex> lk(mu_);
    taskQueue_.push({.method="POST", .url=url, .body=fieldsJson, .cb=cb});
}

void FirebaseClient::listenCollection(
    const std::string& path,
    std::function<void(const std::vector<FirebaseDoc>&)> cb
) {
    auto l = std::make_unique<Listener>();
    l->path = path;
    l->cb   = cb;
    l->active = true;

    l->thread = std::thread([this, ptr=l.get()](){
        while (ptr->active && running_) {
            std::string url = std::string(FS_BASE) + "/" + ptr->path
                            + "?key=" + FIREBASE_API_KEY
                            + "&pageSize=200";
            std::string body = httpRequest("GET", url, "");
            if (!body.empty()) {
                auto docs = parseDocList(body);
                // Post back to main-thread queue
                // (simplified: direct call — in production use a queue)
                ptr->cb(docs);
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }
    });

    listeners_[path] = std::move(l);
}

void FirebaseClient::stopListening(const std::string& path) {
    auto it = listeners_.find(path);
    if (it != listeners_.end()) {
        it->second->active = false;
        if (it->second->thread.joinable()) it->second->thread.join();
        listeners_.erase(it);
    }
}

void FirebaseClient::workerLoop() {
    while (running_) {
        Task task;
        {
            std::lock_guard<std::mutex> lk(mu_);
            if (taskQueue_.empty()) goto sleep;
            task = std::move(taskQueue_.front());
            taskQueue_.pop();
        }
        {
            std::string result = httpRequest(task.method, task.url, task.body);
            if (task.cb) task.cb(!result.empty(), result);
        }
        sleep:
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

void FirebaseClient::tick() {
    // Drain any pending main-thread callbacks
    // (used for listeners that need to call into game logic on main thread)
}

std::vector<FirebaseDoc> FirebaseClient::parseDocList(const std::string& json) {
    // Minimal JSON parser for Firestore list response
    // In production, use nlohmann/json or rapidjson
    std::vector<FirebaseDoc> docs;
    // Placeholder — real implementation parses the JSON documents array
    return docs;
}
