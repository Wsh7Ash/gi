#include "voice_verification.h"
#include "app/app_logger.h"

#include <spdlog/spdlog.h>
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <fstream>
#include <chrono>
#include <thread>
#include <filesystem>
#include <random>

namespace gmail_infinity {

using json = nlohmann::json;
namespace fs = std::filesystem;

static size_t vv_write_cb(void* ptr, size_t sz, size_t nmb, std::string* out) {
    out->append(static_cast<char*>(ptr), sz * nmb); return sz * nmb;
}
static size_t vv_file_cb(void* ptr, size_t sz, size_t nmb, std::ofstream* f) {
    f->write(static_cast<char*>(ptr), static_cast<std::streamsize>(sz * nmb));
    return sz * nmb;
}

static std::string vv_post(const std::string& url, const std::string& body,
    const std::vector<std::string>& hdrs) {
    CURL* c = curl_easy_init(); if (!c) return "";
    std::string resp;
    struct curl_slist* h = nullptr;
    for (auto& s : hdrs) h = curl_slist_append(h, s.c_str());
    curl_easy_setopt(c, CURLOPT_URL, url.c_str());
    curl_easy_setopt(c, CURLOPT_HTTPHEADER, h);
    curl_easy_setopt(c, CURLOPT_POST, 1L);
    curl_easy_setopt(c, CURLOPT_POSTFIELDS, body.c_str());
    curl_easy_setopt(c, CURLOPT_WRITEFUNCTION, vv_write_cb);
    curl_easy_setopt(c, CURLOPT_WRITEDATA, &resp);
    curl_easy_setopt(c, CURLOPT_TIMEOUT, 60L);
    curl_easy_perform(c);
    if (h) curl_slist_free_all(h);
    curl_easy_cleanup(c);
    return resp;
}

// ─────────────────────────────────────────────────────────
// ElevenLabsTTSProvider
// ─────────────────────────────────────────────────────────

ElevenLabsTTSProvider::ElevenLabsTTSProvider(const std::string& api_key)
    : api_key_(api_key) {
    curl_global_init(CURL_GLOBAL_DEFAULT);
}

ElevenLabsTTSProvider::~ElevenLabsTTSProvider() {
    curl_global_cleanup();
}

TTSResult ElevenLabsTTSProvider::synthesize(const std::string& text,
    const std::string& voice_id, const std::string& output_path) {
    TTSResult result;
    json payload = {
        {"text", text},
        {"model_id", "eleven_monolingual_v1"},
        {"voice_settings", {{"stability", 0.5}, {"similarity_boost", 0.75}}}
    };
    std::string url = "https://api.elevenlabs.io/v1/text-to-speech/"
                    + (voice_id.empty() ? default_voice_id_ : voice_id);

    CURL* c = curl_easy_init();
    if (!c) { result.error = "CURL init failed"; return result; }

    std::string body = payload.dump();
    struct curl_slist* h = nullptr;
    h = curl_slist_append(h, ("xi-api-key: " + api_key_).c_str());
    h = curl_slist_append(h, "Content-Type: application/json");
    h = curl_slist_append(h, "Accept: audio/mpeg");

    std::ofstream ofs(output_path, std::ios::binary);
    curl_easy_setopt(c, CURLOPT_URL, url.c_str());
    curl_easy_setopt(c, CURLOPT_HTTPHEADER, h);
    curl_easy_setopt(c, CURLOPT_POST, 1L);
    curl_easy_setopt(c, CURLOPT_POSTFIELDS, body.c_str());
    curl_easy_setopt(c, CURLOPT_WRITEFUNCTION, vv_file_cb);
    curl_easy_setopt(c, CURLOPT_WRITEDATA, &ofs);
    curl_easy_setopt(c, CURLOPT_TIMEOUT, 60L);

    CURLcode res = curl_easy_perform(c);
    curl_slist_free_all(h);
    curl_easy_cleanup(c);

    if (res != CURLE_OK) {
        result.error = curl_easy_strerror(res); return result;
    }
    if (!fs::exists(output_path) || fs::file_size(output_path) < 100) {
        result.error = "Audio output too small or missing"; return result;
    }
    result.success    = true;
    result.audio_path = output_path;
    result.duration_seconds = estimate_duration(text);
    return result;
}

std::vector<VoiceOption> ElevenLabsTTSProvider::get_available_voices() {
    CURL* c = curl_easy_init();
    if (!c) return {};
    std::string resp;
    struct curl_slist* h = nullptr;
    h = curl_slist_append(h, ("xi-api-key: " + api_key_).c_str());
    h = curl_slist_append(h, "Accept: application/json");
    curl_easy_setopt(c, CURLOPT_URL, "https://api.elevenlabs.io/v1/voices");
    curl_easy_setopt(c, CURLOPT_HTTPHEADER, h);
    curl_easy_setopt(c, CURLOPT_WRITEFUNCTION, vv_write_cb);
    curl_easy_setopt(c, CURLOPT_WRITEDATA, &resp);
    curl_easy_perform(c);
    curl_slist_free_all(h);
    curl_easy_cleanup(c);

    std::vector<VoiceOption> voices;
    try {
        for (auto& v : json::parse(resp)["voices"]) {
            VoiceOption vo;
            vo.id   = v.value("voice_id","");
            vo.name = v.value("name","");
            vo.language = "en";
            vo.gender = v.contains("labels") ? v["labels"].value("gender","") : "";
            voices.push_back(vo);
        }
    } catch (...) {}
    return voices;
}

double ElevenLabsTTSProvider::get_character_balance() {
    CURL* c = curl_easy_init();
    if (!c) return 0.0;
    std::string resp;
    struct curl_slist* h = nullptr;
    h = curl_slist_append(h, ("xi-api-key: " + api_key_).c_str());
    curl_easy_setopt(c, CURLOPT_URL, "https://api.elevenlabs.io/v1/user/subscription");
    curl_easy_setopt(c, CURLOPT_HTTPHEADER, h);
    curl_easy_setopt(c, CURLOPT_WRITEFUNCTION, vv_write_cb);
    curl_easy_setopt(c, CURLOPT_WRITEDATA, &resp);
    curl_easy_perform(c);
    curl_slist_free_all(h);
    curl_easy_cleanup(c);
    try {
        auto j = json::parse(resp);
        return static_cast<double>(
            j.value("character_limit",0) - j.value("character_count",0));
    } catch (...) { return 0.0; }
}

double ElevenLabsTTSProvider::estimate_duration(const std::string& text) const {
    // ~150 words per minute average TTS
    double words = static_cast<double>(
        std::count(text.begin(), text.end(), ' ') + 1);
    return (words / 150.0) * 60.0;
}

// ─────────────────────────────────────────────────────────
// VoiceVerificationManager
// ─────────────────────────────────────────────────────────

VoiceVerificationManager::VoiceVerificationManager()
    : tts_provider_(nullptr) {
    curl_global_init(CURL_GLOBAL_DEFAULT);
}

VoiceVerificationManager::~VoiceVerificationManager() {
    curl_global_cleanup();
}

void VoiceVerificationManager::set_elevenlabs_provider(const std::string& api_key) {
    tts_provider_ = std::make_unique<ElevenLabsTTSProvider>(api_key);
}

TTSResult VoiceVerificationManager::generate_verification_audio(
    const std::string& code, const std::string& output_path,
    const std::string& voice_id) {
    if (!tts_provider_) {
        TTSResult r; r.error = "No TTS provider configured"; return r;
    }
    // Format code for TTS: "1 2 3 4 5 6" 
    std::string spoken;
    for (size_t i = 0; i < code.size(); ++i) {
        spoken += code[i];
        if (i + 1 < code.size()) spoken += ". ";
    }
    return tts_provider_->synthesize(
        "Your verification code is: " + spoken + ". I repeat: " + spoken,
        voice_id, output_path);
}

bool VoiceVerificationManager::play_audio(const std::string& audio_path) {
    // Platform-specific audio playback
#ifdef _WIN32
    std::string cmd = "start /wait \"\" \"" + audio_path + "\"";
#elif __APPLE__
    std::string cmd = "afplay \"" + audio_path + "\"";
#else
    std::string cmd = "aplay -q \"" + audio_path + "\" 2>/dev/null || "
                      "mpg123 -q \"" + audio_path + "\" 2>/dev/null || true";
#endif
    return std::system(cmd.c_str()) == 0;
}

} // namespace gmail_infinity
