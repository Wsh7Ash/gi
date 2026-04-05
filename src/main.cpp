#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <chrono>
#include <signal.h>

#include <CLI/CLI.hpp>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>

#include "app/orchestrator.h"
#include "app/app_logger.h"
#include "app/metrics.h"
#include "app/secure_vault.h"

// Global flag for graceful shutdown
std::atomic<bool> g_shutdown_requested{false};

// Signal handler for SIGINT and SIGTERM
void signal_handler(int signal) {
    spdlog::info("Received signal {}, shutting down gracefully...", signal);
    g_shutdown_requested = true;
}

int main(int argc, char** argv) {
    try {
        // Setup CLI
        CLI::App app{"Gmail Infinity Factory - C++ Rewrite"};
        
        std::string config_file = "config/settings.yaml";
        size_t thread_count = std::thread::hardware_concurrency();
        bool mock_mode = false;
        bool enable_api = false;
        uint16_t api_port = 8080;
        bool enable_dashboard = true;
        std::string log_level = "info";
        
        app.add_option("-c,--config", config_file, "Configuration file path");
        app.add_option("-t,--threads", thread_count, "Number of worker threads");
        app.add_flag("-m,--mock", mock_mode, "Run in mock mode (no real browser)");
        app.add_flag("-a,--api", enable_api, "Enable REST API server");
        app.add_option("-p,--port", api_port, "API server port");
        app.add_flag("-d,--dashboard", enable_dashboard, "Enable terminal dashboard");
        app.add_option("-l,--log-level", log_level, "Log level (trace, debug, info, warn, error)")
            ->check(CLI::IsMember({"trace", "debug", "info", "warn", "error"}));
        
        // Parse CLI arguments
        CLI11_PARSE(app, argc, argv);
        
        // Setup logging
        auto logger = gmail_infinity::setup_logging(log_level);
        logger->info("Gmail Infinity Factory starting...");
        logger->info("Configuration: {}", config_file);
        logger->info("Thread count: {}", thread_count);
        logger->info("Mock mode: {}", mock_mode ? "enabled" : "disabled");
        
        // Setup signal handlers
        std::signal(SIGINT, signal_handler);
        std::signal(SIGTERM, signal_handler);
        
        // Initialize application components
        auto metrics = std::make_shared<gmail_infinity::CreationMetrics>();
        auto vault = std::make_shared<gmail_infinity::SecureVault>();
        
        // Load vault (decrypt existing credentials if any)
        if (!vault->load_or_create()) {
            logger->error("Failed to initialize secure vault");
            return 1;
        }
        
        // Create orchestrator
        gmail_infinity::Orchestrator orchestrator(
            config_file,
            thread_count,
            mock_mode,
            metrics,
            vault
        );
        
        // Initialize orchestrator
        if (!orchestrator.initialize()) {
            logger->error("Failed to initialize orchestrator");
            return 1;
        }
        
        // Start API server if requested
        std::thread api_thread;
        if (enable_api) {
            api_thread = std::thread([&]() {
                try {
                    orchestrator.start_api_server(api_port);
                } catch (const std::exception& e) {
                    logger->error("API server error: {}", e.what());
                }
            });
            logger->info("API server started on port {}", api_port);
        }
        
        // Start dashboard if requested
        std::thread dashboard_thread;
        if (enable_dashboard) {
            dashboard_thread = std::thread([&]() {
                try {
                    orchestrator.start_dashboard();
                } catch (const std::exception& e) {
                    logger->error("Dashboard error: {}", e.what());
                }
            });
            logger->info("Terminal dashboard started");
        }
        
        // Main execution loop
        logger->info("Starting main execution loop...");
        orchestrator.run();
        
        // Wait for shutdown signal
        while (!g_shutdown_requested) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        
        // Graceful shutdown
        logger->info("Initiating graceful shutdown...");
        orchestrator.shutdown();
        
        // Wait for threads to finish
        if (api_thread.joinable()) {
            api_thread.join();
        }
        if (dashboard_thread.joinable()) {
            dashboard_thread.join();
        }
        
        // Save final metrics
        metrics->save("output/final_metrics.json");
        logger->info("Final metrics saved");
        
        logger->info("Gmail Infinity Factory stopped successfully");
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Unknown fatal error occurred" << std::endl;
        return 1;
    }
}
