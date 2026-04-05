#include "fingerprint_generator.h"
#include "app/app_logger.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <chrono>
#include <iomanip>
#include <openssl/sha.h>

namespace gmail_infinity {

// BrowserFingerprint implementation
nlohmann::json BrowserFingerprint::to_json() const {
    nlohmann::json j;
    j["user_agent"] = user_agent;
    j["screen_resolution"] = screen_resolution;
    j["color_depth"] = color_depth;
    j["timezone"] = timezone;
    j["language"] = language;
    j["platform"] = platform;
    j["hardware_concurrency"] = hardware_concurrency;
    j["device_memory"] = device_memory;
    j["webgl_vendor"] = webgl_vendor;
    j["webgl_renderer"] = webgl_renderer;
    j["canvas_fingerprint"] = canvas_fingerprint;
    j["audio_fingerprint"] = audio_fingerprint;
    j["fonts"] = fonts;
    j["plugins"] = plugins;
    j["cookie_enabled"] = cookie_enabled;
    j["do_not_track"] = do_not_track;
    j["connection_type"] = connection_type;
    j["effective_type"] = effective_type;
    j["downlink"] = downlink;
    j["rtt"] = rtt;
    j["save_data"] = save_data;
    j["webgl_parameters"] = webgl_parameters;
    j["canvas_noise"] = canvas_noise;
    j["audio_context_noise"] = audio_context_noise;
    j["screen_pixel_ratio"] = screen_pixel_ratio;
    j["touch_support"] = touch_support;
    j["gamepad_support"] = gamepad_support;
    j["vr_support"] = vr_support;
    j["battery_charging"] = battery_charging;
    j["battery_level"] = battery_level;
    j["cpu_class"] = cpu_class;
    j["platform_version"] = platform_version;
    return j;
}

void BrowserFingerprint::from_json(const nlohmann::json& j) {
    if (j.contains("user_agent")) user_agent = j["user_agent"].get<std::string>();
    if (j.contains("screen_resolution")) screen_resolution = j["screen_resolution"].get<std::string>();
    if (j.contains("color_depth")) color_depth = j["color_depth"].get<std::string>();
    if (j.contains("timezone")) timezone = j["timezone"].get<std::string>();
    if (j.contains("language")) language = j["language"].get<std::string>();
    if (j.contains("platform")) platform = j["platform"].get<std::string>();
    if (j.contains("hardware_concurrency")) hardware_concurrency = j["hardware_concurrency"].get<std::string>();
    if (j.contains("device_memory")) device_memory = j["device_memory"].get<std::string>();
    if (j.contains("webgl_vendor")) webgl_vendor = j["webgl_vendor"].get<std::string>();
    if (j.contains("webgl_renderer")) webgl_renderer = j["webgl_renderer"].get<std::string>();
    if (j.contains("canvas_fingerprint")) canvas_fingerprint = j["canvas_fingerprint"].get<std::string>();
    if (j.contains("audio_fingerprint")) audio_fingerprint = j["audio_fingerprint"].get<std::string>();
    if (j.contains("fonts")) fonts = j["fonts"].get<std::string>();
    if (j.contains("plugins")) plugins = j["plugins"].get<std::string>();
    if (j.contains("cookie_enabled")) cookie_enabled = j["cookie_enabled"].get<std::string>();
    if (j.contains("do_not_track")) do_not_track = j["do_not_track"].get<std::string>();
    if (j.contains("connection_type")) connection_type = j["connection_type"].get<std::string>();
    if (j.contains("effective_type")) effective_type = j["effective_type"].get<std::string>();
    if (j.contains("downlink")) downlink = j["downlink"].get<std::string>();
    if (j.contains("rtt")) rtt = j["rtt"].get<std::string>();
    if (j.contains("save_data")) save_data = j["save_data"].get<std::string>();
    if (j.contains("webgl_parameters")) webgl_parameters = j["webgl_parameters"].get<std::string>();
    if (j.contains("canvas_noise")) canvas_noise = j["canvas_noise"].get<std::string>();
    if (j.contains("audio_context_noise")) audio_context_noise = j["audio_context_noise"].get<std::string>();
    if (j.contains("screen_pixel_ratio")) screen_pixel_ratio = j["screen_pixel_ratio"].get<std::string>();
    if (j.contains("touch_support")) touch_support = j["touch_support"].get<std::string>();
    if (j.contains("gamepad_support")) gamepad_support = j["gamepad_support"].get<std::string>();
    if (j.contains("vr_support")) vr_support = j["vr_support"].get<std::string>();
    if (j.contains("battery_charging")) battery_charging = j["battery_charging"].get<std::string>();
    if (j.contains("battery_level")) battery_level = j["battery_level"].get<std::string>();
    if (j.contains("cpu_class")) cpu_class = j["cpu_class"].get<std::string>();
    if (j.contains("platform_version")) platform_version = j["platform_version"].get<std::string>();
}

std::string BrowserFingerprint::generate_hash() const {
    std::string combined = user_agent + screen_resolution + timezone + language + 
                          platform + webgl_vendor + webgl_renderer + canvas_fingerprint;
    
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(combined.c_str()), combined.length(), hash);
    
