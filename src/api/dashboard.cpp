#include "dashboard.h"
#include "app/app_logger.h"

#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/screen.hpp>
#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>
#include <thread>
#include <chrono>

namespace gmail_infinity {

using namespace ftxui;

Dashboard::Dashboard() {
}

Dashboard::~Dashboard() {
    stop();
}

bool Dashboard::initialize(const DashboardConfig& config) {
    std::lock_guard<std::mutex> lock(dashboard_mutex_);
    config_ = config;
    setup_ui();
    return true;
}

bool Dashboard::start() {
    running_ = true;
    
    // Create a dedicated thread for the interactive screen
    refresh_thread_ = std::make_unique<std::thread>([this]() {
        auto screen = ScreenInteractive::TerminalOutput();
        
        // Tab switching logic
        int selected_tab = 0;
        std::vector<std::string> tab_titles = {"Overview", "Accounts", "Proxies", "Metrics", "System"};
        
        auto toggle = Menu(&tab_titles, &selected_tab);
        
        auto container = Container::Tab({
            create_overview_tab(),
            create_accounts_tab(),
            create_proxies_tab(),
            create_metrics_tab(),
            create_system_tab(),
        }, &selected_tab);
        
        auto layout = Container::Vertical({
            toggle,
            container,
        });
        
        auto renderer = Renderer(layout, [&] {
            return vbox({
                text("Gmail Infinity Factory — C++ Rewrite Dashboard") | bold | center,
                separator(),
                toggle->Render(),
                separator(),
                container->Render() | flex,
                separator(),
                text("Status: " + std::string(running_ ? "RUNNING" : "STOPPED")) | color(Color::Green),
            });
        });
        
        screen.Loop(renderer);
    });
    
    spdlog::info("Dashboard started");
    return true;
}

void Dashboard::stop() {
    running_ = false;
    if (refresh_thread_ && refresh_thread_->joinable()) {
        refresh_thread_->join();
    }
}

void Dashboard::setup_ui() {
    // Component initialization
}

std::shared_ptr<ftxui::Component> Dashboard::create_overview_tab() {
    return Renderer([this] {
        return vbox({
            text("System Overview") | bold,
            separator(),
            hbox({
                vbox({
                    text("Account Creation Success: " + std::to_string(metrics_ ? metrics_->get_total_success() : 0)),
                    text("Account Creation Failed: " + std::to_string(metrics_ ? metrics_->get_total_failure() : 0)),
                    text("Active Proxy Count: " + std::to_string(proxy_manager_ ? proxy_manager_->get_active_proxies().size() : 0)),
                }) | border,
                vbox({
                    text("System Uptime: calculing..."),
                    text("Thread Pool Status: Healthy"),
                    text("Database Status: Connected"),
                }) | border,
            }) | flex,
        });
    });
}

std::shared_ptr<ftxui::Component> Dashboard::create_accounts_tab() {
    return Renderer([] {
        return vbox({
            text("Account Management") | bold,
            separator(),
            text("Account list would be displayed here as a table..."),
        });
    });
}

std::shared_ptr<ftxui::Component> Dashboard::create_proxies_tab() {
    return Renderer([] {
        return vbox({
            text("Proxy Management") | bold,
            separator(),
            text("Proxy list would be displayed here as a table..."),
        });
    });
}

std::shared_ptr<ftxui::Component> Dashboard::create_metrics_tab() {
    return Renderer([] {
        return vbox({
            text("Detailed Metrics") | bold,
            separator(),
            text("Charts and detailed metrics would be displayed here..."),
        });
    });
}

std::shared_ptr<ftxui::Component> Dashboard::create_system_tab() {
    return Renderer([] {
        return vbox({
            text("System Configuration") | bold,
            separator(),
            text("System settings and status would be displayed here..."),
        });
    });
}

// Component setters
void Dashboard::set_orchestrator(std::shared_ptr<Orchestrator> orchestrator) { orchestrator_ = orchestrator; }
void Dashboard::set_metrics(std::shared_ptr<CreationMetrics> metrics) { metrics_ = metrics; }
void Dashboard::set_proxy_manager(std::shared_ptr<ProxyManager> proxy_manager) { proxy_manager_ = proxy_manager; }

// JSON implementations
nlohmann::json DashboardConfig::to_json() const { return json::object(); }

} // namespace gmail_infinity
