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

#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/screen.hpp>
#include <nlohmann/json.hpp>

#include "../app/orchestrator.h"
#include "../app/metrics.h"
#include "../core/proxy_manager.h"
#include "../creators/gmail_creator.h"
#include "../warming/activity_simulator.h"
#include "../api/websocket_server.h"

namespace gmail_infinity {

enum class DashboardTab {
    OVERVIEW,
    ACCOUNTS,
    PROXIES,
    METRICS,
    WARMING,
    VERIFICATION,
    SYSTEM,
    LOGS
};

enum class DashboardTheme {
    LIGHT,
    DARK,
    AUTO
};

struct DashboardConfig {
    // Display settings
    DashboardTheme theme = DashboardTheme::DARK;
    bool enable_colors = true;
    bool enable_unicode = true;
    std::string refresh_interval = "1s";
    int max_history_items = 100;
    
    // Layout settings
    bool show_status_bar = true;
    bool show_menu = true;
    bool show_help = false;
    int terminal_width = 120;
    int terminal_height = 40;
    
    // Data settings
    bool auto_refresh = true;
    bool show_detailed_metrics = true;
    bool show_real_time_updates = true;
    std::vector<DashboardTab> enabled_tabs;
    
    // Alert settings
    bool enable_alerts = true;
    bool enable_sound_alerts = false;
    double alert_threshold = 0.8; // 80% threshold for alerts
    std::vector<std::string> alert_events;
    
    // Performance settings
    bool enable_animations = true;
    int animation_fps = 30;
    bool enable_lazy_loading = true;
    
    nlohmann::json to_json() const;
    void from_json(const nlohmann::json& j);
};

struct DashboardWidget {
    std::string id;
    std::string title;
    std::string type;
    std::map<std::string, std::string> properties;
    std::vector<std::string> data_sources;
    bool auto_refresh;
    std::chrono::milliseconds refresh_interval;
    
    nlohmann::json to_json() const;
    void from_json(const nlohmann::json& j);
};

class Dashboard {
public:
    Dashboard();
    ~Dashboard();
    
    // Dashboard lifecycle
    bool initialize(const DashboardConfig& config);
    bool start();
    void stop();
    bool is_running() const;
    
    // Component registration
    void set_orchestrator(std::shared_ptr<Orchestrator> orchestrator);
    void set_metrics(std::shared_ptr<CreationMetrics> metrics);
    void set_proxy_manager(std::shared_ptr<ProxyManager> proxy_manager);
    void set_gmail_creator(std::shared_ptr<GmailCreator> gmail_creator);
    void set_activity_simulator(std::shared_ptr<ActivitySimulator> activity_simulator);
    void set_websocket_server(std::shared_ptr<WebSocketServer> websocket_server);
    
    // Configuration
    void update_config(const DashboardConfig& config);
    DashboardConfig get_config() const;
    
    // Widget management
    void add_widget(const DashboardWidget& widget);
    void remove_widget(const std::string& widget_id);
    std::vector<DashboardWidget> get_widgets() const;
    void update_widget_data(const std::string& widget_id, const nlohmann::json& data);
    
    // Tab management
    void switch_tab(DashboardTab tab);
    DashboardTab get_current_tab() const;
    std::vector<DashboardTab> get_enabled_tabs() const;
    
    // Data updates
    void update_metrics_data(const nlohmann::json& metrics);
    void update_accounts_data(const nlohmann::json& accounts);
    void update_proxies_data(const nlohmann::json& proxies);
    void update_warming_data(const nlohmann::json& warming);
    void update_verification_data(const nlohmann::json& verification);
    void update_system_data(const nloh::system_clock::time_point& status);
    
    // Event handling
    void handle_key_event(const std::string& key);
    void handle_mouse_event(const std::string& event);
    void handle_resize_event(int width, int height);
    
    // Statistics
    std::map<std::string, std::chrono::milliseconds> get_widget_refresh_times() const;
    std::map<DashboardTab, std::chrono::seconds> get_tab_view_times() const;
    std::vector<std::string> get_user_interactions() const;
    std::chrono::milliseconds get_average_render_time() const;

private:
    // FTXUI components
    std::unique_ptr<ftxui::ScreenInteractive> screen_;
    std::shared_ptr<ftxui::Component> root_component_;
    std::map<DashboardTab, std::shared_ptr<ftxui::Component>> tab_components_;
    
