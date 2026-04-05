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

#include "gmail_creator.h"
#include "../identity/persona_generator.h"
#include "../core/proxy_manager.h"
#include "../core/browser_controller.h"
#include "../core/fingerprint_generator.h"
#include "../verification/sms_providers.h"
#include "../verification/captcha_solver.h"
#include "../verification/email_recovery.h"
#include "../verification/voice_verification.h"

namespace gmail_infinity {

enum class RecoveryLinkStep {
    NOT_STARTED,
    LAUNCHING_BROWSER,
    NAVIGATING_TO_RECOVERY,
    ENTERING_EMAIL,
    SELECTING_RECOVERY_METHOD,
    HANDLING_PHONE_RECOVERY,
    HANDLING_EMAIL_RECOVERY,
    ENTERING_VERIFICATION_CODE,
    CREATING_NEW_PASSWORD,
    CONFIRMING_PASSWORD,
    SUCCESS,
    FAILED
};

enum class RecoveryMethod {
    PHONE_VERIFICATION,
    EMAIL_VERIFICATION,
    SECURITY_QUESTIONS,
    BACKUP_CODES,
    RECOVERY_EMAIL,
    ACCOUNT_RECOVERY_FORM
};

enum class RecoveryLinkType {
    PASSWORD_RESET,
    ACCOUNT_RECOVERY,
    TWO_FACTOR_RECOVERY,
    SECURITY_CHECK,
    VERIFICATION_RESEND,
    CUSTOM_RECOVERY
};

struct RecoveryLinkConfig {
    // Target account
    std::string target_email;
    std::string recovery_email;
    std::string phone_number;
    
    // Recovery method preferences
    std::vector<RecoveryMethod> preferred_methods;
    RecoveryMethod primary_method = RecoveryMethod::EMAIL_VERIFICATION;
    bool enable_fallback_methods = true;
    
    // Browser settings
    bool headless = true;
    std::string user_agent;
    std::string window_size = "1920,1080";
    bool enable_stealth = true;
    
    // Timing settings
    std::chrono::seconds step_timeout{60};
    std::chrono::seconds total_timeout{300};
    int max_retries = 3;
    std::chrono::seconds retry_delay{30};
    
    // Verification settings
    SMSProvider sms_provider = SMSProvider::FIVE_SIM;
    EmailProvider email_provider = EmailProvider::MAIL_TM;
    CaptchaProvider captcha_provider = CaptchaProvider::TWO_CAPTCHA;
    
    // New password settings
    bool generate_new_password = true;
    std::string custom_password;
    int password_length = 12;
    bool include_special_chars = true;
    bool include_numbers = true;
    
    // Link settings
    RecoveryLinkType link_type = RecoveryLinkType::PASSWORD_RESET;
    bool generate_direct_link = true;
    bool track_link_usage = true;
    std::chrono::hours link_expiry{24};
    
    // Security settings
    bool clear_browser_data = true;
    bool use_new_fingerprint = true;
    bool change_ip_address = true;
    
    nlohmann::json to_json() const;
    void from_json(const nlohmann::json& j);
};

struct RecoveryLinkProgress {
    RecoveryLinkStep current_step;
    std::chrono::system_clock::time_point step_started_at;
    std::chrono::milliseconds step_duration;
    std::string step_description;
    std::string current_url;
    std::string page_title;
    bool success;
    std::string error_message;
    RecoveryMethod current_method;
    std::map<std::string, std::string> step_data;
    
    nlohmann::json to_json() const;
    void from_json(const nlohmann::json& j);
};

struct RecoveryLinkResult {
    bool success;
    std::string recovery_link;
    std::string link_token;
    RecoveryLinkType link_type;
    std::string target_email;
    RecoveryMethod method_used;
    std::chrono::system_clock::time_point created_at;
    std::chrono::system_clock::time_point expires_at;
    std::chrono::milliseconds total_creation_time;
    std::vector<RecoveryLinkProgress> progress_history;
    std::string new_password;
    std::string error_details;
    std::map<std::string, std::string> metadata;
    std::vector<std::string> warnings;
    
