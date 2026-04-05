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

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/ssl.hpp>
#include <nlohmann/json.hpp>

#include "../app/orchestrator.h"
#include "../app/metrics.h"
#include "../creators/gmail_creator.h"
#include "../warming/activity_simulator.h"

namespace gmail_infinity {

enum class WebSocketMessageType {
    CONNECTION,
    DISCONNECTION,
    HEARTBEAT,
    ERROR,
    STATUS_UPDATE,
    METRICS_UPDATE,
    CREATION_STARTED,
    CREATION_PROGRESS,
    CREATION_COMPLETED,
    CREATION_FAILED,
    WARMING_STARTED,
    WARMING_PROGRESS,
    WARMING_COMPLETED,
    WARMING_FAILED,
    PROXY_UPDATE,
    ACCOUNT_CREATED,
    ACCOUNT_DELETED,
    SYSTEM_STATUS,
    CUSTOM_MESSAGE
};

enum class WebSocketConnectionState {
    CONNECTING,
    CONNECTED,
    AUTHENTICATED,
    DISCONNECTED,
    ERROR
};

struct WebSocketMessage {
    WebSocketMessageType type;
    std::string message_id;
    std::chrono::system_clock::time_point timestamp;
    std::string sender_id;
    std::string recipient_id; // Empty for broadcast
    nlohmann::json data;
    bool requires_ack;
    std::string correlation_id;
    
    nlohmann::json to_json() const;
    void from_json(const nlohmann::json& j);
};

struct WebSocketConnection {
    std::string connection_id;
    std::string client_id;
    WebSocketConnectionState state;
    std::chrono::system_clock::time_point connected_at;
    std::chrono::system_clock::time_point last_activity;
    std::string ip_address;
    std::string user_agent;
    std::map<std::string, std::string> headers;
    bool authenticated;
    std::vector<std::string> subscriptions;
    std::map<std::string, std::string> metadata;
    
    nlohmann::json to_json() const;
    void from_json(const nlohmann::json& j);
    bool is_active() const;
    std::chrono::seconds get_idle_time() const;
};

struct WebSocketServerConfig {
    // Server settings
    std::string host = "0.0.0.0";
    uint16_t port = 8081;
    std::string bind_address;
    int max_connections = 1000;
    int thread_pool_size = 4;
    
    // Security settings
    bool enable_ssl = false;
    std::string ssl_certificate_file;
    std::string ssl_private_key_file;
    std::string ssl_ca_file;
    bool require_client_cert = false;
    
    // Authentication settings
    bool enable_authentication = true;
    std::string auth_method = "jwt"; // jwt, api_key, basic
    std::string jwt_secret;
    std::chrono::seconds session_timeout{3600};
    std::chrono::seconds heartbeat_interval{30};
    
    // Rate limiting
    bool enable_rate_limiting = true;
    int max_messages_per_minute = 60;
    int max_connections_per_ip = 10;
    std::chrono::seconds rate_limit_window{60};
    
    // Message settings
    size_t max_message_size = 1024 * 1024; // 1MB
    bool enable_message_compression = true;
    bool enable_message_encryption = false;
    std::string encryption_key;
    
    // Logging
    bool enable_access_log = true;
    bool enable_message_log = false;
    std::string log_level = "info";
    
    nlohmann::json to_json() const;
    void from_json(const nlohmann::json& j);
};

class WebSocketSession {
public:
    WebSocketSession(boost::asio::ip::tcp::socket socket, std::string connection_id, std::shared_ptr<WebSocketServerConfig> config);
    ~WebSocketSession();
    
    // Session management
    void start();
    void stop();
    bool is_active() const;
    std::string get_connection_id() const;
    std::string get_client_id() const;
    WebSocketConnectionState get_state() const;
    
    // Message handling
    void send_message(const WebSocketMessage& message);
    void send_message_async(const WebSocketMessage& message);
    void broadcast_message(const WebSocketMessage& message);
    void subscribe_to_events(const std::vector<std::string>& events);
    void unsubscribe_from_events(const std::vector<std::string>& events);
    
    // Authentication
    bool authenticate(const std::string& token);
    bool is_authenticated() const;
    std::string get_user_id() const;
    
