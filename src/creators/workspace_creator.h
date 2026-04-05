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

enum class WorkspaceCreationStep {
    NOT_STARTED,
    LAUNCHING_BROWSER,
    NAVIGATING_TO_WORKSPACE,
    INITIATING_SIGNUP,
    ENTERING_BUSINESS_INFO,
    CONFIGURING_DOMAIN,
    SETTING_UP_USERS,
    VERIFYING_DOMAIN,
    CONFIGURING_SECURITY,
    SETTING_UP_BILLING,
    FINALIZING_SETUP,
    SUCCESS,
    FAILED
};

enum class WorkspaceType {
    BUSINESS_STARTER,
    BUSINESS_STANDARD,
    BUSINESS_PLUS,
    ENTERPRISE_STANDARD,
    ENTERPRISE_PLUS,
    EDUCATION_STANDARD,
    EDUCATION_PLUS,
    NONPROFIT,
    GOVERNMENT,
    CUSTOM
};

enum class DomainStatus {
    NOT_CONFIGURED,
    PENDING_VERIFICATION,
    VERIFIED,
    FAILED_VERIFICATION,
    CUSTOM_DOMAIN,
    GOOGLE_DOMAIN
};

struct WorkspaceConfig {
    // Basic information
    std::string organization_name;
    std::string organization_type;
    std::string industry;
    std::string website;
    std::string phone_number;
    std::string address;
    std::string city;
    std::string state;
    std::string country;
    std::string postal_code;
    
    // Workspace settings
    WorkspaceType workspace_type = WorkspaceType::BUSINESS_STARTER;
    int initial_user_count = 1;
    int max_user_limit = 10;
    bool use_custom_domain = false;
    std::string domain_name;
    
    // Admin account
    std::string admin_first_name;
    std::string admin_last_name;
    std::string admin_email;
    std::string admin_phone;
    std::string admin_title;
    
    // Security settings
    bool enable_2fa = true;
    bool enforce_strong_passwords = true;
    bool enable_sso = false;
    std::string sso_provider;
    
    // Billing settings
    std::string billing_currency = "USD";
    std::string payment_method;
    bool enable_auto_renewal = true;
    
    // Advanced settings
    bool enable_gmail = true;
    bool enable_drive = true;
    bool enable_calendar = true;
    bool enable_meet = true;
    bool enable_chat = true;
    bool enable_sites = false;
    bool enable_groups = false;
    
    nlohmann::json to_json() const;
    void from_json(const nlohmann::json& j);
};

struct WorkspaceCreationProgress {
    WorkspaceCreationStep current_step;
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

struct WorkspaceCreationResult {
    bool success;
    std::string workspace_id;
    std::string admin_email;
    std::string admin_password;
    std::string domain_name;
    WorkspaceType workspace_type;
    std::vector<std::string> created_user_emails;
    std::chrono::system_clock::time_point created_at;
    std::chrono::milliseconds total_creation_time;
    std::vector<WorkspaceCreationProgress> progress_history;
    std::string error_details;
    std::map<std::string, std::string> metadata;
    std::map<std::string, std::string> billing_info;
    
    nlohmann::json to_json() const;
    void from_json(const nlohmann::json& j);
};

struct WorkspaceUser {
    std::string user_id;
    std::string email;
    std::string first_name;
    std::string last_name;
    std::string password;
    std::string role; // admin, user, manager
    std::string department;
    std::string title;
    bool is_suspended;
    std::chrono::system_clock::time_point created_at;
    
    nlohmann::json to_json() const;
    void from_json(const nlohmann::json& j);
};

class WorkspaceCreator {
public:
    WorkspaceCreator();
    ~WorkspaceCreator();
    
    // Initialization
    bool initialize(const WorkspaceConfig& config);
    void shutdown();
    
    // Core creation methods
    std::future<WorkspaceCreationResult> create_workspace_async();
    WorkspaceCreationResult create_workspace_sync();
    std::future<WorkspaceCreationResult> create_workspace_with_config_async(const WorkspaceConfig& config);
    WorkspaceCreationResult create_workspace_with_config(const WorkspaceConfig& config);
    
    // Step-by-step creation
    WorkspaceCreationProgress launch_browser();
    WorkspaceCreationProgress navigate_to_workspace_signup();
    WorkspaceCreationProgress initiate_signup();
    WorkspaceCreationProgress enter_business_information();
    WorkspaceCreationProgress configure_domain();
    WorkspaceCreationProgress setup_initial_users();
    WorkspaceCreationProgress verify_domain();
    WorkspaceCreationProgress configure_security_settings();
    WorkspaceCreationProgress setup_billing();
    WorkspaceCreationProgress finalize_workspace_setup();
    
