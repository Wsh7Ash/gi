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

#include <crow.h>
#include <nlohmann/json.hpp>

#include "../app/orchestrator.h"
#include "../app/metrics.h"
#include "../app/secure_vault.h"
#include "../creators/gmail_creator.h"
#include "../warming/activity_simulator.h"
#include "../verification/sms_providers.h"
#include "../verification/captcha_solver.h"

namespace gmail_infinity {

enum class HttpMethod {
    GET,
    POST,
    PUT,
    DELETE,
    PATCH,
    HEAD,
    OPTIONS
};

enum class ApiEndpoint {
    HEALTH,
    METRICS,
    ACCOUNT_CREATE,
    ACCOUNT_LIST,
    ACCOUNT_GET,
    ACCOUNT_DELETE,
    ACCOUNT_EXPORT,
    PROXY_LIST,
    PROXY_ADD,
    PROXY_DELETE,
    PROXY_HEALTH,
    SMS_PROVIDERS,
    SMS_REQUEST,
    SMS_STATUS,
    CAPTCHA_PROVIDERS,
    CAPTCHA_SOLVE,
    CAPTCHA_STATUS,
    WARMING_START,
    WARMING_STOP,
    WARMING_STATUS,
    WARMING_HISTORY,
    SYSTEM_STATUS,
    CONFIG_GET,
    CONFIG_UPDATE,
    LOGS_GET,
    AUTH_LOGIN,
    AUTH_LOGOUT,
    AUTH_REFRESH,
    WEBHOOK_CREATE,
    WEBHOOK_DELETE,
    WEBHOOK_LIST
};

struct ApiRequest {
    HttpMethod method;
    std::string endpoint;
    std::map<std::string, std::string> headers;
    std::map<std::string, std::string> query_params;
    nlohmann::json body;
    std::string client_ip;
    std::string user_agent;
    std::chrono::system_clock::time_point timestamp;
    std::string request_id;
    
    nlohmann::json to_json() const;
    void from_json(const nlohmann::json& j);
};

struct ApiResponse {
    int status_code;
    std::string status_message;
    nlohmann::json data;
    std::map<std::string, std::string> headers;
    std::chrono::milliseconds processing_time;
    std::string request_id;
    std::string error_code;
    std::string error_message;
    std::vector<std::string> warnings;
    
    nlohmann::json to_json() const;
    void from_json(const nlohmann::json& j);
    
    static ApiResponse success(const nlohmann::json& data = nlohmann::json{});
    static ApiResponse error(int status_code, const std::string& error_code, const std::string& error_message);
    static ApiResponse created(const nlohmann::json& data = nlohmann::json{});
    static ApiResponse accepted(const nlohmann::json& data = nlohmann::json{});
    static ApiResponse no_content();
};

struct RestServerConfig {
    // Server settings
    std::string host = "0.0.0.0";
    uint16_t port = 8080;
    std::string bind_address;
    int thread_pool_size = 4;
    int max_connections = 1000;
    std::chrono::seconds request_timeout{30};
    
    // Security settings
    bool enable_ssl = false;
    std::string ssl_certificate_file;
    std::string ssl_private_key_file;
    std::string ssl_ca_file;
    bool require_client_cert = false;
    
    // Authentication settings
    bool enable_authentication = true;
    std::string auth_method = "jwt"; // jwt, api_key, basic, oauth2
    std::string jwt_secret;
    std::chrono::seconds session_timeout{3600};
    std::vector<std::string> public_endpoints;
    
    // Rate limiting
    bool enable_rate_limiting = true;
    int max_requests_per_minute = 60;
    int max_requests_per_hour = 1000;
    std::map<std::string, int> endpoint_limits;
    std::chrono::seconds rate_limit_window{60};
    
    // CORS settings
    bool enable_cors = true;
    std::vector<std::string> allowed_origins;
    std::vector<std::string> allowed_methods;
    std::vector<std::string> allowed_headers;
    bool allow_credentials = true;
    
    // Request settings
    size_t max_request_size = 10 * 1024 * 1024; // 10MB
    bool enable_request_compression = true;
    bool enable_response_compression = true;
    
    // Logging settings
    bool enable_access_log = true;
    bool enable_request_log = false;
    std::string log_level = "info";
    std::string log_format = "combined";
    
    // API versioning
    std::string api_version = "v1";
    bool enable_versioning = true;
    std::map<std::string, std::string> version_deprecation;
    
