#include "rest_server.h"
#include "app/app_logger.h"

#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>
#include <crow.h>
#include <thread>
#include <chrono>

namespace gmail_infinity {

using json = nlohmann::json;

RestServer::RestServer() {
}

RestServer::~RestServer() {
    stop();
}

bool RestServer::initialize(const RestServerConfig& config) {
    std::lock_guard<std::mutex> lock(server_mutex_);
    config_ = config;
    app_ = std::make_unique<crow::SimpleApp>();
    
    setup_routes();
    return true;
}

bool RestServer::start() {
    running_ = true;
    
    server_thread_ = std::make_unique<std::thread>([this]() {
        app_->port(config_.port).bindaddr(config_.host).multithreaded().run();
    });
    
    spdlog::info("RestServer started on {}:{}", config_.host, config_.port);
    return true;
}

void RestServer::stop() {
    running_ = false;
    if (app_) app_->stop();
    if (server_thread_ && server_thread_->joinable()) {
        server_thread_->join();
    }
}

void RestServer::setup_routes() {
    // Health check
    CROW_ROUTE((*app_), "/health")([this](const crow::request& req) {
        return handle_health_check(req);
    });

    // Metrics
    CROW_ROUTE((*app_), "/metrics")([this](const crow::request& req) {
        return handle_get_metrics(req);
    });

    // Account creation
    CROW_ROUTE((*app_), "/accounts").methods(crow::HTTPMethod::Post)([this](const crow::request& req) {
        return handle_create_account(req);
    });

    // Account list
    CROW_ROUTE((*app_), "/accounts").methods(crow::HTTPMethod::Get)([this](const crow::request& req) {
        return handle_list_accounts(req);
    });

    // System status
    CROW_ROUTE((*app_), "/status")([this](const crow::request& req) {
        return handle_get_system_status(req);
    });
}

crow::response RestServer::handle_health_check(const crow::request& req) {
    json response = {{"status", "healthy"}, {"timestamp", std::chrono::system_clock::to_time_t(std::chrono::system_clock::now())}};
    return crow::response(200, response.dump());
}

crow::response RestServer::handle_get_metrics(const crow::request& req) {
    if (!metrics_) return crow::response(503, "Metrics component not available");
    
    json response = metrics_->to_json();
    return crow::response(200, response.dump());
}

crow::response RestServer::handle_create_account(const crow::request& req) {
    if (!orchestrator_) return crow::response(503, "Orchestrator not available");
    
    try {
        json body = json::parse(req.body);
        CreationRequest creation_req; // Assuming this maps to common request structure
        // Populate from body...
        
        auto result = orchestrator_->queue_creation_async(creation_req);
        
        json response = {{"status", "queued"}, {"request_id", "req_" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count())}};
        return crow::response(202, response.dump());
        
    } catch (const std::exception& e) {
        return create_error_response(400, "INVALID_REQUEST", e.what());
    }
}

crow::response RestServer::handle_list_accounts(const crow::request& req) {
    // This would fetch from the database/vault
    json response = json::array();
    return crow::response(200, response.dump());
}

crow::response RestServer::handle_get_system_status(const crow::request& req) {
    json response = {
        {"uptime", "calculating..."},
        {"active_tasks", orchestrator_ ? orchestrator_->get_active_task_count() : 0},
        {"total_success", metrics_ ? metrics_->get_total_success() : 0}
    };
    return crow::response(200, response.dump());
}

crow::response RestServer::create_error_response(int status_code, const std::string& error_code, const std::string& error_message) {
    json response = {
        {"error", {
            {"code", error_code},
            {"message", error_message}
        }}
    };
    return crow::response(status_code, response.dump());
}

// Configuration
void RestServer::set_orchestrator(std::shared_ptr<Orchestrator> orchestrator) { orchestrator_ = orchestrator; }
void RestServer::set_metrics(std::shared_ptr<CreationMetrics> metrics) { metrics_ = metrics; }
void RestServer::set_secure_vault(std::shared_ptr<SecureVault> secure_vault) { secure_vault_ = secure_vault; }

// JSON implementations
nlohmann::json RestServerConfig::to_json() const { return json::object(); }

} // namespace gmail_infinity
