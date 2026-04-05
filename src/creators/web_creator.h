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

enum class WebCreationStep {
    NOT_STARTED,
    LAUNCHING_BROWSER,
    NAVIGATING_TO_GMAIL,
    DETECTING_SIGNUP_FLOW,
    HANDLING_CAPTCHA,
    ENTERING_PERSONAL_INFO,
    SELECTING_USERNAME,
    SETTING_PASSWORD,
    PHONE_VERIFICATION,
    EMAIL_VERIFICATION,
    SECURITY_SETUP,
    PROFILE_CUSTOMIZATION,
    COMPLETING_PROCESS,
    SUCCESS,
    FAILED
};

enum class WebCreationMethod {
    DIRECT_SIGNUP,
    THROUGH_GOOGLE_ACCOUNT,
    THROUGH_ANDROID_SETUP,
    THROUGH_WORKSPACE_SETUP,
    THROUGH_FAMILY_LINK,
    THROUGH_EDUCATION_PORTAL
};

struct WebCreationConfig {
    // Browser configuration
    bool headless = true;
    std::string user_agent;
    std::string window_size = "1920,1080";
    bool enable_stealth = true;
    bool disable_web_security = false;
    bool ignore_certificate_errors = true;
    
    // Creation method
    WebCreationMethod method = WebCreationMethod::DIRECT_SIGNUP;
    std::string target_url = "https://accounts.google.com/signup";
    
    // Timing and retry
    std::chrono::seconds page_load_timeout{30};
    std::chrono::seconds element_wait_timeout{10};
    std::chrono::seconds total_creation_timeout{600};
    int max_retries = 3;
    std::chrono::seconds retry_delay{30};
    
    // Verification preferences
    bool prefer_phone_verification = true;
    bool prefer_email_recovery = true;
    bool enable_voice_option = false;
    
    // Security and stealth
    bool randomize_fingerprint = true;
    bool inject_anti_detection_scripts = true;
    bool simulate_human_behavior = true;
    std::chrono::milliseconds typing_delay{50};
    std::chrono::milliseconds mouse_delay{100};
    
    // Proxy configuration
    bool use_proxy = true;
    std::string preferred_country = "US";
    bool rotate_proxy_on_failure = true;
    
    // Persona configuration
    bool generate_random_persona = true;
    std::string target_language = "en";
    int age_range_min = 18;
    int age_range_max = 65;
    
    // Advanced options
    bool skip_optional_steps = false;
    bool accept_all_policies = true;
    bool enable_debug_mode = false;
    std::string custom_user_data_dir;
    
    nlohmann::json to_json() const;
    void from_json(const nlohmann::json& j);
};

struct WebCreationProgress {
    WebCreationStep current_step;
    std::chrono::system_clock::time_point step_started_at;
    std::chrono::milliseconds step_duration;
    std::string step_description;
    std::string current_url;
    std::string page_title;
    bool success;
    std::string error_message;
    std::map<std::string, std::string> step_data;
    
    nlohmann::json to_json() const;
    void from_json(const nlohmann::json& j);
};

struct WebCreationResult {
    bool success;
    std::string email_address;
    std::string password;
    std::string recovery_email;
    std::string phone_number;
    HumanPersona persona_used;
    WebCreationMethod method_used;
    std::shared_ptr<Proxy> proxy_used;
    std::chrono::system_clock::time_point created_at;
    std::chrono::milliseconds total_creation_time;
    std::vector<WebCreationProgress> progress_history;
    std::string error_details;
    std::map<std::string, std::string> metadata;
    std::vector<std::string> warnings;
    
    nlohmann::json to_json() const;
    void from_json(const nlohmann::json& j);
};

class WebCreator {
public:
    WebCreator();
    ~WebCreator();
    
    // Initialization
    bool initialize(const WebCreationConfig& config);
    void shutdown();
    
    // Core creation methods
    std::future<WebCreationResult> create_account_async();
    WebCreationResult create_account_sync();
    std::future<WebCreationResult> create_account_with_method_async(WebCreationMethod method);
    WebCreationResult create_account_with_method(WebCreationMethod method);
    std::future<WebCreationResult> create_account_with_persona_async(const HumanPersona& persona);
    WebCreationResult create_account_with_persona(const HumanPersona& persona);
    
    // Method-specific creation
    std::future<WebCreationResult> create_direct_signup_async();
    WebCreationResult create_direct_signup();
    std::future<WebCreationResult> create_through_google_account_async();
    WebCreationResult create_through_google_account();
    std::future<WebCreationResult> create_through_android_setup_async();
    WebCreationResult create_through_android_setup();
    std::future<WebCreationResult> create_through_workspace_async();
    WebCreationResult create_through_workspace();
    std::future<WebCreationResult> create_through_family_link_async();
    WebCreationResult create_through_family_link();
    std::future<WebCreationResult> create_through_education_portal_async();
    WebCreationResult create_through_education_portal();
    
