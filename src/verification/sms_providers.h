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

enum class SMSProvider {
    FIVE_SIM,
    SMS_ACTIVATE,
    TEXT_VERIFIED,
    ONLINESIM,
    SMS_POOL,
    TEMP_SMS,
    RECEIVE_SMS,
    SMS_API
};

enum class SMSStatus {
    PENDING,
    RECEIVED,
    EXPIRED,
    CANCELLED,
    ERROR,
    BLACKLISTED
};

enum class SMSCountry {
    USA,
    UK,
    CANADA,
    AUSTRALIA,
    GERMANY,
    FRANCE,
    SPAIN,
    ITALY,
    NETHERLANDS,
    POLAND,
    RUSSIA,
    CHINA,
    INDIA,
    JAPAN,
    BRAZIL,
    MEXICO,
    ANY
};

struct SMSNumber {
    std::string id;
    std::string phone_number;
    SMSCountry country;
    SMSProvider provider;
    std::string service; // gmail, whatsapp, telegram, etc.
    SMSStatus status;
    std::chrono::system_clock::time_point created_at;
    std::chrono::system_clock::time_point expires_at;
    std::string activation_code;
    std::string forwarded_number;
    double cost;
    std::map<std::string, std::string> metadata;
    
    nlohmann::json to_json() const;
    void from_json(const nlohmann::json& j);
    bool is_expired() const;
    int get_remaining_seconds() const;
};

struct SMSProviderConfig {
    std::string api_key;
    std::string api_url;
    std::string api_secret;
    int timeout_seconds = 30;
    int max_retries = 3;
    std::map<std::string, std::string> headers;
    std::map<std::string, std::string> parameters;
    
    nlohmann::json to_json() const;
    void from_json(const nlohmann::json& j);
};

class SMSProviderInterface {
public:
    virtual ~SMSProviderInterface() = default;
    
    // Core functionality
    virtual bool initialize(const SMSProviderConfig& config) = 0;
    virtual std::shared_ptr<SMSNumber> request_number(SMSCountry country, const std::string& service) = 0;
    virtual SMSStatus check_status(const std::string& number_id) = 0;
    virtual std::string get_sms_code(const std::string& number_id) = 0;
    virtual bool cancel_number(const std::string& number_id) = 0;
    
    // Provider information
    virtual std::string get_provider_name() const = 0;
    virtual std::vector<SMSCountry> get_supported_countries() const = 0;
    virtual std::vector<std::string> get_supported_services() const = 0;
    virtual double get_cost_per_sms(SMSCountry country, const std::string& service) const = 0;
    
    // Health and connectivity
    virtual bool test_connection() = 0;
    virtual double get_balance() const = 0;
    virtual std::map<std::string, std::string> get_provider_info() const = 0;

protected:
    SMSProviderConfig config_;
    mutable std::mutex provider_mutex_;
    
    // HTTP helpers
    std::string make_http_request(const std::string& url, const std::string& method = "GET",
                                 const std::map<std::string, std::string>& params = {},
                                 const std::map<std::string, std::string>& headers = {});
    std::string make_post_request(const std::string& url, const std::string& data,
                                 const std::map<std::string, std::string>& headers = {});
    nlohmann::json parse_json_response(const std::string& response);
    
    // Utility methods
    std::string country_to_string(SMSCountry country) const;
    SMSCountry string_to_country(const std::string& country_str) const;
    std::string status_to_string(SMSStatus status) const;
    SMSStatus string_to_status(const std::string& status_str) const;
};

// 5Sim provider implementation
class FiveSimProvider : public SMSProviderInterface {
public:
    bool initialize(const SMSProviderConfig& config) override;
    std::shared_ptr<SMSNumber> request_number(SMSCountry country, const std::string& service) override;
    SMSStatus check_status(const std::string& number_id) override;
    std::string get_sms_code(const std::string& number_id) override;
    bool cancel_number(const std::string& number_id) override;
    
    std::string get_provider_name() const override { return "5Sim"; }
    std::vector<SMSCountry> get_supported_countries() const override;
    std::vector<std::string> get_supported_services() const override;
    double get_cost_per_sms(SMSCountry country, const std::string& service) const override;
    
    bool test_connection() override;
    double get_balance() const override;
    std::map<std::string, std::string> get_provider_info() const override;

private:
    std::string build_api_url(const std::string& endpoint) const;
    std::map<std::string, std::string> get_default_headers() const;
};

// SMS-Activate provider implementation
class SMSActivateProvider : public SMSProviderInterface {
public:
    bool initialize(const SMSProviderConfig& config) override;
    std::shared_ptr<SMSNumber> request_number(SMSCountry country, const std::string& service) override;
    SMSStatus check_status(const std::string& number_id) override;
    std::string get_sms_code(const std::string& number_id) override;
    bool cancel_number(const std::string& number_id) override;
    
