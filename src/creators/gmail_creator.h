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
#include <functional>

#include <nlohmann/json.hpp>

#include "../identity/persona_generator.h"
#include "../core/proxy_manager.h"
#include "../core/browser_controller.h"
#include "../core/fingerprint_generator.h"
#include "../verification/sms_providers.h"
#include "../verification/captcha_solver.h"
#include "../verification/email_recovery.h"
#include "../verification/voice_verification.h"
#include "../app/secure_vault.h"

namespace gmail_infinity {

enum class CreationStep {
    NOT_STARTED,
    LAUNCHING_BROWSER,
    NAVIGATING_TO_GMAIL,
    CLICKING_CREATE_ACCOUNT,
    ENTERING_NAME,
    ENTERING_BIRTHDAY,
    SELECTING_GENDER,
    ENTERING_USERNAME,
    ENTERING_PASSWORD,
    ENTERING_PHONE,
    VERIFYING_PHONE,
    ENTERING_RECOVERY_EMAIL,
    ACCEPTING_TERMS,
    VERIFYING_EMAIL,
    COMPLETING_SETUP,
    SUCCESS,
    FAILED
};

enum class CreationFailureReason {
    NONE,
    BROWSER_LAUNCH_FAILED,
    NAVIGATION_FAILED,
    CAPTCHA_FAILED,
    USERNAME_UNAVAILABLE,
    PHONE_VERIFICATION_FAILED,
    EMAIL_VERIFICATION_FAILED,
    TERMS_REJECTED,
    NETWORK_ERROR,
    PROXY_ERROR,
    TIMEOUT,
    UNKNOWN_ERROR
};

struct CreationProgress {
    CreationStep current_step;
    std::chrono::system_clock::time_point step_started_at;
    std::chrono::milliseconds step_duration;
    std::string step_description;
    std::string error_message;
    CreationFailureReason failure_reason;
    std::map<std::string, std::string> step_data;
    
    nlohmann::json to_json() const;
    void from_json(const nlohmann::json& j);
};

struct CreationResult {
    bool success;
    std::string email_address;
    std::string password;
    std::string recovery_email;
    std::string phone_number;
    HumanPersona persona;
    std::shared_ptr<Proxy> proxy_used;
    std::chrono::system_clock::time_point created_at;
    std::chrono::milliseconds total_creation_time;
    std::vector<CreationProgress> progress_history;
    CreationFailureReason failure_reason;
    std::string error_details;
    std::map<std::string, std::string> metadata;
    
    nlohmann::json to_json() const;
    void from_json(const nlohmann::json& j);
};

struct CreationConfig {
    // Browser settings
    bool headless = true;
    std::string user_agent;
    std::string window_size = "1920,1080";
    bool disable_gpu = false;
    bool enable_stealth = true;
    
    // Creation settings
    bool use_phone_verification = true;
    bool use_email_recovery = true;
    bool use_voice_verification = false;
    bool accept_terms_automatically = true;
    std::chrono::seconds step_timeout{60};
    std::chrono::seconds total_timeout{600};
    
    // Retry settings
    int max_retries = 3;
    std::chrono::seconds retry_delay{30};
    bool retry_on_username_failure = true;
    int username_attempts = 5;
    
    // Verification settings
    SMSProvider sms_provider = SMSProvider::FIVE_SIM;
    CaptchaProvider captcha_provider = CaptchaProvider::TWO_CAPTCHA;
    EmailProvider email_provider = EmailProvider::MAIL_TM;
    VoiceProvider voice_provider = VoiceProvider::ELEVENLABS;
    
    // Proxy settings
    bool use_proxy = true;
    std::string proxy_country = "US";
    bool rotate_proxy_on_failure = true;
    
    // Persona settings
    std::string target_country = "US";
    int min_age = 18;
    int max_age = 65;
    std::string preferred_language = "en";
    
    nlohmann::json to_json() const;
    void from_json(const nlohmann::json& j);
};

class GmailCreator {
public:
    GmailCreator();
    ~GmailCreator();
    
    // Initialization
    bool initialize(const CreationConfig& config);
    void shutdown();
    