    std::stringstream ss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
    }
    
    return ss.str();
}

// FingerprintCache implementation
FingerprintCache::FingerprintCache() : rng_(std::random_device{}()) {}

bool FingerprintCache::load_fingerprints(const std::string& filename) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    try {
        std::ifstream file(filename);
        if (!file.is_open()) {
            log_fingerprint_info("Fingerprint file not found, starting with empty cache: " + filename);
            return false;
        }
        
        nlohmann::json json_data;
        file >> json_data;
        
        fingerprints_.clear();
        rotation_order_.clear();
        
        if (json_data.contains("fingerprints")) {
            for (const auto& fp_json : json_data["fingerprints"]) {
                auto fingerprint = std::make_shared<BrowserFingerprint>();
                fingerprint->from_json(fp_json);
                
                std::string hash = fingerprint->generate_hash();
                fingerprints_[hash] = fingerprint;
                rotation_order_.push_back(hash);
                
                // Initialize counters
                success_count_[hash] = 0;
                failure_count_[hash] = 0;
                usage_count_[hash] = 0;
                last_used_[hash] = std::chrono::system_clock::now();
            }
        }
        
        log_fingerprint_info("Loaded " + std::to_string(fingerprints_.size()) + " fingerprints from " + filename);
        return true;
        
    } catch (const std::exception& e) {
        log_fingerprint_error("Failed to load fingerprints: " + std::string(e.what()));
        return false;
    }
}

bool FingerprintCache::save_fingerprints(const std::string& filename) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    try {
        nlohmann::json json_data;
        json_data["fingerprints"] = nlohmann::json::array();
        
        for (const auto& [hash, fingerprint] : fingerprints_) {
            json_data["fingerprints"].push_back(fingerprint->to_json());
        }
        
        json_data["version"] = "1.0";
        json_data["timestamp"] = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        
        std::ofstream file(filename);
        if (!file.is_open()) {
            log_fingerprint_error("Failed to open fingerprint file for writing: " + filename);
            return false;
        }
        
        file << json_data.dump(4);
        file.close();
        
        log_fingerprint_info("Saved " + std::to_string(fingerprints_.size()) + " fingerprints to " + filename);
        return true;
        
    } catch (const std::exception& e) {
        log_fingerprint_error("Failed to save fingerprints: " + std::string(e.what()));
        return false;
    }
}

bool FingerprintCache::add_fingerprint(const BrowserFingerprint& fingerprint) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    std::string hash = fingerprint.generate_hash();
    
    if (fingerprints_.find(hash) != fingerprints_.end()) {
        log_fingerprint_info("Fingerprint already exists: " + hash);
        return false;
    }
    
    auto fp = std::make_shared<BrowserFingerprint>(fingerprint);
    fingerprints_[hash] = fp;
    rotation_order_.push_back(hash);
    
    // Initialize counters
    success_count_[hash] = 0;
    failure_count_[hash] = 0;
    usage_count_[hash] = 0;
    last_used_[hash] = std::chrono::system_clock::now();
    
    log_fingerprint_info("Added new fingerprint: " + hash);
    return true;
}

std::shared_ptr<BrowserFingerprint> FingerprintCache::get_next_fingerprint() {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    if (fingerprints_.empty()) {
        log_fingerprint_warn("No fingerprints available in cache");
        return nullptr;
    }
    
    std::string hash = get_next_fingerprint_hash();
    
    auto it = fingerprints_.find(hash);
    if (it != fingerprints_.end()) {
        update_fingerprint_usage(hash);
        return it->second;
    }
    
    return nullptr;
}