    nlohmann::json to_json() const;
    void from_json(const nlohmann::json& j);
};

class RestServer {
public:
    RestServer();
    ~RestServer();
    
    // Server lifecycle
    bool initialize(const RestServerConfig& config);
    bool start();
    void stop();
    bool is_running() const;
    
    // Configuration
    void update_config(const RestServerConfig& config);
    RestServerConfig get_config() const;
    
    // Component registration
    void set_orchestrator(std::shared_ptr<Orchestrator> orchestrator);
    void set_metrics(std::shared_ptr<CreationMetrics> metrics);
    void set_secure_vault(std::shared_ptr<SecureVault> secure_vault);
    void set_gmail_creator(std::shared_ptr<GmailCreator> gmail_creator);
    void set_activity_simulator(std::shared_ptr<ActivitySimulator> activity_simulator);
    void set_sms_manager(std::shared_ptr<SMSVerificationManager> sms_manager);
    void set_captcha_solver(std::shared_ptr<CaptchaSolver> captcha_solver);
    
    // Endpoint registration
    void register_custom_endpoint(const std::string& path, HttpMethod method, std::function<ApiResponse(const ApiRequest&)> handler);
    void unregister_endpoint(const std::string& path, HttpMethod method);
    
    // Statistics
    std::map<std::string, std::chrono::milliseconds> get_endpoint_response_times() const;
    std::map<HttpMethod, size_t> get_method_distribution() const;
    std::map<int, size_t> get_status_code_distribution() const;
    std::map<std::string, size_t> get_client_request_counts() const;
    std::chrono::milliseconds get_average_response_time() const;
    std::map<std::string, std::string> get_server_statistics() const;
    
    // Request history
    std::vector<ApiRequest> get_request_history(size_t limit = 1000);
    std::vector<ApiResponse> get_response_history(size_t limit = 1000);
    bool export_request_history(const std::string& filename) const;
    
    // Health checks
    bool is_healthy() const;
    std::map<std::string, bool> get_component_health() const;
    std::map<std::string, std::string> get_detailed_health() const;

private:
    // Server components
    std::unique_ptr<crow::SimpleApp> app_;
    std::unique_ptr<std::thread> server_thread_;
    std::atomic<bool> running_{false};
    
    // Configuration
    RestServerConfig config_;
    
    // Component references
    std::shared_ptr<Orchestrator> orchestrator_;
    std::shared_ptr<CreationMetrics> metrics_;
    std::shared_ptr<SecureVault> secure_vault_;
    std::shared_ptr<GmailCreator> gmail_creator_;
    std::shared_ptr<ActivitySimulator> activity_simulator_;
    std::shared_ptr<SMSVerificationManager> sms_manager_;
    std::shared_ptr<CaptchaSolver> captcha_solver_;
    
    // Statistics
    std::atomic<size_t> total_requests_{0};
    std::atomic<size_t> successful_requests_{0};
    std::atomic<size_t> failed_requests_{0};
    std::map<std::string, std::atomic<size_t>> endpoint_counts_;
    std::map<HttpMethod, std::atomic<size_t>> method_counts_;
    std::map<int, std::atomic<size_t>> status_code_counts_;
    std::map<std::string, std::vector<std::chrono::milliseconds>> response_times_;
    mutable std::mutex stats_mutex_;
    
    // Request history
    std::vector<ApiRequest> request_history_;
    std::vector<ApiResponse> response_history_;
    mutable std::mutex history_mutex_;
    
    // Rate limiting
    std::map<std::string, std::pair<size_t, std::chrono::system_clock::time_point>> rate_limits_;
    mutable std::mutex rate_limits_mutex_;
    
    // Authentication
    std::map<std::string, std::chrono::system_clock::time_point> active_sessions_;
    std::map<std::string, std::vector<std::string>> user_permissions_;
    mutable std::mutex auth_mutex_;
    
    // Thread safety
    mutable std::mutex server_mutex_;
    
    // Private methods
    void setup_routes();
    void setup_middleware();
    void setup_error_handlers();
    
    // Health endpoints
    crow::response handle_health_check(const crow::request& req);
    crow::response handle_detailed_health(const crow::request& req);
    crow::response handle_component_health(const crow::request& req);
    
    // Metrics endpoints
    crow::response handle_get_metrics(const crow::request& req);
    crow::response handle_get_metrics_summary(const crow::request& req);
    crow::response handle_export_metrics(const crow::request& req);
    