    // User management
    std::future<std::vector<WorkspaceUser>> create_users_async(const std::vector<HumanPersona>& personas);
    std::vector<WorkspaceUser> create_users(const std::vector<HumanPersona>& personas);
    std::future<WorkspaceUser> create_user_async(const HumanPersona& persona);
    WorkspaceUser create_user(const HumanPersona& persona);
    
    // Domain management
    std::future<bool> verify_domain_async(const std::string& domain);
    bool verify_domain(const std::string& domain);
    std::future<bool> add_custom_domain_async(const std::string& domain);
    bool add_custom_domain(const std::string& domain);
    
    // Progress monitoring
    WorkspaceCreationProgress get_current_progress() const;
    std::vector<WorkspaceCreationProgress> get_progress_history() const;
    bool is_creation_complete() const;
    void set_progress_callback(std::function<void(const WorkspaceCreationProgress&)> callback);
    
    // Configuration
    void update_config(const WorkspaceConfig& config);
    WorkspaceConfig get_config() const;
    
    // Workspace management
    std::future<bool> suspend_user_async(const std::string& user_email);
    bool suspend_user(const std::string& user_email);
    std::future<bool> delete_user_async(const std::string& user_email);
    bool delete_user(const std::string& user_email);
    std::future<std::vector<WorkspaceUser>> list_users_async();
    std::vector<WorkspaceUser> list_users();
    
    // Advanced features
    bool take_screenshot_at_step(WorkspaceCreationStep step, const std::string& filename = "");
    bool enable_network_logging(bool enable);
    std::vector<std::string> get_network_requests(WorkspaceCreationStep step);
    
    // Statistics
    std::map<WorkspaceCreationStep, std::chrono::milliseconds> get_average_step_times() const;
    std::map<WorkspaceType, double> get_workspace_type_success_rates() const;
    double get_overall_success_rate() const;
    std::chrono::milliseconds get_average_creation_time() const;
    
    // History and logging
    std::vector<WorkspaceCreationResult> get_creation_history(size_t limit = 100);
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
    WorkspaceConfig config_;
    
    // Creation state
    std::atomic<WorkspaceCreationStep> current_step_{WorkspaceCreationStep::NOT_STARTED};
    WorkspaceCreationProgress current_progress_;
    std::vector<WorkspaceCreationProgress> progress_history_;
    std::shared_ptr<HumanPersona> current_persona_;
    std::shared_ptr<Proxy> current_proxy_;
    
    // Statistics
    std::map<WorkspaceCreationStep, std::vector<std::chrono::milliseconds>> step_times_;
    std::map<WorkspaceType, std::vector<bool>> workspace_type_results_;
    std::atomic<size_t> total_attempts_{0};
    std::atomic<size_t> successful_creations_{0};
    std::atomic<std::chrono::milliseconds::rep> total_creation_time_ms_{0};
    
    // Creation history
    std::vector<WorkspaceCreationResult> creation_history_;
    mutable std::mutex history_mutex_;
    
    // Debug features
    std::map<WorkspaceCreationStep, std::string> screenshots_;
    bool network_logging_enabled_{false};
    std::map<WorkspaceCreationStep, std::vector<std::string>> network_requests_;
    
    // Callbacks
    std::function<void(const WorkspaceCreationProgress&)> progress_callback_;
    
    // Thread safety
    mutable std::mutex creator_mutex_;
    std::atomic<bool> creation_in_progress_{false};
    
    // Private methods
    WorkspaceCreationResult execute_creation_workflow();
    WorkspaceCreationResult execute_creation_with_config(const WorkspaceConfig& config);
    
    // Step implementations
    WorkspaceCreationProgress execute_launch_browser();
    WorkspaceCreationProgress execute_navigate_to_workspace_signup();
    WorkspaceCreationProgress execute_initiate_signup();
    WorkspaceCreationProgress execute_enter_business_information();
    WorkspaceCreationProgress execute_configure_domain();
    WorkspaceCreationProgress execute_setup_initial_users();
    WorkspaceCreationProgress execute_verify_domain();
    WorkspaceCreationProgress execute_configure_security_settings();
    WorkspaceCreationProgress execute_setup_billing();
    WorkspaceCreationProgress execute_finalize_workspace_setup();
    
