#pragma once

#include <string>
#include <vector>
#include <memory>
#include <map>
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <chrono>

#include <curl/curl.h>
#include <nlohmann/json.hpp>

namespace gmail_infinity {

enum class VoiceProvider {
    ELEVENLABS,
    AZURE_SPEECH,
    GOOGLE_TTS,
    AMAZON_POLLY,
    IBM_WATSON,
    MICROSOFT_SPEECH,
    OPENAI_TTS,
    COQUI_TTS,
    VOICE_RSS,
    NEURALSPEECH
};

enum class VoiceType {
    MALE,
    FEMALE,
    NEUTRAL,
    CHILD,
    ELDERLY,
    CUSTOM
};

enum class VoiceLanguage {
    ENGLISH_US,
    ENGLISH_UK,
    SPANISH,
    FRENCH,
    GERMAN,
    ITALIAN,
    PORTUGUESE,
    RUSSIAN,
    CHINESE,
    JAPANESE,
    KOREAN,
    ARABIC,
    HINDI
};

enum class VoiceEmotion {
    NEUTRAL,
    HAPPY,
    SAD,
    ANGRY,
    EXCITED,
    CALM,
    CONFIDENT,
    GENTLE,
    SERIOUS,
    FRIENDLY
};

struct VoiceProfile {
    std::string id;
    std::string name;
    VoiceProvider provider;
    VoiceType type;
    VoiceLanguage language;
    VoiceEmotion emotion;
    std::string voice_id; // Provider-specific voice ID
    double pitch;
    double speed;
    double volume;
    std::map<std::string, std::string> parameters;
    
    nlohmann::json to_json() const;
    void from_json(const nlohmann::json& j);
};

struct VoiceScript {
    std::string id;
    std::string title;
    std::string content;
    std::string purpose; // verification, greeting, confirmation, etc.
    VoiceLanguage language;
    int estimated_duration_seconds;
    std::map<std::string, std::string> variables;
    
    nlohmann::json to_json() const;
    void from_json(const nlohmann::json& j);
};

struct VoiceGeneration {
    std::string id;
    std::string script_id;
    VoiceProfile voice_profile;
    std::string audio_data; // Base64 encoded audio
    std::string audio_format; // mp3, wav, ogg, etc.
    std::string file_path;
    std::chrono::system_clock::time_point generated_at;
    std::chrono::milliseconds duration;
    double cost;
    std::string provider_response;
    std::map<std::string, std::string> metadata;
    
    nlohmann::json to_json() const;
    void from_json(const nlohmann::json& j);
};

struct VoiceProviderConfig {
    std::string api_key;
    std::string api_url;
    std::string api_secret;
    std::string region;
    int timeout_seconds = 30;
    int max_retries = 3;
    std::map<std::string, std::string> headers;
    std::map<std::string, std::string> parameters;
    double cost_per_minute = 0.30;
    
    nlohmann::json to_json() const;
    void from_json(const nlohmann::json& j);
};

class VoiceProviderInterface {
public:
    virtual ~VoiceProviderInterface() = default;
    
    // Core functionality
    virtual bool initialize(const VoiceProviderConfig& config) = 0;
    virtual std::shared_ptr<VoiceGeneration> generate_speech(const std::string& text,
                                                            const VoiceProfile& profile) = 0;
    virtual std::shared_ptr<VoiceGeneration> generate_speech_from_script(const VoiceScript& script,
                                                                         const VoiceProfile& profile) = 0;
    
    // Voice management
    virtual std::vector<VoiceProfile> get_available_voices() = 0;
    virtual std::shared_ptr<VoiceProfile> get_voice_by_id(const std::string& voice_id) = 0;
    virtual std::vector<VoiceProfile> get_voices_by_language(VoiceLanguage language) = 0;
    virtual std::vector<VoiceProfile> get_voices_by_type(VoiceType type) = 0;
    
    // Provider information
    virtual std::string get_provider_name() const = 0;
    virtual std::vector<VoiceLanguage> get_supported_languages() const = 0;
    virtual std::vector<VoiceType> get_supported_types() const = 0;
    virtual std::vector<std::string> get_supported_formats() const = 0;
    virtual double get_cost_per_minute() const = 0;
    virtual std::map<std::string, std::string> get_provider_info() const = 0;
    