    // Account endpoints
    crow::response handle_create_account(const crow::request& req);
    crow::response handle_list_accounts(const crow::request& req);
    crow::response handle_get_account(const crow::request& req);
    crow::response handle_delete_account(const crow::request& req);
    crow::response handle_export_accounts(const crow::request& req);
    
    // Proxy endpoints
    crow::response handle_list_proxies(const crow::request& req);
    crow::response handle_add_proxy(const crow::request& req);
    crow::response handle_delete_proxy(const crow::request& req);
    crow::response handle_proxy_health(const crow::request& req);
    
    // SMS endpoints
    crow::response handle_list_sms_providers(const crow::request& req);
    crow::response handle_request_sms(const crow::request& req);
    crow::response handle_get_sms_status(const crow::request& req);
    
    // CAPTCHA endpoints
    crow::response handle_list_captcha_providers(const crow::request& req);
    crow::response handle_solve_captcha(const crow::request& req);
    crow::response handle_get_captcha_status(const crow::request& req);
    
    // Warming endpoints
    crow::response handle_start_warming(const crow::request& req);
    crow::response handle_stop_warming(const crow::request& req);
    crow::response handle_get_warming_status(const crow::request& req);
    crow::response handle_get_warming_history(const crow::request& req);
    
    // System endpoints
    crow::response handle_get_system_status(const crow::request& req);
    crow::response handle_get_config(const crow::request& req);
    crow::response handle_update_config(const crow::request& req);
    crow::response handle_get_logs(const crow::request& req);
    
    // Authentication endpoints
    crow::response handle_login(const crow::request& req);
    crow::response handle_logout(const crow::request& req);
    crow::response handle_refresh_token(const crow::request& req);
    
    // Webhook endpoints
    crow::response handle_create_webhook(const crow::request& req);
    crow::response handle_delete_webhook(const crow::request& req);
    crow::response handle_list_webhooks(const crow::request& req);
    
    // Helper methods
    ApiResponse create_account_from_request(const crow::request& req);
    ApiResponse validate_request(const crow::request& req, const std::vector<std::string>& required_fields = {});
    ApiResponse authenticate_request(const crow::request& req);
    ApiResponse rate_limit_check(const crow::request& req);
    
    // Response helpers
    crow::response create_response(const ApiResponse& api_response);
    crow::response create_error_response(int status_code, const std::string& error_code, const std::string& error_message);
    crow::response create_success_response(const nlohmann::json& data = nlohmann::json{});
    crow::response create_created_response(const nlohmann::json& data = nlohmann::json{});
    
    // Authentication helpers
    std::string extract_token(const crow::request& req);
    bool validate_token(const std::string& token, std::string& user_id);
    std::string generate_jwt_token(const std::string& user_id);
    bool is_public_endpoint(const std::string& path);
    
    // Rate limiting helpers
    bool is_rate_limited(const crow::request& req);
    void update_rate_limit(const crow::request& req);
    void cleanup_expired_rate_limits();
    
    // Statistics helpers
    void record_request(const std::string& endpoint, HttpMethod method, int status_code, std::chrono::milliseconds response_time);
    void record_response_time(const std::string& endpoint, std::chrono::milliseconds response_time);
    
    // Utility methods
    std::string generate_request_id() const;
    std::string get_client_ip(const crow::request& req) const;
    std::chrono::milliseconds get_request_duration(const std::chrono::system_clock::time_point& start) const;
    nlohmann::json parse_request_body(const crow::request& req);
    std::map<std::string, std::string> parse_query_params(const crow::request& req);
    
    // Logging
    void log_request(const crow::request& req, const ApiResponse& response, std::chrono::milliseconds duration);
    void log_server_info(const std::string& message) const;
    void log_server_error(const std::string& message) const;
    void log_server_debug(const std::string& message) const;
};

// API middleware
class ApiMiddleware {
public:
    ApiMiddleware(std::shared_ptr<RestServerConfig> config);
    ~ApiMiddleware() = default;
    
    // CORS middleware
    crow::response handle_cors(const crow::request& req, crow::response& res);
    
    // Authentication middleware
    crow::response handle_authentication(const crow::request& req, crow::response& res);
    
    // Rate limiting middleware
    crow::response handle_rate_limiting(const crow::request& req, crow::response& res);
    
    // Request logging middleware
    crow::response handle_logging(const crow::request& req, crow::response& res);
    
