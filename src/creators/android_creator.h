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

#include <nlohmann/json.hpp>

#include "gmail_creator.h"
#include "../identity/persona_generator.h"
#include "../core/proxy_manager.h"
#include "../core/fingerprint_generator.h"
#include "../verification/sms_providers.h"
#include "../verification/captcha_solver.h"
#include "../verification/email_recovery.h"
#include "../verification/voice_verification.h"

namespace gmail_infinity {

enum class AndroidCreationStep {
    NOT_STARTED,
    LAUNCHING_EMULATOR,
    CONFIGURING_EMULATOR,
    INSTALLING_APPS,
    OPENING_GMAIL_APP,
    INITIATING_SIGNUP,
    ENTERING_PERSONAL_INFO,
    SELECTING_USERNAME,
    SETTING_PASSWORD,
    PHONE_VERIFICATION,
    EMAIL_VERIFICATION,
    ACCOUNT_SETUP,
    SYNCING_ACCOUNT,
    SUCCESS,
    FAILED
};

enum class AndroidEmulatorType {
    ANDROID_STUDIO_EMULATOR,
    GENYMOTION,
    BLUESTACKS,
    NOX_PLAYER,
    MEMU_PLAY,
    LDPLAYER,
    CUSTOM_EMULATOR
};

enum class AndroidVersion {
    ANDROID_10,
    ANDROID_11,
    ANDROID_12,
    ANDROID_13,
    ANDROID_14,
    LATEST
};

struct AndroidEmulatorConfig {
    AndroidEmulatorType emulator_type = AndroidEmulatorType::ANDROID_STUDIO_EMULATOR;
    AndroidVersion android_version = AndroidVersion::ANDROID_13;
    std::string device_name = "Pixel_4a";
    std::string avd_name;
    std::string system_image;
    int screen_width = 1080;
    int screen_height = 2340;
    int screen_density = 420;
    int ram_mb = 4096;
    int storage_mb = 8192;
    bool enable_google_play = true;
    std::string proxy_server;
    std::map<std::string, std::string> custom_properties;
    
    nlohmann::json to_json() const;
    void from_json(const nlohmann::json& j);
};

struct AndroidCreationConfig {
    // Emulator settings
    AndroidEmulatorConfig emulator_config;
    bool headless_mode = true;
    bool enable_gpu_acceleration = false;
    std::chrono::seconds emulator_startup_timeout{120};
    
    // Creation settings
    std::chrono::seconds step_timeout{60};
    std::chrono::seconds total_creation_timeout{900};
    int max_retries = 3;
    std::chrono::seconds retry_delay{60};
    
    // App configuration
    std::string gmail_app_version = "latest";
    bool auto_update_apps = false;
    bool skip_google_services_setup = false;
    
    // Verification preferences
    bool prefer_phone_verification = true;
    bool enable_voice_verification = false;
    std::string preferred_country_code = "+1";
    
    // Device fingerprinting
    bool randomize_device_id = true;
    bool randomize_advertising_id = true;
    bool randomize_android_id = true;
    bool randomize_imei = false;
    
    // Network configuration
    bool use_mobile_network = true;
    bool enable_wifi = false;
    std::string network_operator = "T-Mobile";
    std::string sim_country = "US";
    
    // Location settings
    bool enable_location_services = false;
    double latitude = 37.7749; // San Francisco
    double longitude = -122.4194;
    
    // Security settings
    bool enable_screen_lock = false;
    std::string lock_type = "pattern";
    bool enable_biometric = false;
    
    nlohmann::json to_json() const;
    void from_json(const nlohmann::json& j);
};

struct AndroidCreationProgress {
    AndroidCreationStep current_step;
    std::chrono::system_clock::time_point step_started_at;
    std::chrono::milliseconds step_duration;
    std::string step_description;
    std::string current_activity;
    std::string current_package;
    bool success;
    std::string error_message;
    std::map<std::string, std::string> step_data;
    
    nlohmann::json to_json() const;
    void from_json(const nlohmann::json& j);
};

struct AndroidCreationResult {
    bool success;
    std::string email_address;
    std::string password;
    std::string recovery_email;
    std::string phone_number;
    HumanPersona persona_used;
    std::string emulator_id;
    std::string device_id;
    std::string android_id;
    std::chrono::system_clock::time_point created_at;
    std::chrono::milliseconds total_creation_time;
    std::vector<AndroidCreationProgress> progress_history;
    std::string error_details;
    std::map<std::string, std::string> device_info;
    std::map<std::string, std::string> metadata;
    
    nlohmann::json to_json() const;
    void from_json(const nlohmann::json& j);
};

// Android device interface
class AndroidDeviceInterface {
public:
    virtual ~AndroidDeviceInterface() = default;
    
