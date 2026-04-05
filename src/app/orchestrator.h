#pragma once

#include <memory>
#include <string>
#include <thread>
#include <atomic>
#include <vector>
#include <functional>

#include <boost/asio/thread_pool.hpp>
#include <boost/asio/post.hpp>

#include "app/metrics.h"
#include "app/secure_vault.h"

namespace gmail_infinity {

class Orchestrator {
public:
    Orchestrator(
        const std::string& config_file,
        size_t thread_count,
        bool mock_mode,
        std::shared_ptr<CreationMetrics> metrics,
        std::shared_ptr<SecureVault> vault
    );
    
    ~Orchestrator();
    
    // Initialization
    bool initialize();
    void shutdown();
    
    // Main execution
    void run();
    
    // API and dashboard
    void start_api_server(uint16_t port);
    void start_dashboard();
    
    // Configuration access
    const std::string& get_config_file() const { return config_file_; }
    bool is_mock_mode() const { return mock_mode_; }
    size_t get_thread_count() const { return thread_count_; }
    
    // Metrics access
    std::shared_ptr<CreationMetrics> get_metrics() { return metrics_; }
    std::shared_ptr<SecureVault> get_vault() { return vault_; }

private:
    // Configuration
    std::string config_file_;
    size_t thread_count_;
    bool mock_mode_;
    
    // Core components
    std::shared_ptr<CreationMetrics> metrics_;
    std::shared_ptr<SecureVault> vault_;
    
    // Thread management
    std::unique_ptr<boost::asio::thread_pool> thread_pool_;
    std::atomic<bool> running_{false};
    
    // Task management
    std::vector<std::thread> worker_threads_;
    std::atomic<size_t> active_tasks_{0};
    
    // API server
    std::unique_ptr<class RestServer> rest_server_;
    std::unique_ptr<class WebSocketServer> websocket_server_;
    
    // Dashboard
    std::unique_ptr<class Dashboard> dashboard_;
    
    // Private methods
    bool load_configuration();
    void setup_thread_pool();
    void worker_loop(size_t worker_id);
    void schedule_account_creation();
    void handle_task_completion(bool success, const std::string& details);
    void update_dashboard();
    
    // Task scheduling
    std::chrono::milliseconds get_next_delay();
    bool should_continue_creating();
    
    // Statistics
    void log_statistics();
    void save_statistics();
};

} // namespace gmail_infinity
