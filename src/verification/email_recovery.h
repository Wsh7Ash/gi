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

enum class EmailProvider {
    MAIL_TM,
    TEMPMAIL_ORG,
    GUERRILLA_MAIL,
    10MINUTEMAIL,
    TEMPMAIL_IO,
    MAILPOOF,
    EMAIL_FAKE,
    THROWAWAY_EMAIL,
    MINTEMAIL,
    TEMPMAIL_DE
};

enum class EmailStatus {
    ACTIVE,
    INACTIVE,
    EXPIRED,
    BOUNCED,
    BLOCKED,
    ERROR
};

struct EmailAccount {
    std::string id;
    std::string email_address;
    std::string password;
    EmailProvider provider;
    EmailStatus status;
    std::chrono::system_clock::time_point created_at;
    std::chrono::system_clock::time_point expires_at;
    std::chrono::system_clock::time_point last_checked;
    std::vector<std::string> messages;
    std::map<std::string, std::string> metadata;
    
    nlohmann::json to_json() const;
    void from_json(const nlohmann::json& j);
    bool is_expired() const;
    int get_remaining_minutes() const;
};

struct EmailMessage {
    std::string id;
    std::string from;
    std::string to;
    std::string subject;
    std::string body;
    std::string body_html;
    std::chrono::system_clock::time_point received_at;
    std::chrono::system_clock::time_point read_at;
    bool is_read;
    bool has_attachments;
    std::vector<std::string> attachments;
    std::map<std::string, std::string> headers;
    
    nlohmann::json to_json() const;
    void from_json(const nlohmann::json& j);
    std::string get_verification_code() const;
    std::string get_reset_link() const;
};

struct EmailProviderConfig {
    std::string api_url;
    std::string api_key;
    int timeout_seconds = 30;
    int max_retries = 3;
    std::map<std::string, std::string> headers;
    std::map<std::string, std::string> parameters;
    std::chrono::minutes default_expiry{10};
    
    nlohmann::json to_json() const;
    void from_json(const nlohmann::json& j);
};

class EmailProviderInterface {
public:
    virtual ~EmailProviderInterface() = default;
    
    // Core functionality
    virtual bool initialize(const EmailProviderConfig& config) = 0;
    virtual std::shared_ptr<EmailAccount> create_account() = 0;
    virtual std::shared_ptr<EmailAccount> get_account(const std::string& email_address) = 0;
    virtual bool delete_account(const std::string& email_address) = 0;
    
    // Message operations
    virtual std::vector<EmailMessage> get_messages(const std::string& email_address) = 0;
    virtual std::shared_ptr<EmailMessage> get_message(const std::string& email_address, const std::string& message_id) = 0;
    virtual bool mark_message_read(const std::string& email_address, const std::string& message_id) = 0;
    virtual bool delete_message(const std::string& email_address, const std::string& message_id) = 0;
    
    // Provider information
    virtual std::string get_provider_name() const = 0;
    virtual std::chrono::minutes get_max_expiry_time() const = 0;
    virtual std::map<std::string, std::string> get_provider_info() const = 0;
    
    // Health and connectivity
    virtual bool test_connection() = 0;
    virtual std::map<std::string, std::string> get_statistics() const = 0;

protected:
    EmailProviderConfig config_;
    mutable std::mutex provider_mutex_;
    
    // HTTP helpers
    std::string make_http_request(const std::string& url, const std::string& method = "GET",
                                 const std::map<std::string, std::string>& params = {},
                                 const std::map<std::string, std::string>& headers = {});
    std::string make_post_request(const std::string& url, const std::string& data,
                                 const std::map<std::string, std::string>& headers = {});
    nlohmann::json parse_json_response(const std::string& response);
    
    // Utility methods
    std::string generate_random_email() const;
    std::string extract_domain(const std::string& email) const;
    std::chrono::system_clock::time_point parse_timestamp(const std::string& timestamp) const;
};