    // Statistics
    std::chrono::system_clock::time_point get_connected_at() const;
    std::chrono::system_clock::time_point get_last_activity() const;
    size_t get_messages_sent() const;
    size_t get_messages_received() const;
    std::chrono::milliseconds get_average_message_latency() const;

private:
    // WebSocket components
    boost::asio::ip::tcp::socket socket_;
    std::unique_ptr<boost::beast::websocket::stream<boost::beast::tcp_stream>> ws_;
    std::unique_ptr<boost::beast::websocket::stream<boost::beast::ssl::stream<boost::beast::tcp_stream>>> wss_;
    
    // Session state
    std::string connection_id_;
    std::string client_id_;
    WebSocketConnectionState state_;
    std::atomic<bool> running_{false};
    std::atomic<bool> authenticated_{false};
    
    // Configuration
    std::shared_ptr<WebSocketServerConfig> config_;
    
    // Statistics
    std::atomic<size_t> messages_sent_{0};
    std::atomic<size_t> messages_received_{0};
    std::vector<std::chrono::milliseconds> message_latencies_;
    mutable std::mutex stats_mutex_;
    
    // Timing
    std::chrono::system_clock::time_point connected_at_;
    std::chrono::system_clock::time_point last_activity_;
    std::chrono::steady_clock::time_point last_heartbeat_;
    
    // Subscriptions
    std::vector<std::string> subscriptions_;
    mutable std::mutex subscriptions_mutex_;
    
    // Message queue
    std::queue<WebSocketMessage> message_queue_;
    std::mutex queue_mutex_;
    std::condition_variable queue_cv_;
    
    // Thread safety
    mutable std::mutex session_mutex_;
    
    // Private methods
    void do_read();
    void do_write(const WebSocketMessage& message);
    void process_message_queue();
    void handle_message(const std::string& message_str);
    void handle_heartbeat();
    void send_heartbeat();
    
    // Message processing
    WebSocketMessage parse_message(const std::string& message_str);
    void handle_connection_message(const WebSocketMessage& message);
    void handle_authentication_message(const WebSocketMessage& message);
    void handle_subscription_message(const WebSocketMessage& message);
    void handle_custom_message(const WebSocketMessage& message);
    
    // Authentication
    bool verify_jwt_token(const std::string& token);
    bool verify_api_key(const std::string& api_key);
    bool verify_basic_auth(const std::string& credentials);
    
    // Utility methods
    std::string generate_client_id() const;
    void update_last_activity();
    void record_message_latency(std::chrono::milliseconds latency);
    bool is_rate_limited() const;
    void log_message(const std::string& message, const std::string& level = "info") const;
};

class WebSocketServer {
public:
    WebSocketServer();
    ~WebSocketServer();
    
    // Server lifecycle
    bool initialize(const WebSocketServerConfig& config);
    bool start();
    void stop();
    bool is_running() const;
    
    // Configuration
    void update_config(const WebSocketServerConfig& config);
    WebSocketServerConfig get_config() const;
    
    // Connection management
    std::vector<std::string> get_active_connections() const;
    std::shared_ptr<WebSocketSession> get_connection(const std::string& connection_id);
    std::shared_ptr<WebSocketSession> get_connection_by_client(const std::string& client_id);
    size_t get_active_connection_count() const;
    
    // Message broadcasting
    void broadcast_message(const WebSocketMessage& message);
    void broadcast_to_authenticated(const WebSocketMessage& message);
    void broadcast_to_subscribers(const WebSocketMessage& message, const std::string& event);
    void send_to_connection(const std::string& connection_id, const WebSocketMessage& message);
    void send_to_client(const std::string& client_id, const WebSocketMessage& message);
    
    // Event callbacks
    void set_orchestrator(std::shared_ptr<Orchestrator> orchestrator);
    void set_metrics(std::shared_ptr<CreationMetrics> metrics);
    void set_gmail_creator(std::shared_ptr<GmailCreator> gmail_creator);
    void set_activity_simulator(std::shared_ptr<ActivitySimulator> activity_simulator);
    
    // Event publishing
    void publish_creation_started(const std::string& account_id, const std::map<std::string, std::string>& details);
    void publish_creation_progress(const std::string& account_id, const std::string& step, int progress);
    void publish_creation_completed(const std::string& account_id, const CreationResult& result);
    void publish_creation_failed(const std::string& account_id, const std::string& error);
    