    nlohmann::json to_json() const;
    void from_json(const nlohmann::json& j);
    bool is_link_expired() const;
    int get_hours_until_expiry() const;
};

class RecoveryLinkCreator {
public:
    RecoveryLinkCreator();
    ~RecoveryLinkCreator();
    
    // Initialization
    bool initialize(const RecoveryLinkConfig& config);
    void shutdown();
    
    // Core link creation methods
    std::future<RecoveryLinkResult> create_recovery_link_async();
    RecoveryLinkResult create_recovery_link_sync();
    std::future<RecoveryLinkResult> create_password_reset_link_async(const std::string& email);
    RecoveryLinkResult create_password_reset_link(const std::string& email);
    std::future<RecoveryLinkResult> create_account_recovery_link_async(const std::string& email);
    RecoveryLinkResult create_account_recovery_link(const std::string& email);
    
    // Method-specific link creation
    std::future<RecoveryLinkResult> create_phone_recovery_link_async(const std::string& email, const std::string& phone);
    RecoveryLinkResult create_phone_recovery_link(const std::string& email, const std::string& phone);
    std::future<RecoveryLinkResult> create_email_recovery_link_async(const std::string& email, const std::string& recovery_email);
    RecoveryLinkResult create_email_recovery_link(const std::string& email, const std::string& recovery_email);
    
    // Step-by-step creation
    RecoveryLinkProgress launch_browser();
    RecoveryLinkProgress navigate_to_recovery_page();
    RecoveryLinkProgress enter_target_email(const std::string& email);
    RecoveryLinkProgress select_recovery_method(RecoveryMethod method);
    RecoveryLinkProgress handle_phone_recovery(const std::string& phone);
    RecoveryLinkProgress handle_email_recovery(const std::string& recovery_email);
    RecoveryLinkProgress enter_verification_code(const std::string& code);
    RecoveryLinkProgress create_new_password(const std::string& password);
    RecoveryLinkProgress confirm_password(const std::string& password);
    
    // Link management
    std::future<bool> validate_recovery_link_async(const std::string& link);
    bool validate_recovery_link(const std::string& link);
    std::future<bool> check_link_status_async(const std::string& link);
    bool check_link_status(const std::string& link);
    std::future<bool> revoke_recovery_link_async(const std::string& link);
    bool revoke_recovery_link(const std::string& link);
    
    // Batch operations
    std::vector<std::future<RecoveryLinkResult>> create_multiple_links_async(const std::vector<std::string>& emails);
    std::vector<RecoveryLinkResult> create_multiple_links(const std::vector<std::string>& emails);
    std::vector<std::future<RecoveryLinkResult>> create_links_with_methods_async(
        const std::vector<std::pair<std::string, RecoveryMethod>>& email_method_pairs);
    std::vector<RecoveryLinkResult> create_links_with_methods(
        const std::vector<std::pair<std::string, RecoveryMethod>>& email_method_pairs);
    
    // Progress monitoring
    RecoveryLinkProgress get_current_progress() const;
    std::vector<RecoveryLinkProgress> get_progress_history() const;
    bool is_creation_complete() const;
    void set_progress_callback(std::function<void(const RecoveryLinkProgress&)> callback);
    
    // Configuration
    void update_config(const RecoveryLinkConfig& config);
    RecoveryLinkConfig get_config() const;
    
    // Link tracking
    std::future<std::map<std::string, std::string>> get_link_usage_stats_async(const std::string& link);
    std::map<std::string, std::string> get_link_usage_stats(const std::string& link);
    std::vector<std::string> get_active_links();
    std::vector<std::string> get_expired_links();
    
    // Advanced features
    bool take_screenshot_at_step(RecoveryLinkStep step, const std::string& filename = "");
    bool enable_network_logging(bool enable);
    std::vector<std::string> get_network_requests(RecoveryLinkStep step);
    