    // Core creation methods
    std::future<CreationResult> create_account_async();
    CreationResult create_account_sync();
    std::future<CreationResult> create_account_with_persona_async(const HumanPersona& persona);
    CreationResult create_account_with_persona(const HumanPersona& persona);
    
    // Batch creation
    std::vector<std::future<CreationResult>> create_multiple_accounts_async(size_t count);
    std::vector<CreationResult> create_multiple_accounts_sync(size_t count);
    
    // Step-by-step creation (for debugging/customization)
    CreationProgress launch_browser();
    CreationProgress navigate_to_gmail_signup();
    CreationProgress click_create_account();
    CreationProgress enter_name(const NameComponents& name);
    CreationProgress enter_birthday(const DateComponents& birthday);
    CreationProgress select_gender(Gender gender);
    CreationProgress enter_username(const std::string& preferred_username);
    CreationProgress enter_password(const std::string& password);
    CreationProgress enter_phone_number(const std::string& phone_number);
    CreationProgress verify_phone_number(const std::string& verification_code);
    CreationProgress enter_recovery_email(const std::string& recovery_email);
    CreationProgress accept_terms();
    CreationProgress verify_recovery_email();
    CreationProgress complete_account_setup();
    
    // Progress monitoring
    CreationProgress get_current_progress() const;
    std::vector<CreationProgress> get_progress_history() const;
    bool is_creation_complete() const;
    void set_progress_callback(std::function<void(const CreationProgress&)> callback);
    
    // Configuration
    void update_config(const CreationConfig& config);
    CreationConfig get_config() const;
    
    // Statistics
    std::map<CreationStep, std::chrono::milliseconds> get_average_step_times() const;
    std::map<CreationFailureReason, size_t> get_failure_reasons() const;
    double get_success_rate() const;
    std::chrono::milliseconds get_average_creation_time() const;
    
    // History and logging
    std::vector<CreationResult> get_creation_history(size_t limit = 100);
    bool export_creation_history(const std::string& filename) const;
    bool import_creation_history(const std::string& filename);

private:
    // Core components
    std::shared_ptr<BrowserController> browser_;
    std::shared_ptr<ProxyManager> proxy_manager_;
    std::shared_ptr<FingerprintCache> fingerprint_cache_;
    std::shared_ptr<SMSVerificationManager> sms_manager_;
    std::shared_ptr<CaptchaSolver> captcha_solver_;
    std::shared_ptr<EmailRecoveryManager> email_manager_;
    std::shared_ptr<VoiceVerificationManager> voice_manager_;
    std::shared_ptr<SecureVault> secure_vault_;
    
    // Configuration
    CreationConfig config_;
    
    // Creation state
    std::atomic<CreationStep> current_step_{CreationStep::NOT_STARTED};
    CreationProgress current_progress_;
    std::vector<CreationProgress> progress_history_;
    std::shared_ptr<HumanPersona> current_persona_;
    std::shared_ptr<Proxy> current_proxy_;
    
    // Statistics
    std::map<CreationStep, std::vector<std::chrono::milliseconds>> step_times_;
    std::map<CreationFailureReason, size_t> failure_counts_;
    std::atomic<size_t> total_attempts_{0};
    std::atomic<size_t> successful_creations_{0};
    std::atomic<std::chrono::milliseconds::rep> total_creation_time_ms_{0};
    
    // Creation history
    std::vector<CreationResult> creation_history_;
    mutable std::mutex history_mutex_;
    
    // Callbacks
    std::function<void(const CreationProgress&)> progress_callback_;
    
    // Thread safety
    mutable std::mutex creator_mutex_;
    std::atomic<bool> creation_in_progress_{false};
    
    // Private methods
    CreationResult execute_creation_workflow();
    CreationResult execute_creation_with_persona(const HumanPersona& persona);
    