    std::string get_provider_name() const override { return "SMS-Activate"; }
    std::vector<SMSCountry> get_supported_countries() const override;
    std::vector<std::string> get_supported_services() const override;
    double get_cost_per_sms(SMSCountry country, const std::string& service) const override;
    
    bool test_connection() override;
    double get_balance() const override;
    std::map<std::string, std::string> get_provider_info() const override;

private:
    std::string build_api_url(const std::string& endpoint) const;
    int get_country_code(SMSCountry country) const;
};

// TextVerified provider implementation
class TextVerifiedProvider : public SMSProviderInterface {
public:
    bool initialize(const SMSProviderConfig& config) override;
    std::shared_ptr<SMSNumber> request_number(SMSCountry country, const std::string& service) override;
    SMSStatus check_status(const std::string& number_id) override;
    std::string get_sms_code(const std::string& number_id) override;
    bool cancel_number(const std::string& number_id) override;
    
    std::string get_provider_name() const override { return "TextVerified"; }
    std::vector<SMSCountry> get_supported_countries() const override;
    std::vector<std::string> get_supported_services() const override;
    double get_cost_per_sms(SMSCountry country, const std::string& service) const override;
    
    bool test_connection() override;
    double get_balance() const override;
    std::map<std::string, std::string> get_provider_info() const override;

private:
    std::string build_api_url(const std::string& endpoint) const;
    std::string get_service_id(const std::string& service) const;
};

// OnlineSim provider implementation
class OnlineSimProvider : public SMSProviderInterface {
public:
    bool initialize(const SMSProviderConfig& config) override;
    std::shared_ptr<SMSNumber> request_number(SMSCountry country, const std::string& service) override;
    SMSStatus check_status(const std::string& number_id) override;
    std::string get_sms_code(const std::string& number_id) override;
    bool cancel_number(const std::string& number_id) override;
    
    std::string get_provider_name() const override { return "OnlineSim"; }
    std::vector<SMSCountry> get_supported_countries() const override;
    std::vector<std::string> get_supported_services() const override;
    double get_cost_per_sms(SMSCountry country, const std::string& service) const override;
    
    bool test_connection() override;
    double get_balance() const override;
    std::map<std::string, std::string> get_provider_info() const override;

private:
    std::string build_api_url(const std::string& endpoint) const;
    int get_country_code(SMSCountry country) const;
    int get_service_code(const std::string& service) const;
};

class SMSVerificationManager {
public:
    SMSVerificationManager();
    ~SMSVerificationManager();
    
    // Initialization
    bool initialize(const std::string& config_file = "config/sms_providers.yaml");
    void shutdown();
    
    // Provider management
    bool register_provider(std::unique_ptr<SMSProviderInterface> provider);
    bool set_primary_provider(SMSProvider provider);
    std::vector<SMSProvider> get_available_providers();
    
    // SMS operations
    std::shared_ptr<SMSNumber> request_sms_number(SMSCountry country = SMSCountry::ANY, 
                                                   const std::string& service = "gmail");
    SMSStatus check_sms_status(const std::string& number_id);
    std::string get_verification_code(const std::string& number_id);
    bool cancel_sms_request(const std::string& number_id);
    
    // Advanced operations
    std::shared_ptr<SMSNumber> request_with_fallback(SMSCountry country, const std::string& service);
    std::vector<std::shared_ptr<SMSNumber>> request_multiple_numbers(size_t count, 
                                                                    SMSCountry country = SMSCountry::ANY,
                                                                    const std::string& service = "gmail");
    
    // Batch operations
    std::map<std::string, SMSStatus> check_multiple_status(const std::vector<std::string>& number_ids);
    std::map<std::string, std::string> get_multiple_codes(const std::vector<std::string>& number_ids);
    
    // Provider rotation and load balancing
    void enable_provider_rotation(bool enable);
    void set_rotation_strategy(const std::string& strategy);
    SMSProvider get_next_provider();
    
    // Monitoring and statistics
    std::map<SMSProvider, double> get_provider_balances();
    std::map<SMSProvider, size_t> get_provider_usage_stats();
    std::map<SMSStatus, size_t> get_status_distribution();
    double get_total_cost() const;
    double get_success_rate() const;
    
    // Configuration
    void set_default_country(SMSCountry country);
    void set_default_service(const std::string& service);
    void set_timeout_seconds(int seconds);
    void set_max_retries(int retries);
    void set_retry_delay(std::chrono::seconds delay);
    
    // Queue management
    void enqueue_request(SMSCountry country, const std::string& service);
    std::shared_ptr<SMSNumber> dequeue_request();
    size_t get_queue_size() const;
    void clear_queue();
    
    // History and logging
    std::vector<std::shared_ptr<SMSNumber>> get_recent_requests(size_t limit = 100);
    bool export_history(const std::string& filename) const;
    bool import_history(const std::string& filename);
    