    // Statistics
    std::map<RecoveryLinkStep, std::chrono::milliseconds> get_average_step_times() const;
    std::map<RecoveryMethod, double> get_method_success_rates() const;
    std::map<RecoveryLinkType, double> get_link_type_success_rates() const;
    double get_overall_success_rate() const;
    std::chrono::milliseconds get_average_creation_time() const;
    
    // History and logging
    std::vector<RecoveryLinkResult> get_creation_history(size_t limit = 100);
    bool export_creation_history(const std::string& filename) const;
    bool export_screenshots(const std::string& directory) const;

private:
    // Core components
    std::shared_ptr<BrowserController> browser_;
    std::shared_ptr<ProxyManager> proxy_manager_;
    std::shared_ptr<FingerprintCache> fingerprint_cache_;
    std::shared_ptr<SMSVerificationManager> sms_manager_;
    std::shared_ptr<CaptchaSolver> captcha_solver_;
    std::shared_ptr<EmailRecoveryManager> email_manager_;
    std::shared_ptr<VoiceVerificationManager> voice_manager_;
    
    // Configuration
    RecoveryLinkConfig config_;
    
    // Creation state
    std::atomic<RecoveryLinkStep> current_step_{RecoveryLinkStep::NOT_STARTED};
    RecoveryLinkProgress current_progress_;
    std::vector<RecoveryLinkProgress> progress_history_;
    std::shared_ptr<Proxy> current_proxy_;
    RecoveryMethod current_method_{RecoveryMethod::EMAIL_VERIFICATION};
    
    // Statistics
    std::map<RecoveryLinkStep, std::vector<std::chrono::milliseconds>> step_times_;
    std::map<RecoveryMethod, std::vector<bool>> method_results_;
    std::map<RecoveryLinkType, std::vector<bool>> link_type_results_;
    std::atomic<size_t> total_attempts_{0};
    std::atomic<size_t> successful_creations_{0};
    std::atomic<std::chrono::milliseconds::rep> total_creation_time_ms_{0};
    
    // Creation history
    std::vector<RecoveryLinkResult> creation_history_;
    mutable std::mutex history_mutex_;
    
    // Link tracking
    std::map<std::string, RecoveryLinkResult> active_links_;
    std::map<std::string, std::chrono::system_clock::time_point> link_creation_times_;
    mutable std::mutex links_mutex_;
    
    // Debug features
    std::map<RecoveryLinkStep, std::string> screenshots_;
    bool network_logging_enabled_{false};
    std::map<RecoveryLinkStep, std::vector<std::string>> network_requests_;
    
    // Callbacks
    std::function<void(const RecoveryLinkProgress&)> progress_callback_;
    
    // Thread safety
    mutable std::mutex creator_mutex_;
    std::atomic<bool> creation_in_progress_{false};
    
    // Private methods
    RecoveryLinkResult execute_creation_workflow();
    RecoveryLinkResult execute_password_reset_workflow(const std::string& email);
    RecoveryLinkResult execute_account_recovery_workflow(const std::string& email);
    RecoveryLinkResult execute_method_specific_workflow(RecoveryMethod method);
    
    // Step implementations
    RecoveryLinkProgress execute_launch_browser();
    RecoveryLinkProgress execute_navigate_to_recovery_page();
    RecoveryLinkProgress execute_enter_target_email(const std::string& email);
    RecoveryLinkProgress execute_select_recovery_method(RecoveryMethod method);
    RecoveryLinkProgress execute_handle_phone_recovery(const std::string& phone);
    RecoveryLinkProgress execute_handle_email_recovery(const std::string& recovery_email);
    RecoveryLinkProgress execute_enter_verification_code(const std::string& code);
    RecoveryLinkProgress execute_create_new_password(const std::string& password);
    RecoveryLinkProgress execute_confirm_password(const std::string& password);
    
