#pragma once

#include <string>
#include <memory>
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <map>
#include <chrono>

#include <boost/asio/io_context.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/ssl.hpp>
#include <nlohmann/json.hpp>

namespace gmail_infinity {

enum class BrowserStatus {
    STOPPED,
    STARTING,
    RUNNING,
    CRASHED,
    TIMEOUT
};

enum class NavigationState {
    IDLE,
    LOADING,
    LOADED,
    ERROR
};

struct BrowserConfig {
    std::string chrome_path;
    std::string user_data_dir;
    std::string profile_dir;
    bool headless = true;
    bool disable_gpu = false;
    bool disable_web_security = false;
    bool ignore_certificate_errors = true;
    std::string window_size = "1920,1080";
    std::string user_agent;
    std::string proxy_server;
    std::map<std::string, std::string> extra_flags;
    
    nlohmann::json to_json() const;
    void from_json(const nlohmann::json& j);
};

struct PageNavigation {
    std::string url;
    std::string referrer;
    std::chrono::milliseconds timeout{30000};
    std::chrono::milliseconds wait_for{5000};
    bool wait_until_load_complete = true;
};

struct ElementSelector {
    std::string css_selector;
    std::string text_content;
    int timeout_ms = 10000;
    bool wait_for_visible = true;
    bool wait_for_clickable = false;
};

struct JavaScriptResult {
    bool success = false;
    std::string result;
    std::string error;
    nlohmann::json json_result;
    
    template<typename T>
    T get_result_as() const {
        try {
            if (json_result.is_null()) {
                return T{};
            }
            return json_result.get<T>();
        } catch (...) {
            return T{};
        }
    }
};

class BrowserController {
public:
    BrowserController();
    ~BrowserController();
    
    // Browser lifecycle
    bool start(const BrowserConfig& config);
    void stop();
    bool restart();
    BrowserStatus get_status() const;
    
    // Navigation
    bool navigate_to(const std::string& url, std::chrono::milliseconds timeout = std::chrono::milliseconds(30000));
    bool wait_for_navigation(std::chrono::milliseconds timeout = std::chrono::milliseconds(30000));
    std::string get_current_url() const;
    std::string get_page_title() const;
    
    // Element interaction
    bool click_element(const std::string& selector, int timeout_ms = 10000);
    bool type_text(const std::string& selector, const std::string& text, bool clear_first = true);
    bool select_option(const std::string& selector, const std::string& value);
    bool scroll_to_element(const std::string& selector);
    bool hover_element(const std::string& selector);
    
    // Element queries
    bool element_exists(const std::string& selector);
    bool element_is_visible(const std::string& selector);
    bool element_is_enabled(const std::string& selector);
    std::string get_element_text(const std::string& selector);
    std::string get_element_attribute(const std::string& selector, const std::string& attribute);
    std::vector<std::string> get_all_elements_text(const std::string& selector);
    
    // JavaScript execution
    JavaScriptResult execute_javascript(const std::string& script);
    JavaScriptResult execute_javascript_async(const std::string& script);
    bool add_init_script(const std::string& script);
    bool remove_init_script(const std::string& script_id);
    
    // Screenshot and content
    bool take_screenshot(const std::string& filename);
    std::string get_page_source();
    std::string get_console_logs(int limit = 100);
    std::vector<std::string> get_network_requests(int limit = 100);
    
    // Cookies and storage
    bool set_cookie(const std::string& name, const std::string& value, const std::string& domain = "");
    std::string get_cookie(const std::string& name);
    bool clear_cookies();
    bool clear_browser_data();
    bool export_storage_state(const std::string& filename);
    
    // Stealth and anti-detection
    bool inject_stealth_scripts();
    bool set_random_user_agent();
    bool randomize_viewport();
    bool inject_canvas_noise();
    bool inject_webgl_noise();
    bool inject_audio_noise();
    bool hide_automation_indicators();
    
    // Proxy management
    bool set_proxy(const std::string& proxy_server);
    bool rotate_proxy();
    std::string get_current_proxy();
    
    // Network control
    bool set_offline_mode(bool offline);
    bool set_network_conditions(const std::string& download_speed, const std::string& upload_speed, int latency);
    bool block_request(const std::string& url_pattern);
    bool unblock_request(const std::string& url_pattern);
    
    // Performance monitoring
    double get_page_load_time();
    std::map<std::string, double> get_performance_metrics();
    std::string get_memory_usage();
    
    // Error handling
    std::string get_last_error() const;
    void clear_error();
    bool has_errors() const;

private:
    // Chrome DevTools Protocol (CDP) connection
    class CDPConnection {
    public:
        CDPConnection();
        ~CDPConnection();
        
        bool connect(const std::string& websocket_url);
        void disconnect();
        bool is_connected() const;
        
        JavaScriptResult send_command(const std::string& method, const nlohmann::json& params = {});
        JavaScriptResult send_command_async(const std::string& method, const nlohmann::json& params = {});
        
        // Event handling
        void register_event_handler(const std::string& event, std::function<void(const nlohmann::json&)> handler);
        void unregister_event_handler(const std::string& event);
        