    // Health and connectivity
    virtual bool test_connection() = 0;
    virtual double get_balance() const = 0;
    virtual std::map<std::string, double> get_usage_statistics() const = 0;

protected:
    VoiceProviderConfig config_;
    mutable std::mutex provider_mutex_;
    
    // HTTP helpers
    std::string make_http_request(const std::string& url, const std::string& method = "GET",
                                 const std::map<std::string, std::string>& params = {},
                                 const std::map<std::string, std::string>& headers = {});
    std::string make_post_request(const std::string& url, const std::string& data,
                                 const std::map<std::string, std::string>& headers = {});
    nlohmann::json parse_json_response(const std::string& response);
    
    // Utility methods
    std::string language_to_string(VoiceLanguage language) const;
    VoiceLanguage string_to_language(const std::string& language_str) const;
    std::string type_to_string(VoiceType type) const;
    VoiceType string_to_type(const std::string& type_str) const;
    std::string emotion_to_string(VoiceEmotion emotion) const;
    VoiceEmotion string_to_emotion(const std::string& emotion_str) const;
    std::string encode_base64(const std::string& data) const;
    std::string decode_base64(const std::string& encoded_data) const;
};

// ElevenLabs provider implementation
class ElevenLabsProvider : public VoiceProviderInterface {
public:
    bool initialize(const VoiceProviderConfig& config) override;
    std::shared_ptr<VoiceGeneration> generate_speech(const std::string& text,
                                                     const VoiceProfile& profile) override;
    std::shared_ptr<VoiceGeneration> generate_speech_from_script(const VoiceScript& script,
                                                               const VoiceProfile& profile) override;
    
    std::vector<VoiceProfile> get_available_voices() override;
    std::shared_ptr<VoiceProfile> get_voice_by_id(const std::string& voice_id) override;
    std::vector<VoiceProfile> get_voices_by_language(VoiceLanguage language) override;
    std::vector<VoiceProfile> get_voices_by_type(VoiceType type) override;
    
    std::string get_provider_name() const override { return "ElevenLabs"; }
    std::vector<VoiceLanguage> get_supported_languages() const override;
    std::vector<VoiceType> get_supported_types() const override;
    std::vector<std::string> get_supported_formats() const override;
    double get_cost_per_minute() const override { return 0.30; }
    std::map<std::string, std::string> get_provider_info() const override;
    
    bool test_connection() override;
    double get_balance() const override;
    std::map<std::string, double> get_usage_statistics() const override;

private:
    std::string build_api_url(const std::string& endpoint) const;
    std::map<std::string, std::string> get_default_headers() const;
    VoiceProfile parse_voice_profile(const nlohmann::json& voice_json) const;
    std::string build_elevenlabs_voice_settings(const VoiceProfile& profile) const;
};

// Azure Speech provider implementation
class AzureSpeechProvider : public VoiceProviderInterface {
public:
    bool initialize(const VoiceProviderConfig& config) override;
    std::shared_ptr<VoiceGeneration> generate_speech(const std::string& text,
                                                     const VoiceProfile& profile) override;
    std::shared_ptr<VoiceGeneration> generate_speech_from_script(const VoiceScript& script,
                                                               const VoiceProfile& profile) override;
    
    std::vector<VoiceProfile> get_available_voices() override;
    std::shared_ptr<VoiceProfile> get_voice_by_id(const std::string& voice_id) override;
    std::vector<VoiceProfile> get_voices_by_language(VoiceLanguage language) override;
    std::vector<VoiceProfile> get_voices_by_type(VoiceType type) override;
    
    std::string get_provider_name() const override { return "Azure Speech"; }
    std::vector<VoiceLanguage> get_supported_languages() const override;
    std::vector<VoiceType> get_supported_types() const override;
    std::vector<std::string> get_supported_formats() const override;
    double get_cost_per_minute() const override { return 0.16; }
    std::map<std::string, std::string> get_provider_info() const override;
    