    // Request validation middleware
    crow::response handle_validation(const crow::request& req, crow::response& res);

private:
    std::shared_ptr<RestServerConfig> config_;
    
    void add_cors_headers(crow::response& res);
    bool is_cors_request(const crow::request& req);
    std::string get_origin_header(const crow::request& req);
};

// API authentication manager
class ApiAuthManager {
public:
    ApiAuthManager(std::shared_ptr<RestServerConfig> config);
    ~ApiAuthManager() = default;
    
    // Authentication methods
    std::string generate_jwt_token(const std::string& user_id, const std::vector<std::string>& permissions);
    bool validate_jwt_token(const std::string& token, std::string& user_id, std::vector<std::string>& permissions);
    bool validate_api_key(const std::string& api_key, std::string& user_id);
    bool validate_basic_auth(const std::string& credentials, std::string& user_id);
    
    // Session management
    std::string create_session(const std::string& user_id);
    bool validate_session(const std::string& session_id, std::string& user_id);
    void revoke_session(const std::string& session_id);
    void cleanup_expired_sessions();
    
    // User management
    bool register_user(const std::string& user_id, const std::string& api_key, const std::vector<std::string>& permissions);
    bool unregister_user(const std::string& user_id);
    bool update_user_permissions(const std::string& user_id, const std::vector<std::string>& permissions);
    std::vector<std::string> get_user_permissions(const std::string& user_id);
    
    // API key management
    std::string generate_api_key(const std::string& user_id);
    bool revoke_api_key(const std::string& api_key);
    std::vector<std::string> get_user_api_keys(const std::string& user_id);

private:
    std::shared_ptr<RestServerConfig> config_;
    std::map<std::string, std::string> user_api_keys_;
    std::map<std::string, std::vector<std::string>> user_permissions_;
    std::map<std::string, std::chrono::system_clock::time_point> active_sessions_;
    std::map<std::string, std::chrono::system_clock::time_point> revoked_tokens_;
    mutable std::mutex auth_mutex_;
    
    std::string sign_jwt(const std::string& payload);
    bool verify_jwt_signature(const std::string& token, std::string& payload);
    std::string generate_session_id() const;
    std::string generate_api_key() const;
    std::string generate_secret_key() const;
};

// API rate limiter
class ApiRateLimiter {
public:
    ApiRateLimiter(std::shared_ptr<RestServerConfig> config);
    ~ApiRateLimiter() = default;
    
    // Rate limiting
    bool is_allowed(const std::string& client_id, const std::string& endpoint);
    bool is_allowed_global(const std::string& endpoint);
    void record_request(const std::string& client_id, const std::string& endpoint);
    
    // Configuration
    void update_limits(const std::map<std::string, int>& endpoint_limits);
    void update_global_limits(int requests_per_minute, int requests_per_hour);
    
    // Statistics
    std::map<std::string, size_t> get_client_request_counts();
    std::map<std::string, size_t> get_endpoint_request_counts();
    std::vector<std::string> get_blocked_clients();

private:
    std::shared_ptr<RestServerConfig> config_;
    std::map<std::string, std::pair<size_t, std::chrono::system_clock::time_point>> client_limits_;
    std::map<std::string, std::pair<size_t, std::chrono::system_clock::time_point>> global_limits_;
    std::map<std::string, std::vector<std::chrono::system_clock::time_point>> blocked_clients_;
    mutable std::mutex limiter_mutex_;
    
    void cleanup_expired_limits();
    bool is_client_blocked(const std::string& client_id);
    void block_client(const std::string& client_id, std::chrono::seconds duration);
};

// API documentation generator
class ApiDocumentationGenerator {
public:
    ApiDocumentationGenerator(std::shared_ptr<RestServer> server);
    ~ApiDocumentationGenerator() = default;
    
    // Documentation generation
    std::string generate_openapi_spec();
    std::string generate_swagger_ui_html();
    std::string generate_postman_collection();
    std::string generate_api_markdown();
    
    // Endpoint documentation
    std::string generate_endpoint_docs(ApiEndpoint endpoint);
    std::string generate_example_requests(ApiEndpoint endpoint);
    std::string generate_example_responses(ApiEndpoint endpoint);

private:
    std::shared_ptr<RestServer> server_;
    
    nlohmann::json create_openapi_info();
    nlohmann::json create_openapi_paths();
    nlohmann::json create_endpoint_schema(ApiEndpoint endpoint);
    std::string format_json_response(const nlohmann::json& json);
};

} // namespace gmail_infinity
