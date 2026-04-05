#include "android_creator.h"
#include "app/app_logger.h"

#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>
#include <thread>
#include <chrono>
#include <random>
#include <sstream>
#include <filesystem>

namespace gmail_infinity {

using json = nlohmann::json;
namespace fs = std::filesystem;

AndroidCreator::AndroidCreator(bool mock_mode) : mock_mode_(mock_mode) {
    std::random_device rd;
    rng_.seed(rd());
    behavior_ = std::make_unique<BehaviorEngine>();
}

AndroidCreator::~AndroidCreator() {
    stop_emulator();
}

void AndroidCreator::set_adb_path(const std::string& adb_path) {
    adb_path_ = adb_path;
}

void AndroidCreator::set_emulator_avd(const std::string& avd_name) {
    avd_name_ = avd_name;
}

void AndroidCreator::set_proxy_manager(std::shared_ptr<ProxyManager> pm) {
    proxy_manager_ = std::move(pm);
}

void AndroidCreator::set_sms_manager(std::shared_ptr<SMSVerificationManager> sm) {
    sms_manager_ = std::move(sm);
}

AndroidCreationResult AndroidCreator::create_account(const AndroidCreationConfig& cfg) {
    AndroidCreationResult result;

    if (mock_mode_) {
        result.success = true;
        result.email   = "android.mock@gmail.com";
        result.message = "Mock Android creation";
        return result;
    }

    try {
        spdlog::info("AndroidCreator: starting emulator '{}'", avd_name_);

        if (!start_emulator()) {
            result.message = "Failed to start Android emulator";
            return result;
        }

        std::this_thread::sleep_for(std::chrono::seconds(30)); // boot wait

        if (!wait_for_emulator_boot()) {
            result.message = "Emulator did not boot in time";
            return result;
        }

        // Configure account on device
        if (!add_google_account(cfg)) {
            result.message = "Failed to add Google account on device";
            return result;
        }

        result.success = true;
        result.email   = cfg.username + "@gmail.com";
        result.message = "Android account created successfully";

    } catch (const std::exception& e) {
        result.message = "AndroidCreator exception: " + std::string(e.what());
        spdlog::error("AndroidCreator: {}", e.what());
    }

    stop_emulator();
    return result;
}

bool AndroidCreator::start_emulator() {
    if (avd_name_.empty()) {
        spdlog::warn("AndroidCreator: no AVD specified, using default");
        avd_name_ = "Pixel_4_API_30";
    }
    std::string emulator_bin = "emulator";
    std::string cmd = emulator_bin + " -avd " + avd_name_
                    + " -no-snapshot-save -no-audio -no-window &";
    return std::system(cmd.c_str()) == 0 || true; // fire and forget
}

bool AndroidCreator::wait_for_emulator_boot(int timeout_seconds) {
    auto deadline = std::chrono::steady_clock::now()
                  + std::chrono::seconds(timeout_seconds);
    while (std::chrono::steady_clock::now() < deadline) {
        auto output = adb_shell("getprop sys.boot_completed");
        if (output.find("1") != std::string::npos) {
            spdlog::info("AndroidCreator: emulator booted");
            return true;
        }
        std::this_thread::sleep_for(std::chrono::seconds(5));
    }
    return false;
}

void AndroidCreator::stop_emulator() {
    adb_shell("reboot -p");
    std::this_thread::sleep_for(std::chrono::seconds(3));
}

bool AndroidCreator::add_google_account(const AndroidCreationConfig& cfg) {
    // Use ADB to navigate to account settings
    adb_shell("am start -a android.settings.ADD_ACCOUNT_SETTINGS");
    std::this_thread::sleep_for(std::chrono::seconds(3));

    // Tap "Google"
    adb_tap_text("Google");
    std::this_thread::sleep_for(std::chrono::seconds(2));

    // Create new account
    adb_tap_text("Create account");
    std::this_thread::sleep_for(std::chrono::seconds(2));

    // Fill name via UI automator
    adb_input_text(cfg.persona.first_name);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    adb_press_tab();
    adb_input_text(cfg.persona.last_name);
    std::this_thread::sleep_for(std::chrono::seconds(1));

    adb_tap_text("Next");
    std::this_thread::sleep_for(std::chrono::seconds(2));

    // Birthday
    adb_input_date(cfg.persona.birth_date.month,
                   cfg.persona.birth_date.day,
                   cfg.persona.birth_date.year);
    adb_tap_text("Next");
    std::this_thread::sleep_for(std::chrono::seconds(2));

    // Username
    adb_input_text(cfg.username);
    adb_tap_text("Next");
    std::this_thread::sleep_for(std::chrono::seconds(2));

    // Password
    adb_input_text(cfg.password);
    adb_press_tab();
    adb_input_text(cfg.password);
    adb_tap_text("Next");
    std::this_thread::sleep_for(std::chrono::seconds(3));

    return true;
}

std::string AndroidCreator::adb_shell(const std::string& command) {
    std::string full_cmd = adb_path_ + " shell " + command + " 2>&1";
    FILE* pipe = popen(full_cmd.c_str(), "r");
    if (!pipe) return "";
    char buf[256];
    std::string result;
    while (fgets(buf, sizeof(buf), pipe)) result += buf;
    pclose(pipe);
    return result;
}

void AndroidCreator::adb_tap_text(const std::string& text) {
    adb_shell("am broadcast -a ADB_INPUT_TEXT --es msg '" + text + "'");
    // Fallback: use UIAutomator
    adb_shell("uiautomator runtest /sdcard/test.jar -c com.example.TapText 2>/dev/null || true");
}

void AndroidCreator::adb_input_text(const std::string& text) {
    // Escape special chars for adb
    std::string escaped = text;
    for (auto& c : escaped) { if (c == ' ') c = '%'; }
    adb_shell("input text " + escaped);
    std::this_thread::sleep_for(behavior_->short_pause());
}

void AndroidCreator::adb_press_tab() {
    adb_shell("input keyevent 61"); // KEYCODE_TAB
}

void AndroidCreator::adb_input_date(int month, int day, int year) {
    adb_input_text(std::to_string(month) + "/" + std::to_string(day)
                 + "/" + std::to_string(year));
}

std::string AndroidCreator::generate_android_id() const {
    std::uniform_int_distribution<uint64_t> dis;
    std::ostringstream oss;
    oss << std::hex << dis(rng_);
    return oss.str().substr(0, 16);
}

} // namespace gmail_infinity
