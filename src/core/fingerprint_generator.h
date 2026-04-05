#pragma once

#include <string>
#include <vector>
#include <memory>
#include <random>
#include <map>
#include <mutex>

#include <nlohmann/json.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>

namespace gmail_infinity {

struct BrowserFingerprint {
    std::string user_agent;
    std::string screen_resolution;
    std::string color_depth;
    std::string timezone;
    std::string language;
    std::string platform;
    std::string hardware_concurrency;
    std::string device_memory;
    std::string webgl_vendor;
    std::string webgl_renderer;
    std::string canvas_fingerprint;
    std::string audio_fingerprint;
    std::string fonts;
    std::string plugins;
    std::string cookie_enabled;
    std::string do_not_track;
    std::string connection_type;
    std::string effective_type;
    std::string downlink;
    std::string rtt;
    std::string save_data;
    
    // Advanced fingerprinting
    std::string webgl_parameters;
    std::string canvas_noise;
    std::string audio_context_noise;
    std::string screen_pixel_ratio;
    std::string touch_support;
    std::string gamepad_support;
    std::string vr_support;
    std::string battery_charging;
    std::string battery_level;
    std::string cpu_class;
    std::string platform_version;
    
    nlohmann::json to_json() const;
    void from_json(const nlohmann::json& j);
    std::string generate_hash() const;
};

enum class FingerprintRotationStrategy {
    ROUND_ROBIN,
    RANDOM,
    WEIGHTED_RANDOM,
    GEOGRAPHIC,
    TIME_BASED,
    ADAPTIVE
};

class FingerprintCache {
public:
    FingerprintCache();
    ~FingerprintCache() = default;
    
    // Initialization
    bool load_fingerprints(const std::string& filename = "config/fingerprints.json");
    bool save_fingerprints(const std::string& filename = "config/fingerprints.json");
    
    // Fingerprint management
    bool add_fingerprint(const BrowserFingerprint& fingerprint);
    bool remove_fingerprint(const std::string& hash);
    void clear_cache();
    
    // Fingerprint retrieval
    std::shared_ptr<BrowserFingerprint> get_fingerprint(const std::string& hash);
    std::shared_ptr<BrowserFingerprint> get_next_fingerprint();
    std::shared_ptr<BrowserFingerprint> get_fingerprint_by_criteria(
        const std::map<std::string, std::string>& criteria);
    
    // Rotation strategies
    void set_rotation_strategy(FingerprintRotationStrategy strategy);
    FingerprintRotationStrategy get_rotation_strategy() const;
    
    // Geographic targeting
    std::shared_ptr<BrowserFingerprint> get_fingerprint_for_country(const std::string& country);
    std::shared_ptr<BrowserFingerprint> get_fingerprint_for_timezone(const std::string& timezone);
    
    // Statistics
    size_t get_total_fingerprints() const;
    size_t get_fingerprints_by_country(const std::string& country) const;
    std::map<std::string, size_t> get_country_distribution() const;
    
    // Cache management
    void cleanup_unused_fingerprints(std::chrono::hours age = std::chrono::hours(24 * 7));
    void update_fingerprint_usage(const std::string& hash);
    void mark_fingerprint_successful(const std::string& hash);
    void mark_fingerprint_failed(const std::string& hash);
    
    // Advanced features
    std::shared_ptr<BrowserFingerprint> generate_adaptive_fingerprint(
        const std::map<std::string, std::string>& context);
    bool validate_fingerprint(const BrowserFingerprint& fingerprint);
    
    // Export/Import
    bool export_to_csv(const std::string& filename) const;
    bool import_from_csv(const std::string& filename);

private:
    // Storage
    mutable std::mutex cache_mutex_;
    std::map<std::string, std::shared_ptr<BrowserFingerprint>> fingerprints_;
    std::vector<std::string> rotation_order_;
    std::map<std::string, size_t> usage_count_;
    std::map<std::string, std::chrono::system_clock::time_point> last_used_;
    std::map<std::string, std::atomic<int>> success_count_;
    std::map<std::string, std::atomic<int>> failure_count_;
    
    // Configuration
    FingerprintRotationStrategy rotation_strategy_{FingerprintRotationStrategy::ROUND_ROBIN};
    std::atomic<size_t> current_index_{0};
    std::string last_country_used_;
    std::chrono::system_clock::time_point last_rotation_time_;
    
    // Random generation
    mutable std::mt19937 rng_;
    boost::uuids::random_generator uuid_generator_;
    
    // Private methods
    void rebuild_rotation_order();
    std::string get_next_fingerprint_hash();
    std::shared_ptr<BrowserFingerprint> get_weighted_random_fingerprint();
    std::shared_ptr<BrowserFingerprint> get_geographic_fingerprint(const std::string& country);
    std::shared_ptr<BrowserFingerprint> get_time_based_fingerprint();
    
    // Fingerprint generation
    std::shared_ptr<BrowserFingerprint> generate_random_fingerprint();
    std::shared_ptr<BrowserFingerprint> generate_fingerprint_for_device(const std::string& device_type);
    std::shared_ptr<BrowserFingerprint> generate_fingerprint_for_browser(const std::string& browser);
    