    void publish_warming_started(const std::string& account_id, const std::map<std::string, std::string>& details);
    void publish_warming_progress(const std::string& account_id, const std::string& activity, int progress);
    void publish_warming_completed(const std::string& account_id, const std::map<std::string, std::string>& results);
    void publish_warming_failed(const std::string& account_id, const std::string& error);
    
    void publish_metrics_update(const nlohmann::json& metrics);
    void publish_proxy_update(const std::map<std::string, std::string>& proxy_info);
    void publish_system_status(const nlohmann::json& status);
    
    // Statistics
    std::map<std::string, std::chrono::seconds> get_connection_durations() const;
    std::map<std::string, size_t> get_message_counts() const;
    std::map<WebSocketMessageType, size_t> get_message_type_distribution() const;
    std::chrono::milliseconds get_average_message_latency() const;
    std::map<std::string, std::string> get_server_statistics() const;
    
    // History and logging
    std::vector<WebSocketMessage> get_message_history(size_t limit = 1000);
    bool export_message_history(const std::string& filename) const;
    bool export_connection_history(const std::string& filename) const;

private:
    // Server components
    boost::asio::io_context io_context_;
    boost::asio::ip::tcp::acceptor acceptor_;
    std::unique_ptr<boost::asio::ssl::context> ssl_context_;
    
    // Configuration
    WebSocketServerConfig config_;
    
    // Connection management
    std::map<std::string, std::shared_ptr<WebSocketSession>> connections_;
    std::map<std::string, std::string> client_to_connection_map_;
    mutable std::mutex connections_mutex_;
    
    // Thread pool
    std::vector<std::thread> thread_pool_;
    std::atomic<bool> running_{false};
    
    // Statistics
    std::atomic<size_t> total_connections_{0};
    std::atomic<size_t> total_messages_sent_{0};
    std::atomic<size_t> total_messages_received_{0};
    std::vector<std::chrono::milliseconds> message_latencies_;
    std::map<WebSocketMessageType, std::atomic<size_t>> message_type_counts_;
    mutable std::mutex stats_mutex_;
    
    // Message history
    std::vector<WebSocketMessage> message_history_;
    std::map<std::string, std::chrono::system_clock::time_point> connection_history_;
    mutable std::mutex history_mutex_;
    
    // Event callbacks
    std::shared_ptr<Orchestrator> orchestrator_;
    std::shared_ptr<CreationMetrics> metrics_;
    std::shared_ptr<GmailCreator> gmail_creator_;
    std::shared_ptr<ActivitySimulator> activity_simulator_;
    
    // Rate limiting
    std::map<std::string, std::pair<size_t, std::chrono::system_clock::time_point>> rate_limits_;
    mutable std::mutex rate_limits_mutex_;
    
    // Private methods
    bool setup_ssl_context();
    void do_accept();
    void handle_accept(boost::asio::ip::tcp::socket socket);
    void handle_connection_closed(const std::string& connection_id);
    
    // Message processing
    void process_broadcast_message(const WebSocketMessage& message);
    void process_direct_message(const WebSocketMessage& message);
    void process_subscription_message(const WebSocketMessage& message);
    
    // Rate limiting
    bool is_rate_limited(const std::string& connection_id);
    void update_rate_limit(const std::string& connection_id);
    void cleanup_expired_rate_limits();
    
    // Statistics tracking
    void record_connection(const std::string& connection_id);
    void record_disconnection(const std::string& connection_id);
    void record_message_sent(WebSocketMessageType type);
    void record_message_received(WebSocketMessageType type);
    void record_message_latency(std::chrono::milliseconds latency);
    
    // Event handling
    void setup_event_handlers();
    void handle_orchestrator_events();
    void handle_metrics_events();
    void handle_creator_events();
    void handle_simulator_events();
    
    // Utility methods
    std::string generate_connection_id() const;
    void cleanup_inactive_connections();
    std::vector<std::shared_ptr<WebSocketSession>> get_authenticated_connections();
    std::vector<std::shared_ptr<WebSocketSession>> get_subscribed_connections(const std::string& event);
    