    // Device management
    virtual bool initialize(const AndroidEmulatorConfig& config) = 0;
    virtual bool start_emulator() = 0;
    virtual bool stop_emulator() = 0;
    virtual bool is_emulator_running() = 0;
    virtual std::string get_emulator_id() const = 0;
    
    // App management
    virtual bool install_app(const std::string& apk_path) = 0;
    virtual bool uninstall_app(const std::string& package_name) = 0;
    virtual bool launch_app(const std::string& package_name) = 0;
    virtual bool close_app(const std::string& package_name) = 0;
    virtual bool is_app_installed(const std::string& package_name) = 0;
    
    // Device interaction
    virtual bool click_element(const std::string& selector) = 0;
    virtual bool type_text(const std::string& selector, const std::string& text) = 0;
    virtual bool scroll_view(const std::string& direction, int distance) = 0;
    virtual bool swipe_gesture(int x1, int y1, int x2, int y2, int duration_ms) = 0;
    virtual bool press_key(int keycode) = 0;
    
    // Screen and content
    virtual bool take_screenshot(const std::string& filename) = 0;
    virtual std::string get_screen_content() = 0;
    virtual std::string get_current_activity() = 0;
    virtual std::string get_current_package() = 0;
    
    // Device information
    virtual std::map<std::string, std::string> get_device_info() = 0;
    virtual std::string get_device_id() = 0;
    virtual std::string get_android_id() = 0;
    virtual std::string get_serial_number() = 0;
    
    // Network and location
    virtual bool set_network_state(bool wifi_enabled, bool mobile_enabled) = 0;
    virtual bool set_gps_location(double latitude, double longitude) = 0;
    virtual bool set_proxy(const std::string& proxy_server) = 0;
    
    // Security and fingerprinting
    virtual bool set_device_id(const std::string& device_id) = 0;
    virtual bool set_android_id(const std::string& android_id) = 0;
    virtual bool set_advertising_id(const std::string& advertising_id) = 0;
    virtual bool set_imei(const std::string& imei) = 0;

protected:
    AndroidEmulatorConfig config_;
    std::string emulator_id_;
    mutable std::mutex device_mutex_;
    
    // ADB helpers
    std::string execute_adb_command(const std::string& command);
    std::string execute_adb_shell_command(const std::string& command);
    bool wait_for_device(std::chrono::seconds timeout = std::chrono::seconds(60));
    bool wait_for_boot_complete(std::chrono::seconds timeout = std::chrono::seconds(120));
};

// Android Studio Emulator implementation
class AndroidStudioEmulator : public AndroidDeviceInterface {
public:
    bool initialize(const AndroidEmulatorConfig& config) override;
    bool start_emulator() override;
    bool stop_emulator() override;
    bool is_emulator_running() override;
    std::string get_emulator_id() const override { return emulator_id_; }
    
    bool install_app(const std::string& apk_path) override;
    bool uninstall_app(const std::string& package_name) override;
    bool launch_app(const std::string& package_name) override;
    bool close_app(const std::string& package_name) override;
    bool is_app_installed(const std::string& package_name) override;
    
    bool click_element(const std::string& selector) override;
    bool type_text(const std::string& selector, const std::string& text) override;
    bool scroll_view(const std::string& direction, int distance) override;
    bool swipe_gesture(int x1, int y1, int x2, int y2, int duration_ms) override;
    bool press_key(int keycode) override;
    
    bool take_screenshot(const std::string& filename) override;
    std::string get_screen_content() override;
    std::string get_current_activity() override;
    std::string get_current_package() override;
    
    std::map<std::string, std::string> get_device_info() override;
    std::string get_device_id() override;
    std::string get_android_id() override;
    std::string get_serial_number() override;
    
    bool set_network_state(bool wifi_enabled, bool mobile_enabled) override;
    bool set_gps_location(double latitude, double longitude) override;
    bool set_proxy(const std::string& proxy_server) override;
    
    bool set_device_id(const std::string& device_id) override;
    bool set_android_id(const std::string& android_id) override;
    bool set_advertising_id(const std::string& advertising_id) override;
    bool set_imei(const std::string& imei) override;

private:
    bool create_avd();
    bool start_avd();
    std::string get_emulator_command();
    std::string get_avd_name();
    bool wait_for_emulator_ready();
};

class AndroidCreator {
public:
    AndroidCreator();
    ~AndroidCreator();
    
    // Initialization
    bool initialize(const AndroidCreationConfig& config);
    void shutdown();
    