    // Step-by-step creation
    WebCreationProgress launch_browser();
    WebCreationProgress navigate_to_signup_page();
    WebCreationProgress detect_signup_flow();
    WebCreationProgress handle_captcha_if_present();
    WebCreationProgress enter_personal_information(const HumanPersona& persona);
    WebCreationProgress select_username(const std::string& preferred_username);
    WebCreationProgress set_password(const std::string& password);
    WebCreationProgress complete_phone_verification();
    WebCreationProgress complete_email_verification();
    WebCreationProgress setup_security_options();
    WebCreationProgress customize_profile();
    WebCreationProgress complete_creation_process();
    
    // Progress monitoring
    WebCreationProgress get_current_progress() const;
    std::vector<WebCreationProgress> get_progress_history() const;
    bool is_creation_complete() const;
    void set_progress_callback(std::function<void(const WebCreationProgress&)> callback);
    
    // Configuration
    void update_config(const WebCreationConfig& config);
    WebCreationConfig get_config() const;
    
    // Advanced features
    bool take_screenshot_at_step(WebCreationStep step, const std::string& filename = "");
    bool save_page_source_at_step(WebCreationStep step, const std::string& filename = "");
    bool enable_network_logging(bool enable);
    std::vector<std::string> get_network_requests(WebCreationStep step);
    
    // Statistics
    std::map<WebCreationStep, std::chrono::milliseconds> get_average_step_times() const;
    std::map<WebCreationMethod, double> get_method_success_rates() const;
    double get_overall_success_rate() const;
    std::chrono::milliseconds get_average_creation_time() const;
    
    // History and logging
    std::vector<WebCreationResult> get_creation_history(size_t limit = 100);
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
    WebCreationConfig config_;
    
    // Creation state
    std::atomic<WebCreationStep> current_step_{WebCreationStep::NOT_STARTED};
    WebCreationProgress current_progress_;
    std::vector<WebCreationProgress> progress_history_;
    std::shared_ptr<HumanPersona> current_persona_;
    std::shared_ptr<Proxy> current_proxy_;
    WebCreationMethod current_method_{WebCreationMethod::DIRECT_SIGNUP};
    
    // Statistics
    std::map<WebCreationStep, std::vector<std::chrono::milliseconds>> step_times_;
    std::map<WebCreationMethod, std::vector<bool>> method_results_;
    std::atomic<size_t> total_attempts_{0};
    std::atomic<size_t> successful_creations_{0};
    std::atomic<std::chrono::milliseconds::rep> total_creation_time_ms_{0};
    
    // Creation history
    std::vector<WebCreationResult> creation_history_;
    mutable std::mutex history_mutex_;
    
    // Debug features
    std::map<WebCreationStep, std::string> screenshots_;
    std::map<WebCreationStep, std::string> page_sources_;
    bool network_logging_enabled_{false};
    std::map<WebCreationStep, std::vector<std::string>> network_requests_;
    
    // Callbacks
    std::function<void(const WebCreationProgress&)> progress_callback_;
    
    // Thread safety
    mutable std::mutex creator_mutex_;
    std::atomic<bool> creation_in_progress_{false};
    
    // Private methods
    WebCreationResult execute_creation_workflow();
    WebCreationResult execute_creation_with_method(WebCreationMethod method);
    WebCreationResult execute_creation_with_persona(const HumanPersona& persona);
    
    // Method-specific implementations
    WebCreationResult execute_direct_signup();
    WebCreationResult execute_through_google_account();
    WebCreationResult execute_through_android_setup();
    WebCreationResult execute_through_workspace();
    WebCreationResult execute_through_family_link();
    WebCreationResult execute_through_education_portal();
    
    // Step implementations
    WebCreationProgress execute_launch_browser();
    WebCreationProgress execute_navigate_to_signup_page();
    WebCreationProgress execute_detect_signup_flow();
    WebCreationProgress execute_handle_captcha_if_present();
    WebCreationProgress execute_enter_personal_information(const HumanPersona& persona);
    WebCreationProgress execute_select_username(const std::string& preferred_username);
    WebCreationProgress execute_set_password(const std::string& password);
    WebCreationProgress execute_complete_phone_verification();
    WebCreationProgress execute_complete_email_verification();
    WebCreationProgress execute_setup_security_options();
    WebCreationProgress execute_customize_profile();
    WebCreationProgress execute_complete_creation_process();
    
    // Helper methods
    bool setup_browser_environment();
    bool setup_proxy_connection();
    bool setup_fingerprint();
    HumanPersona generate_or_use_persona();
    std::string generate_username_from_persona(const HumanPersona& persona);
    std::string generate_secure_password();
    bool handle_captcha();
    bool simulate_human_typing(const std::string& selector, const std::string& text);
    bool simulate_human_click(const std::string& selector);
    bool wait_for_page_load_and_element(const std::string& selector);
    