    bool test_connection() override;
    double get_balance() const override;
    std::map<std::string, double> get_usage_statistics() const override;

private:
    std::string build_api_url(const std::string& endpoint) const;
    std::string get_access_token() const;
    bool authenticate();
    std::string access_token_;
    std::chrono::system_clock::time_point token_expiry_;
};

// Google TTS provider implementation
class GoogleTTSProvider : public VoiceProviderInterface {
public:
    bool initialize(const VoiceProviderConfig& config) override;
    std::shared_ptr<VoiceGeneration> generate_speech(const std::string& text,
                                                     const VoiceProfile& profile) override;
    std::shared_ptr<VoiceGeneration> generate_speech_from_script(const VoiceScript& script,
                                                               const VoiceProfile& profile) override;
    
    std::vector<VoiceProfile> get_available_voices() override;
    std::shared_ptr<VoiceProfile> get_voice_by_id(const std::string& voice_id) override;
    std::vector<VoiceProfile> get_voices_by_language(VoiceLanguage language) override;
    std::vector<VoiceProfile> get_voices_by_type(VoiceType type) override;
    
    std::string get_provider_name() const override { return "Google TTS"; }
    std::vector<VoiceLanguage> get_supported_languages() const override;
    std::vector<VoiceType> get_supported_types() const override;
    std::vector<std::string> get_supported_formats() const override;
    double get_cost_per_minute() const override { return 0.16; }
    std::map<std::string, std::string> get_provider_info() const override;
    
    bool test_connection() override;
    double get_balance() const override;
    std::map<std::string, double> get_usage_statistics() const override;

private:
    std::string build_api_url(const std::string& endpoint) const;
    std::string get_access_token() const;
    bool authenticate();
    std::string access_token_;
    std::chrono::system_clock::time_point token_expiry_;
};

class VoiceVerificationManager {
public:
    VoiceVerificationManager();
    ~VoiceVerificationManager();
    
    // Initialization
    bool initialize(const std::string& config_file = "config/voice_verification.yaml");
    void shutdown();
    
    // Provider management
    bool register_provider(std::unique_ptr<VoiceProviderInterface> provider);
    bool set_primary_provider(VoiceProvider provider);
    std::vector<VoiceProvider> get_available_providers();
    
    // Voice profile management
    bool register_voice_profile(const VoiceProfile& profile);
    std::shared_ptr<VoiceProfile> get_voice_profile(const std::string& profile_id);
    std::vector<VoiceProfile> get_all_voice_profiles();
    std::vector<VoiceProfile> get_voice_profiles_by_language(VoiceLanguage language);
    bool remove_voice_profile(const std::string& profile_id);
    
    // Script management
    bool register_script(const VoiceScript& script);
    std::shared_ptr<VoiceScript> get_script(const std::string& script_id);
    std::vector<VoiceScript> get_all_scripts();
    std::vector<VoiceScript> get_scripts_by_purpose(const std::string& purpose);
    bool remove_script(const std::string& script_id);
    
    // Voice generation
    std::shared_ptr<VoiceGeneration> generate_voice_verification(const std::string& text,
                                                               const VoiceProfile& profile);
    std::shared_ptr<VoiceGeneration> generate_from_script(const std::string& script_id,
                                                         const VoiceProfile& profile);
    
    // Predefined verification workflows
    std::shared_ptr<VoiceGeneration> generate_gmail_verification_code(const std::string& code,
                                                                     VoiceLanguage language = VoiceLanguage::ENGLISH_US);
    std::shared_ptr<VoiceGeneration> generate_phone_verification(const std::string& phone_number,
                                                               VoiceLanguage language = VoiceLanguage::ENGLISH_US);
    std::shared_ptr<VoiceGeneration> generate_account_confirmation(const std::string& account_name,
                                                                 VoiceLanguage language = VoiceLanguage::ENGLISH_US);
    
    // Batch generation
    std::vector<std::shared_ptr<VoiceGeneration>> generate_multiple_verifications(
        const std::vector<std::string>& texts,
        const VoiceProfile& profile);
    