    // Helper methods
    bool setup_browser_environment();
    bool setup_proxy_connection();
    bool setup_fingerprint();
    std::string generate_recovery_url(RecoveryLinkType type);
    std::string generate_new_password();
    std::string get_verification_code();
    std::string extract_recovery_link_from_page();
    bool validate_recovery_link_format(const std::string& link);
    
    // Method-specific implementations
    RecoveryLinkResult execute_phone_recovery_workflow(const std::string& phone);
    RecoveryLinkResult execute_email_recovery_workflow(const std::string& recovery_email);
    RecoveryLinkResult execute_security_questions_workflow();
    RecoveryLinkResult execute_backup_codes_workflow();
    RecoveryLinkResult execute_recovery_form_workflow();
    
    // Browser interaction helpers
    bool wait_for_element_and_click(const std::string& selector, int timeout_ms = 10000);
    bool wait_for_element_and_type(const std::string& selector, const std::string& text, bool clear_first = true, int timeout_ms = 10000);
    bool wait_for_element_and_select(const std::string& selector, const std::string& value, int timeout_ms = 10000);
    std::string get_element_text(const std::string& selector, int timeout_ms = 10000);
    bool wait_for_page_load_and_element(const std::string& selector);
    
    // Link management
    std::string generate_link_token();
    void store_link_info(const std::string& link, const RecoveryLinkResult& result);
    void cleanup_expired_links();
    bool is_link_valid(const std::string& link);
    
    // Progress tracking
    void update_progress(RecoveryLinkStep step, const std::string& description = "");
    void update_progress_with_error(RecoveryLinkStep step, const std::string& error);
    void record_step_time(RecoveryLinkStep step, std::chrono::milliseconds duration);
    
    // Debug and logging
    void capture_screenshot(RecoveryLinkStep step);
    void log_network_request(const std::string& request);
    
    // Statistics tracking
    void update_statistics(bool success, std::chrono::milliseconds total_time);
    void record_method_result(RecoveryMethod method, bool success);
    void record_link_type_result(RecoveryLinkType type, bool success);
    
    // Utility methods
    std::string step_to_string(RecoveryLinkStep step) const;
    std::string method_to_string(RecoveryMethod method) const;
    std::string link_type_to_string(RecoveryLinkType type) const;
    std::chrono::milliseconds get_step_timeout(RecoveryLinkStep step) const;
    std::string generate_creation_id() const;
    
    // Validation methods
    bool validate_recovery_config(const RecoveryLinkConfig& config);
    bool validate_email_format(const std::string& email);
    bool validate_phone_format(const std::string& phone);
    
    // URL helpers
    std::string build_recovery_url(RecoveryLinkType type, const std::string& email);
    std::string extract_link_from_response(const std::string& response);
    bool is_valid_recovery_link(const std::string& link);
    
    // Logging
    void log_creator_info(const std::string& message) const;
    void log_creator_error(const std::string& message) const;
    void log_creator_debug(const std::string& message) const;
};

// Recovery link validator
class RecoveryLinkValidator {
public:
    RecoveryLinkValidator();
    ~RecoveryLinkValidator() = default;
    
    // Validation methods
    bool validate_link_format(const std::string& link);
    bool validate_link_content(const std::string& link);
    bool validate_link_expiry(const std::string& link);
    bool validate_link_accessibility(const std::string& link);
    
    // Link analysis
    std::map<std::string, std::string> extract_link_parameters(const std::string& link);
    RecoveryLinkType detect_link_type(const std::string& link);
    std::chrono::system_clock::time_point extract_expiry_time(const std::string& link);
    
    // Security checks
    bool is_suspicious_link(const std::string& link);
    std::vector<std::string> check_link_security(const std::string& link);
    bool is_phishing_attempt(const std::string& link);

private:
    std::vector<std::regex> link_patterns_;
    std::vector<std::regex> suspicious_patterns_;
    
    void initialize_patterns();
    bool matches_pattern(const std::string& link, const std::vector<std::regex>& patterns);
    std::string extract_parameter(const std::string& link, const std::string& parameter);
};

// Recovery link manager
class RecoveryLinkManager {
public:
    RecoveryLinkManager();
    ~RecoveryLinkManager() = default;
    