    // User creation methods
    WorkspaceUser execute_create_user(const HumanPersona& persona);
    std::vector<WorkspaceUser> execute_create_users(const std::vector<HumanPersona>& personas);
    
    // Helper methods
    bool setup_browser_environment();
    bool setup_proxy_connection();
    bool setup_fingerprint();
    HumanPersona generate_or_use_persona();
    std::string generate_username_from_persona(const HumanPersona& persona);
    std::string generate_secure_password();
    std::string generate_workspace_domain(const std::string& organization_name);
    
    // Domain management
    bool setup_domain_verification(const std::string& domain);
    bool add_dns_records(const std::string& domain);
    bool check_domain_status(const std::string& domain);
    
    // User management helpers
    bool create_admin_account();
    bool configure_user_permissions(const std::string& user_email, const std::string& role);
    bool send_user_invitation(const std::string& email);
    
    // Browser interaction helpers
    bool wait_for_element_and_click(const std::string& selector, int timeout_ms = 10000);
    bool wait_for_element_and_type(const std::string& selector, const std::string& text, bool clear_first = true, int timeout_ms = 10000);
    bool wait_for_element_and_select(const std::string& selector, const std::string& value, int timeout_ms = 10000);
    std::string get_element_text(const std::string& selector, int timeout_ms = 10000);
    bool wait_for_page_load_and_element(const std::string& selector);
    
    // Form filling helpers
    bool fill_business_form();
    bool fill_admin_user_form();
    bool fill_billing_form();
    bool fill_security_form();
    
    // Progress tracking
    void update_progress(WorkspaceCreationStep step, const std::string& description = "");
    void update_progress_with_error(WorkspaceCreationStep step, const std::string& error);
    void record_step_time(WorkspaceCreationStep step, std::chrono::milliseconds duration);
    
    // Debug and logging
    void capture_screenshot(WorkspaceCreationStep step);
    void log_network_request(const std::string& request);
    
    // Statistics tracking
    void update_statistics(bool success, std::chrono::milliseconds total_time);
    void record_workspace_type_result(WorkspaceType type, bool success);
    
    // Utility methods
    std::string step_to_string(WorkspaceCreationStep step) const;
    std::string workspace_type_to_string(WorkspaceType type) const;
    std::chrono::milliseconds get_step_timeout(WorkspaceCreationStep step) const;
    std::string generate_creation_id() const;
    
    // Validation methods
    bool validate_workspace_config(const WorkspaceConfig& config);
    bool validate_domain_name(const std::string& domain);
    bool validate_email_format(const std::string& email);
    
    // Logging
    void log_creator_info(const std::string& message) const;
    void log_creator_error(const std::string& message) const;
    void log_creator_debug(const std::string& message) const;
};

// Workspace user manager
class WorkspaceUserManager {
public:
    WorkspaceUserManager(std::shared_ptr<BrowserController> browser);
    ~WorkspaceUserManager() = default;
    
    // User operations
    std::future<WorkspaceUser> create_user_async(const HumanPersona& persona);
    WorkspaceUser create_user(const HumanPersona& persona);
    std::future<bool> update_user_async(const std::string& user_email, const std::map<std::string, std::string>& updates);
    bool update_user(const std::string& user_email, const std::map<std::string, std::string>& updates);
    std::future<bool> suspend_user_async(const std::string& user_email);
    bool suspend_user(const std::string& user_email);
    std::future<bool> delete_user_async(const std::string& user_email);
    bool delete_user(const std::string& user_email);
    
    // User listing and search
    std::future<std::vector<WorkspaceUser>> list_users_async();
    std::vector<WorkspaceUser> list_users();
    std::future<std::vector<WorkspaceUser>> search_users_async(const std::string& query);
    std::vector<WorkspaceUser> search_users(const std::string& query);
    
    // Bulk operations
    std::future<std::vector<WorkspaceUser>> create_users_bulk_async(const std::vector<HumanPersona>& personas);
    std::vector<WorkspaceUser> create_users_bulk(const std::vector<HumanPersona>& personas);
    std::future<bool> suspend_users_bulk_async(const std::vector<std::string>& user_emails);
    bool suspend_users_bulk(const std::vector<std::string>& user_emails);

private:
    std::shared_ptr<BrowserController> browser_;
    