    // Step implementations
    CreationProgress execute_launch_browser();
    CreationProgress execute_navigate_to_gmail_signup();
    CreationProgress execute_click_create_account();
    CreationProgress execute_enter_name(const NameComponents& name);
    CreationProgress execute_enter_birthday(const DateComponents& birthday);
    CreationProgress execute_select_gender(Gender gender);
    CreationProgress execute_enter_username(const std::string& preferred_username);
    CreationProgress execute_enter_password(const std::string& password);
    CreationProgress execute_enter_phone_number(const std::string& phone_number);
    CreationProgress execute_verify_phone_number(const std::string& verification_code);
    CreationProgress execute_enter_recovery_email(const std::string& recovery_email);
    CreationProgress execute_accept_terms();
    CreationProgress execute_verify_recovery_email();
    CreationProgress execute_complete_account_setup();
    
    // Helper methods
    bool setup_browser_environment();
    bool setup_proxy_connection();
    bool setup_fingerprint();
    HumanPersona generate_or_use_persona();
    std::string generate_username_from_persona(const HumanPersona& persona);
    std::string generate_secure_password();
    std::string get_phone_verification_code();
    std::string get_email_verification_code(const std::string& recovery_email);
    
    // Retry and error handling
    CreationResult retry_creation(const CreationResult& failed_result);
    bool should_retry_step(CreationStep step, CreationFailureReason reason);
    void handle_step_failure(CreationStep step, CreationFailureReason reason, const std::string& error);
    
    // Validation methods
    bool validate_creation_result(const CreationResult& result);
    bool verify_account_creation(const std::string& email_address);
    
    // Progress tracking
    void update_progress(CreationStep step, const std::string& description = "");
    void update_progress_with_error(CreationStep step, CreationFailureReason reason, const std::string& error);
    void record_step_time(CreationStep step, std::chrono::milliseconds duration);
    
    // Statistics tracking
    void update_statistics(bool success, std::chrono::milliseconds total_time);
    void record_failure_reason(CreationFailureReason reason);
    
    // Utility methods
    std::string step_to_string(CreationStep step) const;
    std::string failure_reason_to_string(CreationFailureReason reason) const;
    std::chrono::milliseconds get_step_timeout(CreationStep step) const;
    std::string generate_creation_id() const;
    
    // Browser interaction helpers
    bool wait_for_element_and_click(const std::string& selector, int timeout_ms = 10000);
    bool wait_for_element_and_type(const std::string& selector, const std::string& text, bool clear_first = true, int timeout_ms = 10000);
    bool wait_for_element_and_select(const std::string& selector, const std::string& value, int timeout_ms = 10000);
    std::string get_element_text(const std::string& selector, int timeout_ms = 10000);
    bool wait_for_page_load(int timeout_ms = 30000);
    bool solve_captcha_if_present();
    
    // Logging
    void log_creator_info(const std::string& message) const;
    void log_creator_error(const std::string& message) const;
    void log_creator_debug(const std::string& message) const;
};

// Gmail creation orchestrator for managing multiple creators
class GmailCreationOrchestrator {
public:
    GmailCreationOrchestrator();
    ~GmailCreationOrchestrator();
    
    // Initialization
    bool initialize(const std::string& config_file = "config/gmail_creation.yaml");
    void shutdown();
    
    // Creator management
    bool add_creator(std::shared_ptr<GmailCreator> creator);
    bool remove_creator(const std::string& creator_id);
    std::vector<std::string> get_creator_ids();
    std::shared_ptr<GmailCreator> get_creator(const std::string& creator_id);
    
    // Batch creation orchestration
    std::vector<std::future<CreationResult>> create_accounts_batch(size_t count);
    std::vector<std::future<CreationResult>> create_accounts_with_distribution(
        const std::map<std::string, size_t>& country_distribution);
    
    // Load balancing
    void enable_load_balancing(bool enable);
    void set_load_balancing_strategy(const std::string& strategy);
    std::shared_ptr<GmailCreator> get_next_creator();
    
    // Monitoring and statistics
    std::map<std::string, CreationProgress> get_all_creator_progress();
    std::map<std::string, double> get_creator_success_rates();
    std::map<std::string, size_t> get_creator_usage_stats();
    std::vector<CreationResult> get_all_creation_history(size_t limit = 100);
    
    // Configuration
    void set_global_config(const CreationConfig& config);
    void set_max_concurrent_creations(size_t max_concurrent);
    void set_creation_rate_limit(size_t creations_per_minute);
    