    // Logging
    void log_server_info(const std::string& message) const;
    void log_server_error(const std::string& message) const;
    void log_server_debug(const std::string& message) const;
};

// WebSocket message handler interface
class WebSocketMessageHandler {
public:
    virtual ~WebSocketMessageHandler() = default;
    virtual bool can_handle(WebSocketMessageType type) = 0;
    virtual void handle_message(const WebSocketMessage& message, std::shared_ptr<WebSocketSession> session) = 0;
    virtual std::string get_handler_name() const = 0;
};

// Connection message handler
class ConnectionMessageHandler : public WebSocketMessageHandler {
public:
    bool can_handle(WebSocketMessageType type) override;
    void handle_message(const WebSocketMessage& message, std::shared_ptr<WebSocketSession> session) override;
    std::string get_handler_name() const override { return "ConnectionHandler"; }
};

// Authentication message handler
class AuthenticationMessageHandler : public WebSocketMessageHandler {
public:
    AuthenticationMessageHandler(std::shared_ptr<WebSocketServerConfig> config);
    bool can_handle(WebSocketMessageType type) override;
    void handle_message(const WebSocketMessage& message, std::shared_ptr<WebSocketSession> session) override;
    std::string get_handler_name() const override { return "AuthenticationHandler"; }

private:
    std::shared_ptr<WebSocketServerConfig> config_;
    bool authenticate_connection(const WebSocketMessage& message, std::shared_ptr<WebSocketSession> session);
};

// Subscription message handler
class SubscriptionMessageHandler : public WebSocketMessageHandler {
public:
    bool can_handle(WebSocketMessageType type) override;
    void handle_message(const WebSocketMessage& message, std::shared_ptr<WebSocketSession> session) override;
    std::string get_handler_name() const override { return "SubscriptionHandler"; }
};

// Status message handler
class StatusMessageHandler : public WebSocketMessageHandler {
public:
    StatusMessageHandler(std::shared_ptr<WebSocketServer> server);
    bool can_handle(WebSocketMessageType type) override;
    void handle_message(const WebSocketMessage& message, std::shared_ptr<WebSocketSession> session) override;
    std::string get_handler_name() const override { return "StatusHandler"; }

private:
    std::shared_ptr<WebSocketServer> server_;
    nlohmann::json get_server_status();
    nlohmann::json get_connection_status();
};

// WebSocket authentication manager
class WebSocketAuthManager {
public:
    WebSocketAuthManager(std::shared_ptr<WebSocketServerConfig> config);
    ~WebSocketAuthManager() = default;
    
    // Authentication methods
    std::string generate_jwt_token(const std::string& user_id, const std::vector<std::string>& permissions);
    bool validate_jwt_token(const std::string& token, std::string& user_id, std::vector<std::string>& permissions);
    bool validate_api_key(const std::string& api_key, std::string& user_id);
    bool validate_basic_auth(const std::string& credentials, std::string& user_id);
    
    // Token management
    bool revoke_token(const std::string& token);
    bool is_token_revoked(const std::string& token);
    void cleanup_expired_tokens();
    
    // User management
    bool register_user(const std::string& user_id, const std::string& api_key, const std::vector<std::string>& permissions);
    bool unregister_user(const std::string& user_id);
    bool update_user_permissions(const std::string& user_id, const std::vector<std::string>& permissions);
    std::vector<std::string> get_user_permissions(const std::string& user_id);

private:
    std::shared_ptr<WebSocketServerConfig> config_;
    std::map<std::string, std::string> user_api_keys_;
    std::map<std::string, std::vector<std::string>> user_permissions_;
    std::map<std::string, std::chrono::system_clock::time_point> revoked_tokens_;
    mutable std::mutex auth_mutex_;
    
    std::string sign_jwt(const std::string& payload);
    bool verify_jwt_signature(const std::string& token, std::string& payload);
    std::string generate_secret_key() const;
};

// WebSocket message compressor
class WebSocketMessageCompressor {
public:
    WebSocketMessageCompressor();
    ~WebSocketMessageCompressor() = default;
    
    std::string compress_message(const std::string& message);
    std::string decompress_message(const std::string& compressed_message);
    bool is_compression_beneficial(const std::string& message) const;

private:
    std::string compress_gzip(const std::string& data);
    std::string decompress_gzip(const std::string& compressed_data);
};

} // namespace gmail_infinity