    WorkspaceUser execute_create_user(const HumanPersona& persona);
    bool execute_update_user(const std::string& user_email, const std::map<std::string, std::string>& updates);
    bool execute_suspend_user(const std::string& user_email);
    bool execute_delete_user(const std::string& user_email);
    std::vector<WorkspaceUser> execute_list_users();
    std::vector<WorkspaceUser> execute_search_users(const std::string& query);
    
    // Helper methods
    bool navigate_to_admin_console();
    bool navigate_to_user_management();
    std::string generate_user_email(const HumanPersona& persona, const std::string& domain);
    std::string generate_user_password();
};

// Workspace domain manager
class WorkspaceDomainManager {
public:
    WorkspaceDomainManager(std::shared_ptr<BrowserController> browser);
    ~WorkspaceDomainManager() = default;
    
    // Domain operations
    std::future<bool> add_domain_async(const std::string& domain);
    bool add_domain(const std::string& domain);
    std::future<bool> verify_domain_async(const std::string& domain);
    bool verify_domain(const std::string& domain);
    std::future<bool> remove_domain_async(const std::string& domain);
    bool remove_domain(const std::string& domain);
    
    // Domain status
    std::future<DomainStatus> get_domain_status_async(const std::string& domain);
    DomainStatus get_domain_status(const std::string& domain);
    std::future<std::vector<std::string>> list_domains_async();
    std::vector<std::string> list_domains();
    
    // DNS management
    std::future<std::map<std::string, std::string>> get_dns_records_async(const std::string& domain);
    std::map<std::string, std::string> get_dns_records(const std::string& domain);
    std::future<bool> update_dns_records_async(const std::string& domain, const std::map<std::string, std::string>& records);
    bool update_dns_records(const std::string& domain, const std::map<std::string, std::string>& records);

private:
    std::shared_ptr<BrowserController> browser_;
    
    bool execute_add_domain(const std::string& domain);
    bool execute_verify_domain(const std::string& domain);
    bool execute_remove_domain(const std::string& domain);
    DomainStatus execute_get_domain_status(const std::string& domain);
    std::vector<std::string> execute_list_domains();
    std::map<std::string, std::string> execute_get_dns_records(const std::string& domain);
    bool execute_update_dns_records(const std::string& domain, const std::map<std::string, std::string>& records);
    
    // Helper methods
    bool navigate_to_domain_management();
    bool check_dns_propagation(const std::string& domain);
    std::vector<std::string> get_required_dns_records(const std::string& domain);
};

// Workspace creation analyzer
class WorkspaceCreationAnalyzer {
public:
    WorkspaceCreationAnalyzer();
    ~WorkspaceCreationAnalyzer() = default;
    
    // Analysis methods
    std::map<WorkspaceCreationStep, double> analyze_step_success_rates(const std::vector<WorkspaceCreationResult>& results);
    std::map<WorkspaceType, double> analyze_workspace_type_performance(const std::vector<WorkspaceCreationResult>& results);
    std::vector<std::string> identify_common_failures(const std::vector<WorkspaceCreationResult>& results);
    
    // Performance analysis
    std::chrono::milliseconds calculate_average_creation_time(const std::vector<WorkspaceCreationResult>& results);
    std::map<WorkspaceCreationStep, std::chrono::milliseconds> calculate_average_step_times(const std::vector<WorkspaceCreationResult>& results);
    std::vector<std::string> identify_performance_bottlenecks(const std::vector<WorkspaceCreationResult>& results);
    
    // Domain analysis
    std::map<std::string, double> analyze_domain_verification_success(const std::vector<WorkspaceCreationResult>& results);
    std::vector<std::string> identify_domain_issues(const std::vector<WorkspaceCreationResult>& results);
    
    // Recommendations
    std::vector<std::string> generate_optimization_recommendations(const std::vector<WorkspaceCreationResult>& results);
    std::map<std::string, std::string> suggest_config_improvements(const std::vector<WorkspaceCreationResult>& results);

private:
    std::map<WorkspaceCreationStep, std::string> step_descriptions_;
    std::map<WorkspaceType, std::string> workspace_type_descriptions_;
    
    void initialize_descriptions();
    std::vector<WorkspaceCreationResult> filter_successful_results(const std::vector<WorkspaceCreationResult>& results);
    std::vector<WorkspaceCreationResult> filter_failed_results(const std::vector<WorkspaceCreationResult>& results);
};

} // namespace gmail_infinity
