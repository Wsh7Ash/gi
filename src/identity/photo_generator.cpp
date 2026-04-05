#include "photo_generator.h"
#include "app/app_logger.h"

#include <spdlog/spdlog.h>
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <fstream>
#include <sstream>
#include <random>
#include <filesystem>

namespace gmail_infinity {

using json = nlohmann::json;
namespace fs = std::filesystem;

// CURL write callback
static size_t write_callback(void* ptr, size_t size, size_t nmemb, std::string* data) {
    data->append(static_cast<char*>(ptr), size * nmemb);
    return size * nmemb;
}

static size_t write_file_callback(void* ptr, size_t size, size_t nmemb, std::ofstream* file) {
    file->write(static_cast<char*>(ptr), static_cast<std::streamsize>(size * nmemb));
    return size * nmemb;
}

PhotoGenerator::PhotoGenerator() {
    std::random_device rd;
    rng_.seed(rd());
    curl_global_init(CURL_GLOBAL_DEFAULT);
}

PhotoGenerator::~PhotoGenerator() {
    curl_global_cleanup();
}

void PhotoGenerator::set_api_key(const std::string& provider, const std::string& key) {
    api_keys_[provider] = key;
}

std::string PhotoGenerator::generate_profile_photo(
    const PhotoRequest& req, const std::string& output_path) const {
    // Try providers in order
    for (auto& provider : {"this_person_does_not_exist", "stable_diffusion", "dalle"}) {
        try {
            std::string result;
            if (std::string(provider) == "this_person_does_not_exist") {
                result = fetch_thispersondoesnotexist(output_path);
            } else if (std::string(provider) == "stable_diffusion") {
                auto key_it = api_keys_.find("stability");
                if (key_it == api_keys_.end()) continue;
                result = generate_with_stability(req, key_it->second, output_path);
            } else {
                auto key_it = api_keys_.find("openai");
                if (key_it == api_keys_.end()) continue;
                result = generate_with_dalle(req, key_it->second, output_path);
            }
            if (!result.empty() && fs::exists(result)) {
                spdlog::info("PhotoGenerator: photo saved to {}", result);
                return result;
            }
        } catch (const std::exception& e) {
            spdlog::warn("PhotoGenerator provider {} failed: {}", provider, e.what());
        }
    }
    return generate_placeholder(output_path);
}

std::string PhotoGenerator::fetch_thispersondoesnotexist(const std::string& output_path) const {
    CURL* curl = curl_easy_init();
    if (!curl) return "";

    std::ofstream ofs(output_path, std::ios::binary);
    if (!ofs) { curl_easy_cleanup(curl); return ""; }

    // Randomise to get different faces
    std::uniform_int_distribution<int> cache_buster(1, 99999);
    std::string url = "https://thispersondoesnotexist.com/?r=" + std::to_string(cache_buster(rng_));

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_file_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &ofs);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT,
        "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 Chrome/122.0.0.0 Safari/537.36");
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);

    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        spdlog::warn("PhotoGenerator: thispersondoesnotexist.com request failed: {}", curl_easy_strerror(res));
        return "";
    }
    return output_path;
}

std::string PhotoGenerator::generate_with_stability(
    const PhotoRequest& req, const std::string& api_key,
    const std::string& output_path) const {
    CURL* curl = curl_easy_init();
    if (!curl) return "";

    std::string prompt = build_prompt(req);
    json payload = {
        {"text_prompts", {{{"text", prompt}, {"weight", 1.0}}}},
        {"cfg_scale",    7},
        {"height",       512},
        {"width",        512},
        {"samples",      1},
        {"steps",        30}
    };
    std::string body = payload.dump();
    std::string response;

    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, ("Authorization: Bearer " + api_key).c_str());
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, "Accept: application/json");

    curl_easy_setopt(curl, CURLOPT_URL,
        "https://api.stability.ai/v1/generation/stable-diffusion-xl-1024-v1-0/text-to-image");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 60L);

    CURLcode res = curl_easy_perform(curl);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) return "";

    try {
        auto j = json::parse(response);
        auto artifacts = j["artifacts"];
        if (artifacts.empty()) return "";
        auto b64 = artifacts[0]["base64"].get<std::string>();
        return decode_and_save_base64(b64, output_path);
    } catch (...) { return ""; }
}