    // Async generation
    std::future<std::shared_ptr<VoiceGeneration>> generate_voice_async(const std::string& text,
                                                                      const VoiceProfile& profile);
    
    // Provider rotation
    void enable_provider_rotation(bool enable);
    void set_rotation_strategy(const std::string& strategy);
    VoiceProvider get_next_provider();
    
    // Monitoring and statistics
    std::map<VoiceProvider, double> get_provider_balances();
    std::map<VoiceProvider, size_t> get_provider_usage_stats();
    std::map<VoiceLanguage, size_t> get_language_distribution();
    std::map<VoiceType, size_t> get_type_distribution();
    double get_total_cost() const;
    std::chrono::milliseconds get_average_generation_time() const;
    
    // Configuration
    void set_default_voice_profile(const std::string& profile_id);
    void set_default_language(VoiceLanguage language);
    void set_audio_format(const std::string& format);
    void set_output_directory(const std::string& directory);
    
    // File management
    bool save_audio_file(const VoiceGeneration& generation);
    std::shared_ptr<VoiceGeneration> load_audio_file(const std::string& file_path);
    bool delete_audio_file(const std::string& generation_id);
    std::vector<std::shared_ptr<VoiceGeneration>> get_audio_files(size_t limit = 100);
    
    // History and logging
    bool export_generation_history(const std::string& filename) const;
    bool import_generation_history(const std::string& filename);
    
    // Health checking
    void start_health_checking();
    void stop_health_checking();
    std::map<VoiceProvider, bool> get_provider_health_status();
    
    // Cost management
    void set_cost_limit(double daily_limit);
    double get_daily_cost() const;
    bool is_within_cost_limit() const;

private:
    // Provider management
    std::map<VoiceProvider, std::unique_ptr<VoiceProviderInterface>> providers_;
    VoiceProvider primary_provider_{VoiceProvider::ELEVENLABS};
    std::vector<VoiceProvider> provider_rotation_order_;
    std::map<VoiceProvider, bool> provider_health_;
    std::map<VoiceProvider, std::chrono::system_clock::time_point> last_provider_check_;
    
    // Voice profiles
    std::map<std::string, VoiceProfile> voice_profiles_;
    mutable std::mutex profiles_mutex_;
    
    // Scripts
    std::map<std::string, VoiceScript> scripts_;
    mutable std::mutex scripts_mutex_;
    
    // Configuration
    std::string default_profile_id_;
    VoiceLanguage default_language_{VoiceLanguage::ENGLISH_US};
    std::string audio_format_{"mp3"};
    std::string output_directory_{"voice_output/"};
    double cost_limit_{100.0}; // Daily cost limit
    
    // Statistics
    std::map<VoiceProvider, size_t> provider_usage_;
    std::map<VoiceLanguage, size_t> language_counts_;
    std::map<VoiceType, size_t> type_counts_;
    std::atomic<double> total_cost_{0.0};
    std::atomic<size_t> total_generations_{0};
    std::atomic<std::chrono::milliseconds::rep> total_generation_time_ms_{0};
    
    // Generation history
    std::vector<std::shared_ptr<VoiceGeneration>> generation_history_;
    mutable std::mutex history_mutex_;
    
    // Health checking
    std::unique_ptr<std::thread> health_check_thread_;
    std::atomic<bool> health_checking_running_{false};
    std::chrono::seconds health_check_interval_{300}; // 5 minutes
    
    // Thread safety
    mutable std::mutex manager_mutex_;
    std::atomic<bool> provider_rotation_enabled_{false};
    std::string rotation_strategy_{"round_robin"};
    std::atomic<size_t> current_provider_index_{0};
    
    // Private methods
    bool load_configuration(const std::string& config_file);
    void initialize_default_providers();
    void load_default_voice_profiles();
    void load_default_scripts();
    VoiceProviderInterface* get_current_provider();
    VoiceProviderInterface* get_provider_by_enum(VoiceProvider provider);
    