// mail.tm provider implementation
class MailTmProvider : public EmailProviderInterface {
public:
    bool initialize(const EmailProviderConfig& config) override;
    std::shared_ptr<EmailAccount> create_account() override;
    std::shared_ptr<EmailAccount> get_account(const std::string& email_address) override;
    bool delete_account(const std::string& email_address) override;
    
    std::vector<EmailMessage> get_messages(const std::string& email_address) override;
    std::shared_ptr<EmailMessage> get_message(const std::string& email_address, const std::string& message_id) override;
    bool mark_message_read(const std::string& email_address, const std::string& message_id) override;
    bool delete_message(const std::string& email_address, const std::string& message_id) override;
    
    std::string get_provider_name() const override { return "mail.tm"; }
    std::chrono::minutes get_max_expiry_time() const override { return std::chrono::minutes(60); }
    std::map<std::string, std::string> get_provider_info() const override;
    
    bool test_connection() override;
    std::map<std::string, std::string> get_statistics() const override;

private:
    std::string build_api_url(const std::string& endpoint) const;
    std::map<std::string, std::string> get_default_headers() const;
    std::string get_jwt_token() const;
    bool authenticate();
    std::string jwt_token_;
    std::chrono::system_clock::time_point token_expiry_;
};

// Guerrilla Mail provider implementation
class GuerrillaMailProvider : public EmailProviderInterface {
public:
    bool initialize(const EmailProviderConfig& config) override;
    std::shared_ptr<EmailAccount> create_account() override;
    std::shared_ptr<EmailAccount> get_account(const std::string& email_address) override;
    bool delete_account(const std::string& email_address) override;
    
    std::vector<EmailMessage> get_messages(const std::string& email_address) override;
    std::shared_ptr<EmailMessage> get_message(const std::string& email_address, const std::string& message_id) override;
    bool mark_message_read(const std::string& email_address, const std::string& message_id) override;
    bool delete_message(const std::string& email_address, const std::string& message_id) override;
    
    std::string get_provider_name() const override { return "Guerrilla Mail"; }
    std::chrono::minutes get_max_expiry_time() const override { return std::chrono::minutes(15); }
    std::map<std::string, std::string> get_provider_info() const override;
    
    bool test_connection() override;
    std::map<std::string, std::string> get_statistics() const override;

private:
    std::string build_api_url(const std::string& endpoint) const;
    std::string get_sid_token(const std::string& email_address) const;
    std::map<std::string, std::string> sid_tokens_;
    mutable std::mutex sid_mutex_;
};

// 10MinuteMail provider implementation
class TenMinuteMailProvider : public EmailProviderInterface {
public:
    bool initialize(const EmailProviderConfig& config) override;
    std::shared_ptr<EmailAccount> create_account() override;
    std::shared_ptr<EmailAccount> get_account(const std::string& email_address) override;
    bool delete_account(const std::string& email_address) override;
    
    std::vector<EmailMessage> get_messages(const std::string& email_address) override;
    std::shared_ptr<EmailMessage> get_message(const std::string& email_address, const std::string& message_id) override;
    bool mark_message_read(const std::string& email_address, const std::string& message_id) override;
    bool delete_message(const std::string& email_address, const std::string& message_id) override;
    
    std::string get_provider_name() const override { return "10MinuteMail"; }
    std::chrono::minutes get_max_expiry_time() const override { return std::chrono::minutes(10); }
    std::map<std::string, std::string> get_provider_info() const override;
    
    bool test_connection() override;
    std::map<std::string, std::string> get_statistics() const override;

private:
    std::string build_api_url(const std::string& endpoint) const;
    std::string extract_session_key(const std::string& response) const;
    std::map<std::string, std::string> session_keys_;
    mutable std::mutex session_mutex_;
};

class EmailRecoveryManager {
public:
    EmailRecoveryManager();
    ~EmailRecoveryManager();
    
    // Initialization
    bool initialize(const std::string& config_file = "config/email_recovery.yaml");
    void shutdown();
    