    // Link storage and retrieval
    bool store_link(const std::string& link, const RecoveryLinkResult& result);
    std::shared_ptr<RecoveryLinkResult> get_link(const std::string& link);
    std::vector<RecoveryLinkResult> get_all_links();
    std::vector<RecoveryLinkResult> get_links_by_type(RecoveryLinkType type);
    std::vector<RecoveryLinkResult> get_active_links();
    std::vector<RecoveryLinkResult> get_expired_links();
    
    // Link lifecycle management
    bool activate_link(const std::string& link);
    bool deactivate_link(const std::string& link);
    bool revoke_link(const std::string& link);
    bool extend_link_expiry(const std::string& link, std::chrono::hours extension);
    
    // Link analytics
    std::map<std::string, size_t> get_link_usage_statistics();
    std::map<RecoveryLinkType, size_t> get_link_type_distribution();
    std::map<RecoveryMethod, size_t> get_method_distribution();
    double calculate_link_success_rate();
    
    // Cleanup and maintenance
    void cleanup_expired_links();
    void cleanup_inactive_links(std::chrono::hours inactive_threshold = std::chrono::hours(24 * 7));
    
    // Export/Import
    bool export_links(const std::string& filename) const;
    bool import_links(const std::string& filename);

private:
    std::map<std::string, RecoveryLinkResult> stored_links_;
    std::map<std::string, std::chrono::system_clock::time_point> link_access_times_;
    std::map<std::string, size_t> link_usage_counts_;
    mutable std::mutex links_mutex_;
    
    void update_link_access_time(const std::string& link);
    void increment_link_usage(const std::string& link);
};

// Recovery link analyzer
class RecoveryLinkAnalyzer {
public:
    RecoveryLinkAnalyzer();
    ~RecoveryLinkAnalyzer() = default;
    
    // Analysis methods
    std::map<RecoveryLinkStep, double> analyze_step_success_rates(const std::vector<RecoveryLinkResult>& results);
    std::map<RecoveryMethod, double> analyze_method_effectiveness(const std::vector<RecoveryLinkResult>& results);
    std::map<RecoveryLinkType, double> analyze_link_type_performance(const std::vector<RecoveryLinkResult>& results);
    std::vector<std::string> identify_common_failures(const std::vector<RecoveryLinkResult>& results);
    
    // Performance analysis
    std::chrono::milliseconds calculate_average_creation_time(const std::vector<RecoveryLinkResult>& results);
    std::map<RecoveryLinkStep, std::chrono::milliseconds> calculate_average_step_times(const std::vector<RecoveryLinkResult>& results);
    std::vector<std::string> identify_performance_bottlenecks(const std::vector<RecoveryLinkResult>& results);
    
    // Link usage analysis
    std::map<std::string, double> analyze_link_usage_patterns(const std::vector<RecoveryLinkResult>& results);
    std::vector<std::string> identify_unused_links(const std::vector<RecoveryLinkResult>& results);
    std::chrono::milliseconds calculate_average_link_lifetime(const std::vector<RecoveryLinkResult>& results);
    
    // Recommendations
    std::vector<std::string> generate_optimization_recommendations(const std::vector<RecoveryLinkResult>& results);
    std::map<std::string, std::string> suggest_config_improvements(const std::vector<RecoveryLinkResult>& results);

private:
    std::map<RecoveryLinkStep, std::string> step_descriptions_;
    std::map<RecoveryMethod, std::string> method_descriptions_;
    std::map<RecoveryLinkType, std::string> link_type_descriptions_;
    
    void initialize_descriptions();
    std::vector<RecoveryLinkResult> filter_successful_results(const std::vector<RecoveryLinkResult>& results);
    std::vector<RecoveryLinkResult> filter_failed_results(const std::vector<RecoveryLinkResult>& results);
};

} // namespace gmail_infinity