std::string PhotoGenerator::generate_with_dalle(
    const PhotoRequest& req, const std::string& api_key,
    const std::string& output_path) const {
    CURL* curl = curl_easy_init();
    if (!curl) return "";

    std::string prompt = build_prompt(req);
    json payload = {{"model", "dall-e-3"}, {"prompt", prompt}, {"n", 1}, {"size", "1024x1024"},
                    {"response_format", "b64_json"}};
    std::string body = payload.dump();
    std::string response;

    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, ("Authorization: Bearer " + api_key).c_str());
    headers = curl_slist_append(headers, "Content-Type: application/json");

    curl_easy_setopt(curl, CURLOPT_URL, "https://api.openai.com/v1/images/generations");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 60L);

    CURLcode res = curl_easy_perform(curl);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) return "";

    try {
        auto j = json::parse(response);
        auto b64 = j["data"][0]["b64_json"].get<std::string>();
        return decode_and_save_base64(b64, output_path);
    } catch (...) { return ""; }
}

std::string PhotoGenerator::generate_placeholder(const std::string& output_path) const {
    // Create a minimal placeholder: we write a 1x1 white JPEG
    // JPEG SOI + minimal header + EOI
    static const unsigned char placeholder_jpg[] = {
        0xFF,0xD8,0xFF,0xE0,0x00,0x10,0x4A,0x46,0x49,0x46,0x00,0x01,
        0x01,0x00,0x00,0x01,0x00,0x01,0x00,0x00,0xFF,0xDB,0x00,0x43,
        0x00,0x08,0x06,0x06,0x07,0x06,0x05,0x08,0x07,0x07,0x07,0x09,
        0x09,0x08,0x0A,0x0C,0x14,0x0D,0x0C,0x0B,0x0B,0x0C,0x19,0x12,
        0x13,0x0F,0x14,0x1D,0x1A,0x1F,0x1E,0x1D,0x1A,0x1C,0x1C,0x20,
        0x24,0x2E,0x27,0x20,0x22,0x2C,0x23,0x1C,0x1C,0x28,0x37,0x29,
        0x2C,0x30,0x31,0x34,0x34,0x34,0x1F,0x27,0x39,0x3D,0x38,0x32,
        0x3C,0x2E,0x33,0x34,0x32,0xFF,0xC0,0x00,0x0B,0x08,0x00,0x01,
        0x00,0x01,0x01,0x01,0x11,0x00,0xFF,0xC4,0x00,0x1F,0x00,0x00,
        0x01,0x05,0x01,0x01,0x01,0x01,0x01,0x01,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,
        0x09,0x0A,0x0B,0xFF,0xDA,0x00,0x08,0x01,0x01,0x00,0x00,0x3F,
        0x00,0xFB,0x00,0xFF,0xD9
    };
    std::ofstream ofs(output_path, std::ios::binary);
    if (!ofs) return "";
    ofs.write(reinterpret_cast<const char*>(placeholder_jpg), sizeof(placeholder_jpg));
    return output_path;
}

std::string PhotoGenerator::build_prompt(const PhotoRequest& req) const {
    std::string prompt = "Professional headshot portrait photo, ";
    if (!req.gender.empty()) prompt += req.gender + ", ";
    if (req.age_range_min > 0)
        prompt += std::to_string(req.age_range_min) + "-" + std::to_string(req.age_range_max)
                  + " years old, ";
    if (!req.ethnicity.empty()) prompt += req.ethnicity + " ethnicity, ";
    if (!req.style.empty()) prompt += req.style + " style, ";
    prompt += "natural lighting, 4K, photorealistic, LinkedIn profile photo";
    return prompt;
}

std::string PhotoGenerator::decode_and_save_base64(
    const std::string& b64, const std::string& output_path) const {
    static const std::string chars =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string out;
    int val = 0, bits = -8;
    for (unsigned char c : b64) {
        if (c == '=') break;
        auto idx = chars.find(c);
        if (idx == std::string::npos) continue;
        val = (val << 6) + static_cast<int>(idx);
        bits += 6;
        if (bits >= 0) { out += static_cast<char>((val >> bits) & 0xFF); bits -= 8; }
    }
    std::ofstream ofs(output_path, std::ios::binary);
    if (!ofs) return "";
    ofs << out;
    return output_path;
}

} // namespace gmail_infinity