    // Configuration
    DashboardConfig config_;
    
    // Component references
    std::shared_ptr<Orchestrator> orchestrator_;
    std::shared_ptr<CreationMetrics> metrics_;
    std::shared_ptr<ProxyManager> proxy_manager_;
    std::shared_ptr<GmailCreator> gmail_creator_;
    std::shared_ptr<ActivitySimulator> activity_simulator_;
    std::shared_ptr<WebSocketServer> websocket_server_;
    
    // Widget management
    std::vector<DashboardWidget> widgets_;
    std::map<std::string, std::shared_ptr<ftxui::Component>> widget_components_;
    std::map<std::string, nlohmann::json> widget_data_;
    mutable std::mutex widgets_mutex_;
    
    // Data caches
    nlohmann::json cached_metrics_;
    nlohmann::json cached_accounts_;
    nlohmann::json cached_proxies_;
    nlohmann::json cached_warming_;
    nlohmann::json cached_verification_;
    nlohmann::json cached_system_;
    mutable std::mutex data_mutex_;
    
    // UI state
    DashboardTab current_tab_{DashboardTab::OVERVIEW};
    std::atomic<bool> running_{false};
    std::atomic<bool> refresh_enabled_{true};
    
    // Statistics
    std::map<std::string, std::vector<std::chrono::milliseconds>> widget_refresh_times_;
    std::map<DashboardTab, std::chrono::system_clock::time_point> tab_view_times_;
    std::vector<std::string> user_interactions_;
    std::atomic<std::chrono::milliseconds::rep> total_render_time_ms_{0};
    std::atomic<size_t> render_count_{0};
    mutable std::mutex stats_mutex_;
    
    // Refresh thread
    std::unique_ptr<std::thread> refresh_thread_;
    std::atomic<bool> refresh_running_{false};
    
    // Thread safety
    mutable std::mutex dashboard_mutex_;
    
    // Private methods
    void setup_ui();
    void setup_tabs();
    void setup_widgets();
    void setup_keybindings();
    
    // Tab components
    std::shared_ptr<ftxui::Component> create_overview_tab();
    std::shared_ptr<ftxui::Component> create_accounts_tab();
    std::shared_ptr<ftxui::Component> create_proxies_tab();
    std::shared_ptr<ftxui::Component> create_metrics_tab();
    std::shared_ptr<ftxui::Component> create_warming_tab();
    std::shared_ptr<ftxui::Component> create_verification_tab();
    std::shared_ptr<ftxui::Component> create_system_tab();
    std::shared_ptr<ftxui::Component> create_logs_tab();
    
    // Widget components
    std::shared_ptr<ftxui::Component> create_status_widget();
    std::shared_ptr<ftxui::Component> create_metrics_widget();
    std::shared_ptr<ftxui::Component> create_progress_widget();
    std::shared_ptr<ftxui::Component> create_activity_widget();
    std::shared_ptr<ftxui::Component> create_alert_widget();
    std::shared_ptr<ftxui::Component> create_chart_widget(const std::string& chart_type);
    
    // Data formatting
    std::string format_metrics_summary();
    std::string format_accounts_summary();
    std::string format_proxies_summary();
    std::string format_warming_summary();
    std::string format_verification_summary();
    std::string format_system_summary();
    
    // Chart data
    std::vector<std::pair<std::string, int>> get_creation_rate_chart();
    std::vector<std::pair<std::string, double>> get_success_rate_chart();
    std::vector<std::pair<std::string, int>> get_proxy_health_chart();
    std::vector<std::pair<std::string, int>> get_activity_chart();
    
    // Refresh loop
    void refresh_loop();
    void refresh_data();
    void refresh_widget(const std::string& widget_id);
    
    // Event handling
    void on_tab_switch(DashboardTab tab);
    void on_widget_click(const std::string& widget_id);
    void on_key_press(const std::string& key);
    void on_resize();
    