std::shared_ptr<BrowserFingerprint> FingerprintCache::get_fingerprint_for_country(const std::string& country) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    std::vector<std::string> candidates;
    for (const auto& [hash, fingerprint] : fingerprints_) {
        if (fingerprint->language.find(country) != std::string::npos ||
            fingerprint->timezone.find(country) != std::string::npos) {
            candidates.push_back(hash);
        }
    }
    
    if (candidates.empty()) {
        return get_next_fingerprint(); // Fallback to any fingerprint
    }
    
    std::uniform_int_distribution<size_t> dist(0, candidates.size() - 1);
    std::string selected_hash = candidates[dist(rng_)];
    
    auto it = fingerprints_.find(selected_hash);
    if (it != fingerprints_.end()) {
        update_fingerprint_usage(selected_hash);
        return it->second;
    }
    
    return nullptr;
}

void FingerprintCache::update_fingerprint_usage(const std::string& hash) {
    usage_count_[hash]++;
    last_used_[hash] = std::chrono::system_clock::now();
}

void FingerprintCache::mark_fingerprint_successful(const std::string& hash) {
    success_count_[hash]++;
}

void FingerprintCache::mark_fingerprint_failed(const std::string& hash) {
    failure_count_[hash]++;
}

size_t FingerprintCache::get_total_fingerprints() const {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    return fingerprints_.size();
}

std::string FingerprintCache::get_next_fingerprint_hash() {
    switch (rotation_strategy_) {
        case FingerprintRotationStrategy::ROUND_ROBIN:
            if (rotation_order_.empty()) return "";
            return rotation_order_[current_index_++ % rotation_order_.size()];
            
        case FingerprintRotationStrategy::RANDOM:
            if (rotation_order_.empty()) return "";
            std::uniform_int_distribution<size_t> dist(0, rotation_order_.size() - 1);
            return rotation_order_[dist(rng_)];
            
        case FingerprintRotationStrategy::WEIGHTED_RANDOM:
            // Implement weighted random based on success rates
            return get_weighted_random_fingerprint()->generate_hash();
            
        default:
            return rotation_order_.empty() ? "" : rotation_order_[0];
    }
}

std::shared_ptr<BrowserFingerprint> FingerprintCache::get_weighted_random_fingerprint() {
    if (fingerprints_.empty()) return nullptr;
    
    // Calculate weights based on success rates
    std::vector<std::pair<std::string, double>> weighted_hashes;
    
    for (const auto& [hash, fingerprint] : fingerprints_) {
        int success = success_count_[hash].load();
        int failure = failure_count_[hash].load();
        int total = success + failure;
        
        double weight = total > 0 ? static_cast<double>(success) / total : 0.5;
        weighted_hashes.emplace_back(hash, weight);
    }
    
    // Weighted random selection
    std::discrete_distribution<size_t> dist(
        weighted_hashes.size(),
        0, 1,
        [&](size_t i) { return weighted_hashes[i].second; }
    );
    
    size_t selected = dist(rng_);
    std::string selected_hash = weighted_hashes[selected].first;
    
    auto it = fingerprints_.find(selected_hash);
    return it != fingerprints_.end() ? it->second : nullptr;
}

