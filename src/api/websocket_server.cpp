#include "websocket_server.h"
#include "app/app_logger.h"

#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <thread>
#include <chrono>

namespace gmail_infinity {

using json = nlohmann::json;
namespace beast = boost::beast;
namespace http = beast::http;
namespace websocket = beast::websocket;
namespace net = boost::asio;
using tcp = boost::asio::ip::tcp;

// WebSocketSession implementation
WebSocketSession::WebSocketSession(tcp::socket socket, std::string connection_id, std::shared_ptr<WebSocketServerConfig> config)
    : socket_(std::move(socket)), connection_id_(connection_id), config_(config) {
    ws_ = std::make_unique<websocket::stream<beast::tcp_stream>>(std::move(socket_));
}

WebSocketSession::~WebSocketSession() {
    stop();
}

void WebSocketSession::start() {
    running_ = true;
    connected_at_ = std::chrono::system_clock::now();
    last_activity_ = connected_at_;
    
    // Set suggested timeout settings for the websocket
    ws_->set_option(websocket::stream_base::timeout::suggested(beast::role_type::server));
    
    // Accept the websocket handshake
    ws_->async_accept([this](beast::error_code ec) {
        if (ec) {
            spdlog::error("WebSocket accept error: {}", ec.message());
            return;
        }
        state_ = WebSocketConnectionState::CONNECTED;
        do_read();
    });
}

void WebSocketSession::stop() {
    running_ = false;
    beast::error_code ec;
    ws_->close(websocket::close_code::normal, ec);
    state_ = WebSocketConnectionState::DISCONNECTED;
}

void WebSocketSession::do_read() {
    beast::flat_buffer buffer;
    ws_->async_read(buffer, [this, &buffer](beast::error_code ec, std::size_t bytes_transferred) {
        if (ec == websocket::error::closed) return;
        if (ec) {
            spdlog::error("WebSocket read error: {}", ec.message());
            return;
        }
        
        std::string message_str = beast::buffers_to_string(buffer.data());
        handle_message(message_str);
        
        messages_received_++;
        last_activity_ = std::chrono::system_clock::now();
        
        do_read(); // Continue reading
    });
}

void WebSocketSession::handle_message(const std::string& message_str) {
    try {
        json j = json::parse(message_str);
        WebSocketMessage msg;
        msg.from_json(j);
        
        switch (msg.type) {
            case WebSocketMessageType::HEARTBEAT:
                handle_heartbeat();
                break;
            default:
                spdlog::info("WebSocket received message type: {}", static_cast<int>(msg.type));
                break;
        }
    } catch (const std::exception& e) {
        spdlog::error("WebSocket message parse error: {}", e.what());
    }
}

void WebSocketSession::handle_heartbeat() {
    WebSocketMessage pong;
    pong.type = WebSocketMessageType::HEARTBEAT;
    pong.timestamp = std::chrono::system_clock::now();
    send_message(pong);
}

void WebSocketSession::send_message(const WebSocketMessage& message) {
    std::lock_guard<std::mutex> lock(session_mutex_);
    if (!running_) return;
    
    beast::error_code ec;
    ws_->write(net::buffer(message.to_json().dump()), ec);
    if (ec) {
        spdlog::error("WebSocket write error: {}", ec.message());
    } else {
        messages_sent_++;
    }
}

// WebSocketServer implementation
WebSocketServer::WebSocketServer() : acceptor_(io_context_) {}

WebSocketServer::~WebSocketServer() {
    stop();
}

bool WebSocketServer::initialize(const WebSocketServerConfig& config) {
    config_ = config;
    tcp::endpoint endpoint{net::ip::make_address(config.host), config.port};
    
    beast::error_code ec;
    acceptor_.open(endpoint.protocol(), ec);
    acceptor_.set_option(net::socket_base::reuse_address(true), ec);
    acceptor_.bind(endpoint, ec);
    acceptor_.listen(net::socket_base::max_listen_connections, ec);
    
    if (ec) {
        spdlog::error("WebSocket server listen error: {}", ec.message());
        return false;
    }
    
    return true;
}

bool WebSocketServer::start() {
    running_ = true;
    do_accept();
    
    // Run io_context in a pool of threads
    for (int i = 0; i < config_.thread_pool_size; ++i) {
        thread_pool_.emplace_back([this]() {
            io_context_.run();
        });
    }
    
    spdlog::info("WebSocket server started on {}:{}", config_.host, config_.port);
    return true;
}

void WebSocketServer::stop() {
    running_ = false;
    io_context_.stop();
    for (auto& t : thread_pool_) {
        if (t.joinable()) t.join();
    }
    thread_pool_.clear();
}

void WebSocketServer::do_accept() {
    acceptor_.async_accept([this](beast::error_code ec, tcp::socket socket) {
        if (!ec) {
            std::string cid = generate_connection_id();
            auto session = std::make_shared<WebSocketSession>(std::move(socket), cid, std::make_shared<WebSocketServerConfig>(config_));
            
            {
                std::lock_guard<std::mutex> lock(connections_mutex_);
                connections_[cid] = session;
            }
            
            session->start();
            total_connections_++;
        }
        
        if (running_) do_accept();
    });
}

void WebSocketServer::broadcast_message(const WebSocketMessage& message) {
    std::lock_guard<std::mutex> lock(connections_mutex_);
    for (auto& [id, session] : connections_) {
        session->send_message(message);
    }
}

std::string WebSocketServer::generate_connection_id() const {
    return "conn_" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
}

// JSON implementations
nlohmann::json WebSocketMessage::to_json() const {
    return json{
        {"type", static_cast<int>(type)},
        {"message_id", message_id},
        {"timestamp", std::chrono::system_clock::to_time_t(timestamp)},
        {"data", data}
    };
}

void WebSocketMessage::from_json(const nlohmann::json& j) {
    if (j.contains("type")) type = static_cast<WebSocketMessageType>(j["type"].get<int>());
}

nlohmann::json WebSocketServerConfig::to_json() const { return json::object(); }

} // namespace gmail_infinity