    // Utility methods
    std::string generate_canvas_noise() const;
    std::string generate_audio_noise() const;
    std::string generate_webgl_parameters() const;
    std::vector<std::string> get_common_fonts() const;
    std::vector<std::string> get_common_plugins() const;
    
    // Validation
    bool is_valid_user_agent(const std::string& ua) const;
    bool is_valid_screen_resolution(const std::string& resolution) const;
    bool is_valid_timezone(const std::string& timezone) const;
    
    // Logging
    void log_fingerprint_info(const std::string& message) const;
    void log_fingerprint_error(const std::string& message) const;
};

class QuantumFingerprint {
public:
    QuantumFingerprint();
    ~QuantumFingerprint() = default;
    
    // Advanced fingerprinting with quantum noise
    std::shared_ptr<BrowserFingerprint> generate_quantum_fingerprint(
        const std::map<std::string, std::string>& constraints = {});
    
    // Noise injection
    void inject_canvas_noise(BrowserFingerprint& fingerprint, double intensity = 0.1);
    void inject_webgl_noise(BrowserFingerprint& fingerprint, double intensity = 0.1);
    void inject_audio_noise(BrowserFingerprint& fingerprint, double intensity = 0.1);
    void inject_timing_noise(BrowserFingerprint& fingerprint, double intensity = 0.1);
    
    // Behavioral fingerprinting
    void add_behavioral_patterns(BrowserFingerprint& fingerprint);
    void add_mouse_patterns(BrowserFingerprint& fingerprint);
    void add_keyboard_patterns(BrowserFingerprint& fingerprint);
    void add_scroll_patterns(BrowserFingerprint& fingerprint);
    
    // Anti-detection
    void randomize_fingerprint_structure(BrowserFingerprint& fingerprint);
    void add_inconsistencies(BrowserFingerprint& fingerprint);
    void simulate_human_variability(BrowserFingerprint& fingerprint);
    
    // Configuration
    void set_noise_level(double level);
    void set_consistency_threshold(double threshold);
    void set_variation_frequency(double frequency);

private:
    double noise_level_{0.05};
    double consistency_threshold_{0.8};
    double variation_frequency_{0.1};
    
    mutable std::mt19937 rng_;
    std::uniform_real_distribution<double> noise_dist_;
    
    // Noise generation methods
    std::string generate_quantum_canvas_noise() const;
    std::string generate_quantum_webgl_noise() const;
    std::string generate_quantum_audio_noise() const;
    
    // Behavioral simulation
    std::string simulate_mouse_movement_pattern() const;
    std::string simulate_typing_pattern() const;
    std::string simulate_scroll_behavior() const;
    
    // Consistency management
    void ensure_fingerprint_consistency(BrowserFingerprint& fingerprint);
    void add_controlled_inconsistency(BrowserFingerprint& fingerprint);
};

class FingerprintValidator {
public:
    FingerprintValidator();
    ~FingerprintValidator() = default;
    
    // Validation methods
    bool validate_fingerprint(const BrowserFingerprint& fingerprint);
    bool validate_consistency(const BrowserFingerprint& fingerprint);
    bool validate_realism(const BrowserFingerprint& fingerprint);
    
    // Scoring
    double calculate_realism_score(const BrowserFingerprint& fingerprint);
    double calculate_consistency_score(const BrowserFingerprint& fingerprint);
    double calculate_uniqueness_score(const BrowserFingerprint& fingerprint);
    
    // Anomaly detection
    std::vector<std::string> detect_anomalies(const BrowserFingerprint& fingerprint);
    bool is_bot_like(const BrowserFingerprint& fingerprint);
    bool is_suspicious(const BrowserFingerprint& fingerprint);
    
    // Comparison
    double calculate_similarity(const BrowserFingerprint& fp1, const BrowserFingerprint& fp2);
    bool are_too_similar(const BrowserFingerprint& fp1, const BrowserFingerprint& fp2, double threshold = 0.95);

private:
    // Validation rules
    std::map<std::string, std::vector<std::string>> validation_rules_;
    
    // Scoring weights
    double realism_weight_{0.4};
    double consistency_weight_{0.3};
    double uniqueness_weight_{0.3};
    
    // Private validation methods
    bool validate_user_agent_consistency(const BrowserFingerprint& fingerprint);
    bool validate_screen_resolution_consistency(const BrowserFingerprint& fingerprint);
    bool validate_timezone_consistency(const BrowserFingerprint& fingerprint);
    bool validate_hardware_consistency(const BrowserFingerprint& fingerprint);
    
    // Anomaly detection
    std::vector<std::string> check_user_agent_anomalies(const BrowserFingerprint& fingerprint);
    std::vector<std::string> check_hardware_anomalies(const BrowserFingerprint& fingerprint);
    std::vector<std::string> check_browser_anomalies(const BrowserFingerprint& fingerprint);
};

} // namespace gmail_infinity
