#include "orchestrator.h"
#include "app_logger.h"
#include "metrics.h"
#include "secure_vault.h"

#include <yaml-cpp/yaml.h>
#include <chrono>
#include <random>
#include <algorithm>

#include "core/proxy_manager.h"
#include "identity/persona_generator.h"
#include "creators/gmail_creator.h"
#include "api/rest_server.h"
#include "api/websocket_server.h"
#include "api/dashboard.h"

namespace gmail_infinity {

Orchestrator::Orchestrator(
    const std::string& config_file,
    size_t thread_count,
    bool mock_mode,
    std::shared_ptr<CreationMetrics> metrics,
    std::shared_ptr<SecureVault> vault
) : config_file_(config_file),
    thread_count_(thread_count),
    mock_mode_(mock_mode),
    metrics_(metrics),
    vault_(vault) {
    
    auto logger = get_logger();
    logger->info("Orchestrator initialized with {} threads", thread_count_);
}

Orchestrator::~Orchestrator() {
    shutdown();
}

bool Orchestrator::initialize() {
    auto logger = get_logger();
    
    try {
        logger->info("Initializing orchestrator...");
        
        // Load configuration
        if (!load_configuration()) {
            logger->error("Failed to load configuration");
            return false;
        }
        
        // Setup thread pool
        setup_thread_pool();
        
        // Initialize core components
        logger->info("Initializing core components...");
        
        // These will be implemented in the respective modules
        // proxy_manager_ = std::make_unique<ProxyManager>();
        // persona_generator_ = std::make_unique<PersonaGenerator>();
        // gmail_creator_ = std::make_unique<GmailCreator>(mock_mode_);
        
        logger->info("Orchestrator initialized successfully");
        return true;
        
    } catch (const std::exception& e) {
        logger->error("Orchestrator initialization failed: {}", e.what());
        return false;
    }
}

void Orchestrator::shutdown() {
    auto logger = get_logger();
    logger->info("Shutting down orchestrator...");
    
    running_ = false;
    
    // Shutdown thread pool
    if (thread_pool_) {
        thread_pool_->join();
        thread_pool_.reset();
    }
    
    // Wait for worker threads
    for (auto& thread : worker_threads_) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    worker_threads_.clear();
    
    // Shutdown API servers
    if (rest_server_) {
        rest_server_->stop();
        rest_server_.reset();
    }
    
    if (websocket_server_) {
        websocket_server_->stop();
        websocket_server_.reset();
    }
    
    // Shutdown dashboard
    if (dashboard_) {
        dashboard_->stop();
        dashboard_.reset();
    }
    
    logger->info("Orchestrator shutdown complete");
}

void Orchestrator::run() {
    auto logger = get_logger();
    logger->info("Starting main execution loop...");
    
    running_ = true;
    
    // Start worker threads
    for (size_t i = 0; i < thread_count_; ++i) {
        worker_threads_.emplace_back([this, i]() {
            worker_loop(i);
        });
    }
    
    // Main orchestration loop
    while (running_ && should_continue_creating()) {
        try {
            schedule_account_creation();
            std::this_thread::sleep_for(get_next_delay());
            update_dashboard();
            log_statistics();
        } catch (const std::exception& e) {
            logger->error("Error in main loop: {}", e.what());
            std::this_thread::sleep_for(std::chrono::seconds(5));
        }
    }
    
    logger->info("Main execution loop completed");
}

void Orchestrator::start_api_server(uint16_t port) {
    auto logger = get_logger();
    
    try {
        // These will be implemented when we create the API modules
        // rest_server_ = std::make_unique<RestServer>(port, metrics_, vault_);
        // websocket_server_ = std::make_unique<WebSocketServer>(port + 1, metrics_);
        
        // rest_server_->start();
        // websocket_server_->start();
        
        logger->info("API server started on port {}", port);
        
    } catch (const std::exception& e) {
        logger->error("Failed to start API server: {}", e.what());
        throw;
    }
}

void Orchestrator::start_dashboard() {
    auto logger = get_logger();
    
    try {
        // This will be implemented when we create the dashboard module
        // dashboard_ = std::make_unique<Dashboard>(metrics_);
        // dashboard_->start();
        
        logger->info("Terminal dashboard started");
        
    } catch (const std::exception& e) {
        logger->error("Failed to start dashboard: {}", e.what());
        throw;
    }
}

bool Orchestrator::load_configuration() {
    auto logger = get_logger();
    
    try {
        YAML::Node config = YAML::LoadFile(config_file_);
        
        // Validate configuration structure
        if (!config["creation"]) {
            logger->error("Missing 'creation' section in configuration");
            return false;
        }
        
        if (!config["proxies"]) {
            logger->error("Missing 'proxies' section in configuration");
            return false;
        }
        
        if (!config["browser"]) {
            logger->error("Missing 'browser' section in configuration");
            return false;
        }
        
        logger->info("Configuration loaded successfully");
        return true;
        
    } catch (const YAML::Exception& e) {
        logger->error("Configuration parsing error: {}", e.what());
        return false;
    } catch (const std::exception& e) {
        logger->error("Failed to load configuration: {}", e.what());
        return false;
    }
}

void Orchestrator::setup_thread_pool() {
    auto logger = get_logger();
    
    thread_pool_ = std::make_unique<boost::asio::thread_pool>(thread_count_);
    logger->info("Thread pool created with {} threads", thread_count_);
}

void Orchestrator::worker_loop(size_t worker_id) {
    auto logger = get_logger();
    logger->debug("Worker thread {} started", worker_id);
    
    while (running_) {
        try {
            // Worker will process tasks from the thread pool
            // Task processing logic will be implemented here
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        } catch (const std::exception& e) {
            logger->error("Worker {} error: {}", worker_id, e.what());
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
    
    logger->debug("Worker thread {} stopped", worker_id);
}

void Orchestrator::schedule_account_creation() {
    auto logger = get_logger();
    
    if (active_tasks_.load() >= thread_count_) {
        logger->debug("Maximum active tasks reached, skipping scheduling");
        return;
    }
    
    // Schedule account creation task
    boost::asio::post(*thread_pool_, [this]() {
        try {
            active_tasks_++;
            
            // Account creation logic will be implemented here
            // This is a placeholder for the actual Gmail creation process
            bool success = true; // Placeholder
            std::string details = "Account created successfully"; // Placeholder
            
            handle_task_completion(success, details);
            
        } catch (const std::exception& e) {
            auto logger = get_logger();
            logger->error("Account creation task failed: {}", e.what());
            handle_task_completion(false, e.what());
        }
        
        active_tasks_--;
    });
}

void Orchestrator::handle_task_completion(bool success, const std::string& details) {
    auto logger = get_logger();
    
    if (success) {
        metrics_->increment_successful();
        logger->info("Task completed successfully: {}", details);
    } else {
        metrics_->increment_failed();
        logger->error("Task failed: {}", details);
    }
    
    metrics_->increment_total();
}

std::chrono::milliseconds Orchestrator::get_next_delay() {
    // Load configuration for delay settings
    // This is a placeholder - will be implemented with actual config loading
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(20, 40); // 20-40 seconds
    
    return std::chrono::seconds(dis(gen));
}

bool Orchestrator::should_continue_creating() {
    // Check if we should continue based on configuration and metrics
    // This is a placeholder - will be implemented with actual logic
    return running_ && metrics_->get_total_created() < 100; // Placeholder limit
}

void Orchestrator::update_dashboard() {
    if (dashboard_) {
        // dashboard_->update();
    }
}

void Orchestrator::log_statistics() {
    auto logger = get_logger();
    
    logger->info("Statistics - Total: {}, Successful: {}, Failed: {}, Active: {}",
        metrics_->get_total_created(),
        metrics_->get_successful_count(),
        metrics_->get_failed_count(),
        active_tasks_.load()
    );
}

void Orchestrator::save_statistics() {
    metrics_->save("output/metrics.json");
}

} // namespace gmail_infinity