    // Theme management
    void apply_theme();
    ftxui::Color get_color_for_value(double value, double max);
    std::string get_status_indicator(bool status);
    
    // Utility methods
    std::string truncate_string(const std::string& str, size_t max_length);
    std::string format_number(double number, int precision = 2);
    std::string format_duration(std::chrono::milliseconds duration);
    std::string format_percentage(double percentage);
    std::string format_bytes(size_t bytes);
    
    // Statistics tracking
    void record_widget_refresh(const std::string& widget_id, std::chrono::milliseconds duration);
    void record_tab_view(DashboardTab tab);
    void record_user_interaction(const std::string& interaction);
    void record_render_time(std::chrono::milliseconds duration);
    
    // Logging
    void log_dashboard_info(const std::string& message) const;
    void log_dashboard_error(const std::string& message) const;
    void log_dashboard_debug(const std::string& message) const;
};

// Dashboard widget factory
class DashboardWidgetFactory {
public:
    DashboardWidgetFactory();
    ~DashboardWidgetFactory() = default;
    
    // Widget creation
    static std::shared_ptr<ftxui::Component> create_status_widget(const DashboardWidget& config, const nlohmann::json& data);
    static std::shared_ptr<ftxui::Component> create_progress_widget(const DashboardWidget& config, const nlohmann::json& data);
    static std::shared_ptr<ftxui::Component> create_chart_widget(const DashboardWidget& config, const nlohmann::json& data);
    static std::shared_ptr<ftxui::Component> create_table_widget(const DashboardWidget& config, const nlohmann::json& data);
    static std::shared_ptr<ftxui::Component> create_log_widget(const DashboardWidget& config, const nlohmann::json& data);
    
    // Chart types
    static std::shared_ptr<ftxui::Component> create_bar_chart(const std::vector<std::pair<std::string, int>>& data);
    static std::shared_ptr<ftxui::Component> create_line_chart(const std::vector<std::pair<std::string, double>>& data);
    static std::shared_ptr<ftxui::Component> create_pie_chart(const std::map<std::string, int>& data);
    static std::shared_ptr<ftxui::Component> create_gauge_chart(double value, double max, const std::string& label);
    
    // Table formatting
    static std::vector<std::vector<std::string>> format_table_data(const nlohmann::json& data, const std::vector<std::string>& columns);
    static std::string format_table_row(const std::vector<std::string>& row, const std::vector<int>& column_widths);

private:
    static std::string format_chart_bar(int value, int max_value, int bar_width);
    static std::string format_chart_point(double value, double max_value, int chart_width);
};

// Dashboard theme manager
class DashboardThemeManager {
public:
    DashboardThemeManager();
    ~DashboardThemeManager() = default;
    
    // Theme management
    void set_theme(DashboardTheme theme);
    DashboardTheme get_theme() const;
    void apply_theme_to_screen(std::shared_ptr<ftxui::ScreenInteractive> screen);
    
    // Color schemes
    ftxui::Color get_primary_color() const;
    ftxui::Color get_secondary_color() const;
    ftxui::Color get_success_color() const;
    ftxui::Color get_warning_color() const;
    ftxui::Color get_error_color() const;
    ftxui::Color get_background_color() const;
    ftxui::Color get_foreground_color() const;
    
    // Custom themes
    void register_custom_theme(const std::string& name, const std::map<std::string, ftxui::Color>& colors);
    bool apply_custom_theme(const std::string& name);

private:
    DashboardTheme current_theme_{DashboardTheme::DARK};
    std::map<std::string, std::map<std::string, ftxui::Color>> custom_themes_;
    
    void apply_light_theme();
    void apply_dark_theme();
    void apply_auto_theme();
    ftxui::Color hex_to_color(const std::string& hex) const;
};

// Dashboard event handler
class DashboardEventHandler {
public:
    DashboardEventHandler(std::shared_ptr<Dashboard> dashboard);
    ~DashboardEventHandler() = default;
    
    // Event registration
    void register_key_binding(const std::string& key, std::function<void()> handler);
    void register_mouse_binding(const std::string& gesture, std::function<void()> handler);
    void register_command(const std::string& command, std::function<void(const std::vector<std::string>&)> handler);
    