    // Flow detection
    WebCreationMethod detect_current_signup_flow();
    bool is_direct_signup_flow();
    bool is_google_account_flow();
    bool is_android_setup_flow();
    bool is_workspace_signup_flow();
    bool is_family_link_flow();
    bool is_education_portal_flow();
    
    // URL and navigation helpers
    std::string get_signup_url_for_method(WebCreationMethod method);
    bool navigate_to_method_specific_page(WebCreationMethod method);
    bool handle_method_specific_redirections();
    
    // Progress tracking
    void update_progress(WebCreationStep step, const std::string& description = "");
    void update_progress_with_error(WebCreationStep step, const std::string& error);
    void record_step_time(WebCreationStep step, std::chrono::milliseconds duration);
    
    // Debug and logging
    void capture_screenshot(WebCreationStep step);
    void capture_page_source(WebCreationStep step);
    void log_network_request(const std::string& request);
    
    // Statistics tracking
    void update_statistics(bool success, std::chrono::milliseconds total_time);
    void record_method_result(WebCreationMethod method, bool success);
    
    // Utility methods
    std::string step_to_string(WebCreationStep step) const;
    std::string method_to_string(WebCreationMethod method) const;
    std::chrono::milliseconds get_step_timeout(WebCreationStep step) const;
    std::string generate_creation_id() const;
    
    // Browser interaction helpers
    bool wait_for_element_and_click(const std::string& selector, int timeout_ms = 10000);
    bool wait_for_element_and_type(const std::string& selector, const std::string& text, bool clear_first = true, int timeout_ms = 10000);
    bool wait_for_element_and_select(const std::string& selector, const std::string& value, int timeout_ms = 10000);
    std::string get_element_text(const std::string& selector, int timeout_ms = 10000);
    bool wait_for_navigation_complete(int timeout_ms = 30000);
    
    // Human behavior simulation
    void simulate_mouse_movement(const std::string& selector);
    void simulate_scroll_behavior();
    void simulate_reading_time(int word_count);
    void simulate_hesitation();
    
    // Logging
    void log_creator_info(const std::string& message) const;
    void log_creator_error(const std::string& message) const;
    void log_creator_debug(const std::string& message) const;
};

// Web creation flow detector
class WebFlowDetector {
public:
    WebFlowDetector();
    ~WebFlowDetector() = default;
    
    WebCreationMethod detect_flow_from_url(const std::string& url);
    WebCreationMethod detect_flow_from_page_content(const std::string& content);
    WebCreationMethod detect_flow_from_page_elements(const std::vector<std::string>& elements);
    
    bool is_signup_page(const std::string& url, const std::string& content);
    bool is_captcha_present(const std::string& content);
    bool is_phone_verification_required(const std::string& content);
    bool is_email_verification_required(const std::string& content);
    
    std::vector<std::string> get_required_fields(WebCreationMethod method);
    std::vector<std::string> get_optional_fields(WebCreationMethod method);

private:
    std::map<WebCreationMethod, std::vector<std::string>> method_urls_;
    std::map<WebCreationMethod, std::vector<std::string>> method_indicators_;
    
    void initialize_method_patterns();
    bool matches_pattern(const std::string& content, const std::vector<std::string>& patterns);
};

// Web creation optimizer
class WebCreationOptimizer {
public:
    WebCreationOptimizer();
    ~WebCreationOptimizer() = default;
    
    // Optimization analysis
    WebCreationConfig optimize_config_for_success(const std::vector<WebCreationResult>& results);
    WebCreationConfig optimize_config_for_speed(const std::vector<WebCreationResult>& results);
    WebCreationConfig optimize_config_for_stealth(const std::vector<WebCreationResult>& results);
    
    // Performance analysis
    std::vector<std::string> identify_slow_steps(const std::vector<WebCreationResult>& results);
    std::vector<std::string> identify_failure_points(const std::vector<WebCreationResult>& results);
    std::map<WebCreationMethod, double> analyze_method_effectiveness(const std::vector<WebCreationResult>& results);
    
    // Recommendations
    std::vector<std::string> generate_optimization_recommendations(const std::vector<WebCreationResult>& results);
    std::map<std::string, std::string> suggest_config_improvements(const std::vector<WebCreationResult>& results);

private:
    std::map<WebCreationStep, std::string> step_optimization_tips_;
    std::map<WebCreationMethod, std::string> method_optimization_tips_;
    
    void initialize_optimization_tips();
    double calculate_step_success_rate(WebCreationStep step, const std::vector<WebCreationResult>& results);
    std::chrono::milliseconds calculate_average_step_time(WebCreationStep step, const std::vector<WebCreationResult>& results);
};

} // namespace gmail_infinity