    // Provider management
    bool register_provider(std::unique_ptr<EmailProviderInterface> provider);
    bool set_primary_provider(EmailProvider provider);
    std::vector<EmailProvider> get_available_providers();
    
    // Account operations
    std::shared_ptr<EmailAccount> create_temporary_email();
    std::shared_ptr<EmailAccount> get_email_account(const std::string& email_address);
    bool delete_email_account(const std::string& email_address);
    
    // Message operations
    std::vector<EmailMessage> get_messages(const std::string& email_address);
    std::shared_ptr<EmailMessage> get_latest_message(const std::string& email_address);
    std::shared_ptr<EmailMessage> wait_for_message(const std::string& email_address,
                                                  std::chrono::seconds timeout = std::chrono::seconds(300));
    
    // Recovery-specific operations
    std::string wait_for_verification_code(const std::string& email_address,
                                         std::chrono::seconds timeout = std::chrono::seconds(300));
    std::string wait_for_reset_link(const std::string& email_address,
                                   std::chrono::seconds timeout = std::chrono::seconds(300));
    std::vector<std::string> extract_all_links(const std::string& email_address);
    
    // Batch operations
    std::vector<std::shared_ptr<EmailAccount>> create_multiple_emails(size_t count);
    std::map<std::string, std::vector<EmailMessage>> get_all_messages(
        const std::vector<std::string>& email_addresses);
    
    // Provider rotation
    void enable_provider_rotation(bool enable);
    void set_rotation_strategy(const std::string& strategy);
    EmailProvider get_next_provider();
    
    // Monitoring and statistics
    std::map<EmailProvider, size_t> get_provider_usage_stats();
    std::map<EmailStatus, size_t> get_status_distribution();
    size_t get_total_accounts() const;
    size_t get_active_accounts() const;
    std::vector<std::shared_ptr<EmailAccount>> get_expiring_accounts(std::chrono::minutes threshold = std::chrono::minutes(5));
    
    // Configuration
    void set_default_expiry(std::chrono::minutes expiry);
    void set_check_interval(std::chrono::seconds interval);
    void set_max_accounts_per_provider(size_t max_accounts);
    
    // Account management
    void start_account_monitoring();
    void stop_account_monitoring();
    void cleanup_expired_accounts();
    std::vector<std::shared_ptr<EmailAccount>> get_account_history(size_t limit = 100);
    
    // History and logging
    bool export_account_history(const std::string& filename) const;
    bool import_account_history(const std::string& filename);
    
    // Health checking
    void start_health_checking();
    void stop_health_checking();
    std::map<EmailProvider, bool> get_provider_health_status();

private:
    // Provider management
    std::map<EmailProvider, std::unique_ptr<EmailProviderInterface>> providers_;
    EmailProvider primary_provider_{EmailProvider::MAIL_TM};
    std::vector<EmailProvider> provider_rotation_order_;
    std::map<EmailProvider, bool> provider_health_;
    std::map<EmailProvider, std::chrono::system_clock::time_point> last_provider_check_;
    
    // Configuration
    std::chrono::minutes default_expiry_{30};
    std::chrono::seconds check_interval_{30};
    size_t max_accounts_per_provider_{100};
    
    // Statistics
    std::map<EmailProvider, size_t> provider_usage_;
    std::map<EmailStatus, size_t> status_counts_;
    std::atomic<size_t> total_accounts_{0};
    std::atomic<size_t> active_accounts_{0};
    
    // Account storage
    std::map<std::string, std::shared_ptr<EmailAccount>> email_accounts_;
    mutable std::mutex accounts_mutex_;
    
    // Account history
    std::vector<std::shared_ptr<EmailAccount>> account_history_;
    mutable std::mutex history_mutex_;
    
    // Monitoring
    std::unique_ptr<std::thread> monitoring_thread_;
    std::atomic<bool> monitoring_running_{false};
    
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
    EmailProviderInterface* get_current_provider();
    EmailProviderInterface* get_provider_by_enum(EmailProvider provider);
    