std::shared_ptr<BrowserFingerprint> FingerprintCache::generate_random_fingerprint() {
    auto fingerprint = std::make_shared<BrowserFingerprint>();
    
    // Generate realistic random values
    std::vector<std::string> user_agents = {
        "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36",
        "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/119.0.0.0 Safari/537.36",
        "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36",
        "Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:109.0) Gecko/20100101 Firefox/121.0",
        "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/605.1.15 (KHTML, like Gecko) Version/17.2 Safari/605.1.15"
    };
    
    std::vector<std::string> resolutions = {"1920x1080", "1366x768", "1440x900", "1536x864", "1280x720"};
    std::vector<std::string> timezones = {"America/New_York", "Europe/London", "Asia/Tokyo", "America/Los_Angeles"};
    std::vector<std::string> languages = {"en-US,en;q=0.9", "en-GB,en;q=0.9", "en;q=0.9"};
    
    std::uniform_int_distribution<size_t> ua_dist(0, user_agents.size() - 1);
    std::uniform_int_distribution<size_t> res_dist(0, resolutions.size() - 1);
    std::uniform_int_distribution<size_t> tz_dist(0, timezones.size() - 1);
    std::uniform_int_distribution<size_t> lang_dist(0, languages.size() - 1);
    
    fingerprint->user_agent = user_agents[ua_dist(rng_)];
    fingerprint->screen_resolution = resolutions[res_dist(rng_)];
    fingerprint->color_depth = "24";
    fingerprint->timezone = timezones[tz_dist(rng_)];
    fingerprint->language = languages[lang_dist(rng_)];
    fingerprint->platform = "Win32";
    fingerprint->hardware_concurrency = "4";
    fingerprint->device_memory = "8";
    fingerprint->cookie_enabled = "true";
    fingerprint->do_not_track = "unspecified";
    
    // Generate WebGL parameters
    fingerprint->webgl_vendor = "Google Inc.";
    fingerprint->webgl_renderer = "ANGLE (Intel, Intel(R) UHD Graphics 620 Direct3D11 vs_5_0 ps_5_0, D3D11)";
    fingerprint->webgl_parameters = generate_webgl_parameters();
    
    // Generate canvas and audio fingerprints with noise
    fingerprint->canvas_fingerprint = generate_canvas_noise();
    fingerprint->audio_fingerprint = generate_audio_noise();
    
    // Generate fonts and plugins
    std::vector<std::string> common_fonts = get_common_fonts();
    fingerprint->fonts = "";
    for (const auto& font : common_fonts) {
        if (!fingerprint->fonts.empty()) fingerprint->fonts += ",";
        fingerprint->fonts += font;
    }
    
    std::vector<std::string> common_plugins = get_common_plugins();
    fingerprint->plugins = "";
    for (const auto& plugin : common_plugins) {
        if (!fingerprint->plugins.empty()) fingerprint->plugins += ",";
        fingerprint->plugins += plugin;
    }
    
    // Network information
    fingerprint->connection_type = "4g";
    fingerprint->effective_type = "4g";
    fingerprint->downlink = "10";
    fingerprint->rtt = "50";
    fingerprint->save_data = "false";
    
    // Advanced fingerprinting
    fingerprint->screen_pixel_ratio = "1";
    fingerprint->touch_support = "false";
    fingerprint->gamepad_support = "true";
    fingerprint->vr_support = "false";
    fingerprint->battery_charging = "true";
    fingerprint->battery_level = "0.8";
    fingerprint->cpu_class = "unknown";
    fingerprint->platform_version = "10.0.0";
    
    return fingerprint;
}

std::string FingerprintCache::generate_canvas_noise() const {
    std::uniform_int_distribution<int> dist(0, 255);
    std::stringstream ss;
    
    for (int i = 0; i < 64; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << dist(rng_);
    }
    
    return ss.str();
}

std::string FingerprintCache::generate_audio_noise() const {
    std::uniform_int_distribution<int> dist(0, 255);
    std::stringstream ss;
    
    for (int i = 0; i < 32; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << dist(rng_);
    }
    
    return ss.str();
}

std::string FingerprintCache::generate_webgl_parameters() const {
    std::uniform_int_distribution<int> dist(1, 100);
    
    return "version:" + std::to_string(dist(rng_)) + 
           ",vendor:" + std::to_string(dist(rng_)) +
           ",renderer:" + std::to_string(dist(rng_));
}

std::vector<std::string> FingerprintCache::get_common_fonts() const {
    return {
        "Arial", "Arial Black", "Comic Sans MS", "Courier New", "Georgia",
        "Helvetica", "Impact", "Times New Roman", "Trebuchet MS", "Verdana",
        "Calibri", "Cambria", "Consolas", "Palatino", "Tahoma"
    };
}

std::vector<std::string> FingerprintCache::get_common_plugins() const {
    return {
        "Chrome PDF Plugin", "Chrome PDF Viewer", "Native Client",
        "Built-in PDF viewer", "WebKit built-in PDF"
    };
}

void FingerprintCache::log_fingerprint_info(const std::string& message) const {
    LOG_INFO("FingerprintCache: {}", message);
}

void FingerprintCache::log_fingerprint_error(const std::string& message) const {
    LOG_ERROR("FingerprintCache: {}", message);
}

void FingerprintCache::log_fingerprint_warn(const std::string& message) const {
    LOG_WARN("FingerprintCache: {}", message);
}

// QuantumFingerprint implementation
QuantumFingerprint::QuantumFingerprint() : noise_dist_(0.0, 1.0), rng_(std::random_device{}()) {}