    private:
        boost::asio::io_context io_context_;
        std::unique_ptr<boost::beast::websocket::stream<boost::beast::tcp_stream>> ws_;
        std::unique_ptr<boost::beast::websocket::stream<boost::asio::ssl::stream<boost::beast::tcp_stream>>> wss_;
        std::atomic<bool> connected_{false};
        std::atomic<int> next_id_{1};
        
        std::map<std::string, std::function<void(const nlohmann::json&)>> event_handlers_;
        mutable std::mutex handlers_mutex_;
        
        std::thread message_handler_thread_;
        std::atomic<bool> running_{false};
        
        void message_handler_loop();
        void handle_message(const std::string& message);
        JavaScriptResult send_command_internal(const std::string& method, const nlohmann::json& params, bool async = false);
    };
    
    // Process management
    class ChromeProcess {
    public:
        ChromeProcess();
        ~ChromeProcess();
        
        bool start(const BrowserConfig& config);
        void stop();
        bool is_running() const;
        int get_pid() const;
        std::string get_debug_port() const;
        
    private:
        int process_id_ = -1;
        std::string debug_port_;
        std::atomic<bool> running_{false};
        
        std::string build_chrome_command(const BrowserConfig& config);
        bool wait_for_debug_port(std::chrono::seconds timeout = std::chrono::seconds(30));
    };
    
    // Member variables
    std::unique_ptr<CDPConnection> cdp_connection_;
    std::unique_ptr<ChromeProcess> chrome_process_;
    BrowserConfig config_;
    BrowserStatus status_{BrowserStatus::STOPPED};
    NavigationState navigation_state_{NavigationState::IDLE};
    
    mutable std::mutex controller_mutex_;
    std::string last_error_;
    std::atomic<int> command_timeout_ms_{30000};
    
    // Stealth scripts
    std::vector<std::string> init_scripts_;
    std::map<std::string, std::string> stealth_scripts_;
    
    // Performance tracking
    std::chrono::steady_clock::time_point navigation_start_time_;
    std::atomic<double> last_page_load_time_{0.0};
    
    // Private methods
    bool initialize_cdp_connection();
    void setup_event_handlers();
    void handle_page_load_events();
    void handle_console_events();
    void handle_network_events();
    
    // Command helpers
    JavaScriptResult navigate_to_internal(const std::string& url);
    JavaScriptResult click_element_internal(const std::string& selector);
    JavaScriptResult type_text_internal(const std::string& selector, const std::string& text, bool clear_first);
    JavaScriptResult execute_script_internal(const std::string& script, bool async = false);
    
    // Stealth script management
    void load_stealth_scripts();
    std::string get_stealth_script(const std::string& script_name) const;
    
    // Utility methods
    std::string generate_random_user_agent() const;
    std::string generate_random_viewport() const;
    std::string wait_for_element_script(const std::string& selector, int timeout_ms) const;
    std::string click_element_script(const std::string& selector) const;
    std::string type_text_script(const std::string& selector, const std::string& text, bool clear_first) const;
    
    // Error handling
    void set_error(const std::string& error);
    void clear_error_internal();
    
    // Logging
    void log_browser_info(const std::string& message) const;
    void log_browser_error(const std::string& message) const;
    void log_browser_debug(const std::string& message) const;
};

// Stealth script injector
class StealthScriptInjector {
public:
    explicit StealthScriptInjector(std::shared_ptr<BrowserController> controller);
    ~StealthScriptInjector() = default;
    
    bool inject_all_stealth_scripts();
    bool inject_canvas_fingerprint_script();
    bool inject_webgl_fingerprint_script();
    bool inject_audio_fingerprint_script();
    bool inject_navigator_script();
    bool inject_screen_script();
    bool inject_timezone_script();
    bool inject_language_script();
    bool inject_permissions_script();
    bool inject_webdriver_script();
    
    // Custom scripts
    bool inject_custom_script(const std::string& script_name, const std::string& script_content);
    
private:
    std::shared_ptr<BrowserController> controller_;
    std::map<std::string, std::string> stealth_scripts_;
    
    void load_stealth_scripts();
    std::string get_canvas_script() const;
    std::string get_webgl_script() const;
    std::string get_audio_script() const;
    std::string get_navigator_script() const;
    std::string get_screen_script() const;
    std::string get_timezone_script() const;
    std::string get_language_script() const;
    std::string get_permissions_script() const;
    std::string get_webdriver_script() const;
};

// Browser factory for creating configured browsers
class BrowserFactory {
public:
    static std::shared_ptr<BrowserController> create_browser(const BrowserConfig& config);
    static std::shared_ptr<BrowserController> create_stealth_browser(const BrowserConfig& config);
    static std::shared_ptr<BrowserController> create_mobile_browser(const BrowserConfig& config);
    static std::shared_ptr<BrowserController> create_tablet_browser(const BrowserConfig& config);
    
    // Configuration builders
    static BrowserConfig build_desktop_config();
    static BrowserConfig build_mobile_config();
    static BrowserConfig build_stealth_config();
    static BrowserConfig build_config_with_proxy(const std::string& proxy_server);
    
private:
    static BrowserConfig create_base_config();
    static void apply_desktop_settings(BrowserConfig& config);
    static void apply_mobile_settings(BrowserConfig& config);
    static void apply_stealth_settings(BrowserConfig& config);
};

} // namespace gmail_infinity