    // Core creation methods
    std::future<AndroidCreationResult> create_account_async();
    AndroidCreationResult create_account_sync();
    std::future<AndroidCreationResult> create_account_with_persona_async(const HumanPersona& persona);
    AndroidCreationResult create_account_with_persona(const HumanPersona& persona);
    
    // Step-by-step creation
    AndroidCreationProgress launch_emulator();
    AndroidCreationProgress configure_emulator();
    AndroidCreationProgress install_required_apps();
    AndroidCreationProgress open_gmail_app();
    AndroidCreationProgress initiate_signup();
    AndroidCreationProgress enter_personal_information(const HumanPersona& persona);
    AndroidCreationProgress select_username(const std::string& preferred_username);
    AndroidCreationProgress set_password(const std::string& password);
    AndroidCreationProgress complete_phone_verification();
    AndroidCreationProgress complete_email_verification();
    AndroidCreationProgress complete_account_setup();
    AndroidCreationProgress sync_account();
    
    // Progress monitoring
    AndroidCreationProgress get_current_progress() const;
    std::vector<AndroidCreationProgress> get_progress_history() const;
    bool is_creation_complete() const;
    void set_progress_callback(std::function<void(const AndroidCreationProgress&)> callback);
    
    // Configuration
    void update_config(const AndroidCreationConfig& config);
    AndroidCreationConfig get_config() const;
    
    // Device management
    bool create_custom_emulator(const AndroidEmulatorConfig& emulator_config);
    bool delete_emulator(const std::string& emulator_id);
    std::vector<std::string> get_available_emulators();
    bool switch_emulator(const std::string& emulator_id);
    
    // Advanced features
    bool take_screenshot_at_step(AndroidCreationStep step, const std::string& filename = "");
    bool enable_ui_automator_logging(bool enable);
    std::vector<std::string> get_ui_hierarchy(AndroidCreationStep step);
    
    // Statistics
    std::map<AndroidCreationStep, std::chrono::milliseconds> get_average_step_times() const;
    std::map<AndroidEmulatorType, double> get_emulator_success_rates() const;
    double get_overall_success_rate() const;
    std::chrono::milliseconds get_average_creation_time() const;
    
    // History and logging
    std::vector<AndroidCreationResult> get_creation_history(size_t limit = 100);
    bool export_creation_history(const std::string& filename) const;
    bool export_screenshots(const std::string& directory) const;

private:
    // Core components
    std::unique_ptr<AndroidDeviceInterface> device_;
    std::shared_ptr<ProxyManager> proxy_manager_;
    std::shared_ptr<FingerprintCache> fingerprint_cache_;
    std::shared_ptr<SMSVerificationManager> sms_manager_;
    std::shared_ptr<CaptchaSolver> captcha_solver_;
    std::shared_ptr<EmailRecoveryManager> email_manager_;
    std::shared_ptr<VoiceVerificationManager> voice_manager_;
    
    // Configuration
    AndroidCreationConfig config_;
    
    // Creation state
    std::atomic<AndroidCreationStep> current_step_{AndroidCreationStep::NOT_STARTED};
    AndroidCreationProgress current_progress_;
    std::vector<AndroidCreationProgress> progress_history_;
    std::shared_ptr<HumanPersona> current_persona_;
    
    // Statistics
    std::map<AndroidCreationStep, std::vector<std::chrono::milliseconds>> step_times_;
    std::map<AndroidEmulatorType, std::vector<bool>> emulator_results_;
    std::atomic<size_t> total_attempts_{0};
    std::atomic<size_t> successful_creations_{0};
    std::atomic<std::chrono::milliseconds::rep> total_creation_time_ms_{0};
    
    // Creation history
    std::vector<AndroidCreationResult> creation_history_;
    mutable std::mutex history_mutex_;
    
    // Debug features
    std::map<AndroidCreationStep, std::string> screenshots_;
    std::map<AndroidCreationStep, std::vector<std::string>> ui_hierarchies_;
    bool ui_automator_logging_enabled_{false};
    
    // Callbacks
    std::function<void(const AndroidCreationProgress&)> progress_callback_;
    
    // Thread safety
    mutable std::mutex creator_mutex_;
    std::atomic<bool> creation_in_progress_{false};
    
    // Private methods
    AndroidCreationResult execute_creation_workflow();
    AndroidCreationResult execute_creation_with_persona(const HumanPersona& persona);
    
    // Step implementations
    AndroidCreationProgress execute_launch_emulator();
    AndroidCreationProgress execute_configure_emulator();
    AndroidCreationProgress execute_install_required_apps();
    AndroidCreationProgress execute_open_gmail_app();
    AndroidCreationProgress execute_initiate_signup();
    AndroidCreationProgress execute_enter_personal_information(const HumanPersona& persona);
    AndroidCreationProgress execute_select_username(const std::string& preferred_username);
    AndroidCreationProgress execute_set_password(const std::string& password);
    AndroidCreationProgress execute_complete_phone_verification();
    AndroidCreationProgress execute_complete_email_verification();
    AndroidCreationProgress execute_complete_account_setup();
    AndroidCreationProgress execute_sync_account();
    