std::shared_ptr<BrowserFingerprint> QuantumFingerprint::generate_quantum_fingerprint(
    const std::map<std::string, std::string>& constraints) {
    
    auto fingerprint = std::make_shared<BrowserFingerprint>();
    
    // Generate base fingerprint
    FingerprintCache cache;
    auto base_fp = cache.generate_random_fingerprint();
    if (base_fp) {
        *fingerprint = *base_fp;
    }
    
    // Apply constraints
    for (const auto& [key, value] : constraints) {
        if (key == "user_agent") fingerprint->user_agent = value;
        else if (key == "timezone") fingerprint->timezone = value;
        else if (key == "language") fingerprint->language = value;
        else if (key == "country") fingerprint->language = value + "-" + value;
    }
    
    // Inject quantum noise
    inject_canvas_noise(*fingerprint, noise_level_);
    inject_webgl_noise(*fingerprint, noise_level_);
    inject_audio_noise(*fingerprint, noise_level_);
    
    // Add behavioral patterns
    add_behavioral_patterns(*fingerprint);
    
    // Ensure consistency
    ensure_fingerprint_consistency(*fingerprint);
    
    return fingerprint;
}

void QuantumFingerprint::inject_canvas_noise(BrowserFingerprint& fingerprint, double intensity) {
    if (noise_dist_(rng_) < intensity) {
        fingerprint.canvas_fingerprint = generate_quantum_canvas_noise();
    }
}

void QuantumFingerprint::inject_webgl_noise(BrowserFingerprint& fingerprint, double intensity) {
    if (noise_dist_(rng_) < intensity) {
        fingerprint.webgl_parameters = generate_quantum_webgl_noise();
    }
}

void QuantumFingerprint::inject_audio_noise(BrowserFingerprint& fingerprint, double intensity) {
    if (noise_dist_(rng_) < intensity) {
        fingerprint.audio_fingerprint = generate_quantum_audio_noise();
    }
}

std::string QuantumFingerprint::generate_quantum_canvas_noise() const {
    std::uniform_int_distribution<int> dist(0, 255);
    std::stringstream ss;
    
    // Generate quantum-inspired noise pattern
    for (int i = 0; i < 128; i++) {
        int value = dist(rng_);
        // Add quantum fluctuations
        if (noise_dist_(rng_) < 0.1) {
            value ^= static_cast<int>(noise_dist_(rng_) * 255);
        }
        ss << std::hex << std::setw(2) << std::setfill('0') << value;
    }
    
    return ss.str();
}

std::string QuantumFingerprint::generate_quantum_webgl_noise() const {
    std::uniform_int_distribution<int> dist(1, 1000);
    
    return "quantum_version:" + std::to_string(dist(rng_)) + 
           ",quantum_vendor:" + std::to_string(dist(rng_)) +
           ",quantum_renderer:" + std::to_string(dist(rng_)) +
           ",quantum_noise:" + std::to_string(dist(rng_));
}

std::string QuantumFingerprint::generate_quantum_audio_noise() const {
    std::uniform_int_distribution<int> dist(0, 255);
    std::stringstream ss;
    
    // Generate quantum audio signature
    for (int i = 0; i < 64; i++) {
        int value = dist(rng_);
        // Add quantum interference patterns
        if (i % 8 == 0 && noise_dist_(rng_) < 0.2) {
            value = (value + static_cast<int>(noise_dist_(rng_) * 100)) % 256;
        }
        ss << std::hex << std::setw(2) << std::setfill('0') << value;
    }
    
    return ss.str();
}

void QuantumFingerprint::add_behavioral_patterns(BrowserFingerprint& fingerprint) {
    // Add simulated behavioral markers
    fingerprint.touch_support = (noise_dist_(rng_) < 0.3) ? "true" : "false";
    fingerprint.gamepad_support = (noise_dist_(rng_) < 0.1) ? "true" : "false";
    fingerprint.vr_support = (noise_dist_(rng_) < 0.05) ? "true" : "false";
    
    // Battery simulation
    fingerprint.battery_charging = (noise_dist_(rng_) < 0.7) ? "true" : "false";
    std::uniform_real_distribution<double> battery_dist(0.1, 1.0);
    fingerprint.battery_level = std::to_string(battery_dist(rng_));
}

void QuantumFingerprint::ensure_fingerprint_consistency(BrowserFingerprint& fingerprint) {
    // Ensure user agent matches platform
    if (fingerprint.user_agent.find("Windows") != std::string::npos) {
        fingerprint.platform = "Win32";
    } else if (fingerprint.user_agent.find("Macintosh") != std::string::npos) {
        fingerprint.platform = "MacIntel";
    } else if (fingerprint.user_agent.find("Linux") != std::string::npos) {
        fingerprint.platform = "Linux x86_64";
    }
    
    // Ensure screen resolution is realistic
    if (fingerprint.screen_resolution.empty()) {
        fingerprint.screen_resolution = "1920x1080";
    }
    
    // Ensure color depth matches resolution
    fingerprint.color_depth = "24";
}

} // namespace gmail_infinity