    // Account management
    std::shared_ptr<EmailAccount> create_account_with_provider(EmailProviderInterface* provider);
    void update_account_status(const std::string& email_address, EmailStatus status);
    void remove_expired_account(const std::string& email_address);
    
    // Message processing
    std::string extract_verification_code_from_message(const EmailMessage& message);
    std::string extract_reset_link_from_message(const EmailMessage& message);
    std::vector<std::string> extract_links_from_text(const std::string& text);
    
    // Provider rotation
    void build_rotation_order();
    EmailProvider select_next_provider();
    bool is_provider_healthy(EmailProvider provider);
    
    // Monitoring
    void monitoring_loop();
    void check_account_expiry();
    void update_account_statistics();
    
    // Health checking
    void health_check_loop();
    void check_provider_health(EmailProvider provider);
    
    // Utility methods
    std::string generate_account_id() const;
    void log_email_info(const std::string& message) const;
    void log_email_error(const std::string& message) const;
    void log_email_debug(const std::string& message) const;
};

// Email recovery result
struct EmailRecoveryResult {
    bool success;
    std::string email_address;
    std::string verification_code;
    std::string reset_link;
    EmailProvider provider;
    std::chrono::milliseconds response_time;
    std::string error_message;
    
    nlohmann::json to_json() const;
    void from_json(const nlohmann::json& j);
};

// Email recovery orchestrator
class EmailRecoveryOrchestrator {
public:
    EmailRecoveryOrchestrator(std::shared_ptr<EmailRecoveryManager> manager);
    ~EmailRecoveryOrchestrator() = default;
    
    // High-level recovery workflows
    EmailRecoveryResult setup_recovery_email();
    EmailRecoveryResult wait_for_gmail_verification(std::chrono::seconds timeout = std::chrono::seconds(300));
    EmailRecoveryResult wait_for_password_reset(std::chrono::seconds timeout = std::chrono::seconds(300));
    
    // Async operations
    std::future<EmailRecoveryResult> setup_recovery_email_async();
    std::future<EmailRecoveryResult> wait_for_verification_async(const std::string& email_address,
                                                                std::chrono::seconds timeout = std::chrono::seconds(300));
    
    // Batch operations
    std::vector<EmailRecoveryResult> setup_multiple_recovery_emails(size_t count);

private:
    std::shared_ptr<EmailRecoveryManager> manager_;
    
    EmailRecoveryResult execute_recovery_workflow();
    bool wait_for_verification_code(const std::string& email_address,
                                   std::chrono::seconds timeout,
                                   std::string& code);
    bool wait_for_reset_link(const std::string& email_address,
                            std::chrono::seconds timeout,
                            std::string& link);
};

// Email content analyzer
class EmailContentAnalyzer {
public:
    EmailContentAnalyzer();
    ~EmailContentAnalyzer() = default;
    
    // Content analysis
    std::string extract_verification_code(const EmailMessage& message);
    std::string extract_reset_link(const EmailMessage& message);
    std::string extract_confirmation_link(const EmailMessage& message);
    std::vector<std::string> extract_all_links(const EmailMessage& message);
    
    // Message classification
    std::string classify_message(const EmailMessage& message);
    bool is_verification_email(const EmailMessage& message);
    bool is_password_reset_email(const EmailMessage& message);
    bool is_account_confirmation_email(const EmailMessage& message);
    
    // Content filtering
    std::string clean_text_content(const std::string& content);
    std::string extract_main_content(const EmailMessage& message);
    std::vector<std::string> extract_keywords(const EmailMessage& message);

private:
    std::vector<std::regex> verification_code_patterns_;
    std::vector<std::regex> reset_link_patterns_;
    std::vector<std::regex> confirmation_link_patterns_;
    
    void initialize_patterns();
    std::string extract_with_regex(const std::string& content, const std::vector<std::regex>& patterns);
    std::vector<std::string> extract_links_with_regex(const std::string& content, const std::regex& pattern);
};

} // namespace gmail_infinity