    // Event processing
    bool handle_key_event(const std::string& key);
    bool handle_mouse_event(const std::string& event);
    bool handle_command(const std::string& command, const std::vector<std::string>& args);
    
    // Help system
    std::vector<std::string> get_available_keybindings();
    std::vector<std::string> get_available_commands();
    std::string get_help_text() const;

private:
    std::shared_ptr<Dashboard> dashboard_;
    std::map<std::string, std::function<void()>> key_handlers_;
    std::map<std::string, std::function<void()>> mouse_handlers_;
    std::map<std::string, std::function<void(const std::vector<std::string>&)>> command_handlers_;
    mutable std::mutex handlers_mutex_;
    
    void setup_default_keybindings();
    void setup_default_commands();
    void setup_help_system();
    std::string format_keybinding_help() const;
    std::string format_command_help() const;
};

// Dashboard data provider
class DashboardDataProvider {
public:
    DashboardDataProvider();
    ~DashboardDataProvider() = default;
    
    // Data sources
    void register_data_source(const std::string& name, std::function<nlohmann::json()> provider);
    void unregister_data_source(const std::string& name);
    nlohmann::json get_data(const std::string& source_name);
    std::vector<std::string> get_available_sources() const;
    
    // Data transformation
    void register_transformer(const std::string& name, std::function<nlohmann::json(const nlohmann::json&)> transformer);
    nlohmann::json transform_data(const std::string& transformer_name, const nlohmann::json& data);
    
    // Data caching
    void enable_caching(const std::string& source_name, std::chrono::seconds cache_duration);
    void disable_caching(const std::string& source_name);
    void clear_cache();
    
    // Data validation
    bool validate_data(const std::string& source_name, const nlohmann::json& data);
    std::vector<std::string> get_validation_errors(const std::string& source_name, const nlohmann::json& data);

private:
    std::map<std::string, std::function<nlohmann::json()>> data_providers_;
    std::map<std::string, std::function<nlohmann::json(const nlohmann::json&)>> data_transformers_;
    std::map<std::string, std::pair<nlohmann::json, std::chrono::system_clock::time_point>> data_cache_;
    std::map<std::string, std::chrono::seconds> cache_durations_;
    mutable std::mutex providers_mutex_;
    
    bool is_cache_valid(const std::string& source_name);
    void update_cache(const std::string& source_name, const nlohmann::json& data);
};

// Dashboard analytics
class DashboardAnalytics {
public:
    DashboardAnalytics();
    ~DashboardAnalytics() = default;
    
    // Usage analytics
    std::map<std::string, size_t> get_widget_usage_stats();
    std::map<DashboardTab, std::chrono::seconds> get_tab_usage_stats();
    std::vector<std::string> get_most_used_widgets(size_t limit = 10);
    std::vector<DashboardTab> get_most_used_tabs(size_t limit = 5);
    
    // Performance analytics
    std::map<std::string, std::chrono::milliseconds> get_widget_performance_stats();
    std::chrono::milliseconds get_average_render_time();
    std::chrono::milliseconds get_average_refresh_time();
    std::vector<std::string> get_slow_widgets(size_t limit = 10);
    
    // User behavior analytics
    std::vector<std::string> get_common_interaction_patterns();
    std::map<std::string, double> get_interaction_frequency();
    std::chrono::milliseconds get_average_session_duration();
    
    // Recommendations
    std::vector<std::string> generate_performance_recommendations();
    std::vector<std::string> generate_usability_recommendations();
    std::map<std::string, std::string> generate_widget_optimization_suggestions();

private:
    std::map<std::string, std::vector<std::chrono::milliseconds>> widget_performance_data_;
    std::map<DashboardTab, std::vector<std::chrono::system_clock::time_point>> tab_usage_data_;
    std::vector<std::string> interaction_history_;
    std::vector<std::chrono::milliseconds> render_times_;
    mutable std::mutex analytics_mutex_;
    
    void record_widget_performance(const std::string& widget_id, std::chrono::milliseconds duration);
    void record_tab_usage(DashboardTab tab);
    void record_interaction(const std::string& interaction);
    void record_render_time(std::chrono::milliseconds duration);
};

} // namespace gmail_infinity