    // Helper methods
    bool setup_emulator_environment();
    bool setup_device_fingerprint();
    HumanPersona generate_or_use_persona();
    std::string generate_username_from_persona(const HumanPersona& persona);
    std::string generate_secure_password();
    bool install_gmail_app();
    bool handle_app_permissions();
    bool simulate_human_interaction();
    
    // App interaction helpers
    bool wait_for_app_launch(const std::string& package_name, int timeout_ms = 30000);
    bool wait_for_element_and_click(const std::string& selector, int timeout_ms = 10000);
    bool wait_for_element_and_type(const std::string& selector, const std::string& text, bool clear_first = true, int timeout_ms = 10000);
    bool wait_for_element_and_select(const std::string& selector, const std::string& value, int timeout_ms = 10000);
    std::string get_element_text(const std::string& selector, int timeout_ms = 10000);
    
    // Android-specific helpers
    bool handle_android_permissions();
    bool handle_google_play_services();
    bool setup_device_security();
    bool configure_network_settings();
    
    // Progress tracking
    void update_progress(AndroidCreationStep step, const std::string& description = "");
    void update_progress_with_error(AndroidCreationStep step, const std::string& error);
    void record_step_time(AndroidCreationStep step, std::chrono::milliseconds duration);
    
    // Debug and logging
    void capture_screenshot(AndroidCreationStep step);
    void capture_ui_hierarchy(AndroidCreationStep step);
    
    // Statistics tracking
    void update_statistics(bool success, std::chrono::milliseconds total_time);
    void record_emulator_result(AndroidEmulatorType type, bool success);
    
    // Utility methods
    std::string step_to_string(AndroidCreationStep step) const;
    std::string emulator_type_to_string(AndroidEmulatorType type) const;
    std::chrono::milliseconds get_step_timeout(AndroidCreationStep step) const;
    std::string generate_creation_id() const;
    
    // Device fingerprinting
    std::string generate_random_device_id();
    std::string generate_random_android_id();
    std::string generate_random_advertising_id();
    std::string generate_random_imei();
    
    // Logging
    void log_creator_info(const std::string& message) const;
    void log_creator_error(const std::string& message) const;
    void log_creator_debug(const std::string& message) const;
};

// Android emulator factory
class AndroidEmulatorFactory {
public:
    static std::unique_ptr<AndroidDeviceInterface> create_emulator(AndroidEmulatorType type);
    static std::vector<AndroidEmulatorType> get_available_emulators();
    static AndroidEmulatorType get_best_emulator_for_system();
    static bool is_emulator_available(AndroidEmulatorType type);
    
private:
    static bool check_android_studio_availability();
    static bool check_genymotion_availability();
    static bool check_bluestacks_availability();
};

// Android creation analyzer
class AndroidCreationAnalyzer {
public:
    AndroidCreationAnalyzer();
    ~AndroidCreationAnalyzer() = default;
    
    // Analysis methods
    std::map<AndroidCreationStep, double> analyze_step_success_rates(const std::vector<AndroidCreationResult>& results);
    std::map<AndroidEmulatorType, double> analyze_emulator_performance(const std::vector<AndroidCreationResult>& results);
    std::vector<std::string> identify_common_failures(const std::vector<AndroidCreationResult>& results);
    
    // Performance analysis
    std::chrono::milliseconds calculate_average_creation_time(const std::vector<AndroidCreationResult>& results);
    std::map<AndroidCreationStep, std::chrono::milliseconds> calculate_average_step_times(const std::vector<AndroidCreationResult>& results);
    std::vector<std::string> identify_performance_bottlenecks(const std::vector<AndroidCreationResult>& results);
    
    // Recommendations
    std::vector<std::string> generate_optimization_recommendations(const std::vector<AndroidCreationResult>& results);
    std::map<std::string, std::string> suggest_config_improvements(const std::vector<AndroidCreationResult>& results);

private:
    std::map<AndroidCreationStep, std::string> step_descriptions_;
    std::map<AndroidEmulatorType, std::string> emulator_descriptions_;
    
    void initialize_descriptions();
    std::vector<AndroidCreationResult> filter_successful_results(const std::vector<AndroidCreationResult>& results);
    std::vector<AndroidCreationResult> filter_failed_results(const std::vector<AndroidCreationResult>& results);
};

} // namespace gmail_infinity