    // Generation management
    std::shared_ptr<VoiceGeneration> execute_generation(const std::string& text,
                                                       const VoiceProfile& profile,
                                                       VoiceProviderInterface* provider);
    void update_statistics(double cost, std::chrono::milliseconds generation_time);
    
    // Provider rotation
    void build_rotation_order();
    VoiceProvider select_next_provider();
    bool is_provider_healthy(VoiceProvider provider);
    
    // Health checking
    void health_check_loop();
    void check_provider_health(VoiceProvider provider);
    
    // Cost tracking
    void track_cost(double cost, VoiceProvider provider);
    void reset_daily_cost();
    std::chrono::system_clock::time_point last_cost_reset_;
    
    // Utility methods
    std::string generate_generation_id() const;
    std::string build_verification_script(const std::string& content, VoiceLanguage language);
    bool ensure_output_directory();
    std::string get_file_extension(const std::string& format) const;
    
    // Logging
    void log_voice_info(const std::string& message) const;
    void log_voice_error(const std::string& message) const;
    void log_voice_debug(const std::string& message) const;
};

// Voice verification result
struct VoiceVerificationResult {
    bool success;
    std::string audio_file_path;
    std::string script_content;
    VoiceProfile voice_profile;
    VoiceProvider provider;
    std::chrono::milliseconds generation_time;
    double cost;
    std::string error_message;
    
    nlohmann::json to_json() const;
    void from_json(const nlohmann::json& j);
};

// Voice verification orchestrator
class VoiceVerificationOrchestrator {
public:
    VoiceVerificationOrchestrator(std::shared_ptr<VoiceVerificationManager> manager);
    ~VoiceVerificationOrchestrator() = default;
    
    // High-level verification workflows
    VoiceVerificationResult create_verification_code_audio(const std::string& code,
                                                         VoiceLanguage language = VoiceLanguage::ENGLISH_US);
    VoiceVerificationResult create_phone_verification_audio(const std::string& phone_number,
                                                           VoiceLanguage language = VoiceLanguage::ENGLISH_US);
    VoiceVerificationResult create_account_setup_audio(const std::string& account_name,
                                                     VoiceLanguage language = VoiceLanguage::ENGLISH_US);
    
    // Async operations
    std::future<VoiceVerificationResult> create_verification_async(const std::string& content,
                                                                  VoiceLanguage language = VoiceLanguage::ENGLISH_US);
    
    // Batch operations
    std::vector<VoiceVerificationResult> create_multiple_verifications(
        const std::vector<std::string>& contents,
        VoiceLanguage language = VoiceLanguage::ENGLISH_US);

private:
    std::shared_ptr<VoiceVerificationManager> manager_;
    
    VoiceVerificationResult execute_verification_workflow(const std::string& content,
                                                          VoiceLanguage language);
    VoiceProfile select_optimal_voice_profile(VoiceLanguage language);
};

// Voice script template system
class VoiceScriptTemplate {
public:
    struct Template {
        std::string id;
        std::string name;
        std::string description;
        std::string purpose;
        std::map<VoiceLanguage, std::string> localized_content;
        std::vector<std::string> variables;
        std::vector<std::string> tags;
    };
    
    VoiceScriptTemplate();
    ~VoiceScriptTemplate() = default;
    
    bool load_templates(const std::string& filename = "data/voice_script_templates.json");
    bool save_templates(const std::string& filename = "data/voice_script_templates.json");
    
    std::shared_ptr<Template> get_template(const std::string& template_id);
    std::vector<Template> get_templates_by_purpose(const std::string& purpose);
    std::vector<Template> get_templates_by_tag(const std::string& tag);
    
    bool add_template(const Template& template_obj);
    bool remove_template(const std::string& template_id);
    
    VoiceScript generate_script_from_template(const std::string& template_id,
                                           const std::map<std::string, std::string>& variables,
                                           VoiceLanguage language);

private:
    std::map<std::string, Template> templates_;
    mutable std::mutex template_mutex_;
    
    void load_default_templates();
    Template create_verification_code_template();
    Template create_phone_verification_template();
    Template create_account_confirmation_template();
    Template create_welcome_message_template();
};

} // namespace gmail_infinity