    // Queue management
    void enqueue_creation_request(const CreationConfig& config = {});
    size_t get_queue_size() const;
    void clear_queue();
    
    // Health monitoring
    void start_health_monitoring();
    void stop_health_monitoring();
    std::map<std::string, bool> get_creator_health_status();

private:
    // Creator management
    std::map<std::string, std::shared_ptr<GmailCreator>> creators_;
    std::vector<std::string> creator_rotation_order_;
    std::map<std::string, bool> creator_health_;
    std::map<std::string, std::chrono::system_clock::time_point> last_creator_check_;
    
    // Configuration
    CreationConfig global_config_;
    size_t max_concurrent_creations_{5};
    size_t creations_per_minute_{10};
    std::chrono::seconds rate_limit_interval_{60};
    
    // Statistics
    std::map<std::string, size_t> creator_usage_;
    std::atomic<size_t> total_creations_{0};
    std::atomic<size_t> concurrent_creations_{0};
    
    // Request queue
    std::queue<CreationConfig> creation_queue_;
    mutable std::mutex queue_mutex_;
    std::condition_variable queue_cv_;
    
    // Health monitoring
    std::unique_ptr<std::thread> health_monitor_thread_;
    std::atomic<bool> health_monitoring_running_{false};
    std::chrono::seconds health_check_interval_{300}; // 5 minutes
    
    // Thread safety
    mutable std::mutex orchestrator_mutex_;
    std::atomic<bool> load_balancing_enabled_{false};
    std::string load_balancing_strategy_{"round_robin"};
    std::atomic<size_t> current_creator_index_{0};
    
    // Rate limiting
    std::atomic<size_t> creations_in_interval_{0};
    std::chrono::system_clock::time_point rate_limit_reset_time_;
    
    // Private methods
    bool load_configuration(const std::string& config_file);
    void build_creator_rotation();
    std::shared_ptr<GmailCreator> select_next_creator();
    bool is_creator_healthy(const std::string& creator_id);
    
    // Queue processing
    void process_queue();
    std::thread queue_processor_;
    std::atomic<bool> queue_processing_running_{false};
    
    // Rate limiting
    bool check_rate_limit();
    void reset_rate_limit();
    
    // Health monitoring
    void health_monitor_loop();
    void check_creator_health(const std::string& creator_id);
    
    // Utility methods
    void log_orchestrator_info(const std::string& message) const;
    void log_orchestrator_error(const std::string& message) const;
};

// Gmail creation result analyzer
class GmailCreationAnalyzer {
public:
    GmailCreationAnalyzer();
    ~GmailCreationAnalyzer() = default;
    
    // Result analysis
    std::map<CreationStep, double> analyze_step_success_rates(const std::vector<CreationResult>& results);
    std::map<CreationFailureReason, double> analyze_failure_reasons(const std::vector<CreationResult>& results);
    std::vector<std::string> identify_common_failure_patterns(const std::vector<CreationResult>& results);
    
    // Performance analysis
    std::chrono::milliseconds calculate_average_creation_time(const std::vector<CreationResult>& results);
    std::map<CreationStep, std::chrono::milliseconds> calculate_average_step_times(const std::vector<CreationResult>& results);
    std::vector<std::string> identify_performance_bottlenecks(const std::vector<CreationResult>& results);
    
    // Quality analysis
    std::vector<std::string> analyze_account_quality(const std::vector<CreationResult>& results);
    double calculate_overall_success_rate(const std::vector<CreationResult>& results);
    std::map<std::string, double> analyze_provider_effectiveness(const std::vector<CreationResult>& results);
    
    // Recommendations
    std::vector<std::string> generate_optimization_recommendations(const std::vector<CreationResult>& results);
    std::map<std::string, std::string> suggest_configuration_improvements(const std::vector<CreationResult>& results);

private:
    std::map<CreationStep, std::string> step_descriptions_;
    std::map<CreationFailureReason, std::string> failure_descriptions_;
    
    void initialize_descriptions();
    std::vector<CreationResult> filter_successful_results(const std::vector<CreationResult>& results);
    std::vector<CreationResult> filter_failed_results(const std::vector<CreationResult>& results);
};

} // namespace gmail_infinity