    // Health checking
    void start_health_checking();
    void stop_health_checking();
    std::map<SMSProvider, bool> get_provider_health_status();
    
    // Cost management
    void set_cost_limit(double daily_limit);
    double get_daily_cost() const;
    bool is_within_cost_limit() const;
    std::vector<std::shared_ptr<SMSNumber>> get_cost_breakdown() const;

private:
    // Provider management
    std::map<SMSProvider, std::unique_ptr<SMSProviderInterface>> providers_;
    SMSProvider primary_provider_{SMSProvider::FIVE_SIM};
    std::vector<SMSProvider> provider_rotation_order_;
    std::map<SMSProvider, bool> provider_health_;
    std::map<SMSProvider, std::chrono::system_clock::time_point> last_provider_check_;
    
    // Configuration
    SMSCountry default_country_{SMSCountry::ANY};
    std::string default_service_{"gmail"};
    int timeout_seconds_{60};
    int max_retries_{3};
    std::chrono::seconds retry_delay_{10};
    double cost_limit_{100.0}; // Daily cost limit
    
    // Statistics
    std::map<SMSProvider, size_t> provider_usage_;
    std::map<SMSStatus, size_t> status_counts_;
    std::atomic<double> total_cost_{0.0};
    std::atomic<size_t> total_requests_{0};
    std::atomic<size_t> successful_requests_{0};
    
    // Request queue
    std::queue<std::pair<SMSCountry, std::string>> request_queue_;
    mutable std::mutex queue_mutex_;
    std::condition_variable queue_cv_;
    
    // Request history
    std::vector<std::shared_ptr<SMSNumber>> request_history_;
    mutable std::mutex history_mutex_;
    
    // Active requests
    std::map<std::string, std::shared_ptr<SMSNumber>> active_requests_;
    mutable std::mutex active_mutex_;
    
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
    SMSProviderInterface* get_current_provider();
    SMSProviderInterface* get_provider_by_enum(SMSProvider provider);
    
    // Request management
    std::shared_ptr<SMSNumber> execute_request(SMSCountry country, const std::string& service, 
                                             SMSProviderInterface* provider);
    bool retry_request(std::shared_ptr<SMSNumber> sms_number, SMSProviderInterface* provider);
    void update_statistics(SMSStatus status, double cost);
    
    // Provider rotation
    void build_rotation_order();
    SMSProvider select_next_provider();
    bool is_provider_healthy(SMSProvider provider);
    
    // Health checking
    void health_check_loop();
    void check_provider_health(SMSProvider provider);
    
    // Queue processing
    void process_queue();
    std::thread queue_processor_;
    std::atomic<bool> queue_processing_running_{false};
    
    // Cost tracking
    void track_cost(double cost, SMSProvider provider);
    void reset_daily_cost();
    std::chrono::system_clock::time_point last_cost_reset_;
    
    // Utility methods
    std::string generate_request_id() const;
    void log_sms_info(const std::string& message) const;
    void log_sms_error(const std::string& message) const;
    void log_sms_debug(const std::string& message) const;
};

// SMS verification result
struct SMSVerificationResult {
    bool success;
    std::string code;
    std::string phone_number;
    SMSProvider provider;
    std::chrono::milliseconds response_time;
    double cost;
    std::string error_message;
    
    nlohmann::json to_json() const;
    void from_json(const nlohmann::json& j);
};

// SMS verification orchestrator
class SMSVerificationOrchestrator {
public:
    SMSVerificationOrchestrator(std::shared_ptr<SMSVerificationManager> manager);
    ~SMSVerificationOrchestrator() = default;
    
    // High-level verification workflow
    SMSVerificationResult verify_phone_number(SMSCountry country = SMSCountry::ANY,
                                              const std::string& service = "gmail",
                                              std::chrono::seconds timeout = std::chrono::seconds(300));
    
    // Async verification
    std::future<SMSVerificationResult> verify_phone_number_async(SMSCountry country = SMSCountry::ANY,
                                                                const std::string& service = "gmail",
                                                                std::chrono::seconds timeout = std::chrono::seconds(300));
    
    // Batch verification
    std::vector<SMSVerificationResult> verify_multiple_numbers(size_t count,
                                                              SMSCountry country = SMSCountry::ANY,
                                                              const std::string& service = "gmail");
    
    // Verification with retry logic
    SMSVerificationResult verify_with_retry(SMSCountry country,
                                          const std::string& service,
                                          int max_retries = 3,
                                          std::chrono::seconds retry_delay = std::chrono::seconds(30));

private:
    std::shared_ptr<SMSVerificationManager> manager_;
    
    SMSVerificationResult execute_verification_workflow(SMSCountry country,
                                                       const std::string& service,
                                                       std::chrono::seconds timeout);
    bool wait_for_code(std::shared_ptr<SMSNumber> sms_number,
                      std::chrono::seconds timeout,
                      std::string& code);
};

} // namespace gmail_infinity
