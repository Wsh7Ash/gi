#include "recovery_link_creator.h"
#include "app/app_logger.h"

#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>
#include <thread>
#include <chrono>
#include <random>
#include <future>
#include <regex>

namespace gmail_infinity {

using json = nlohmann::json;

RecoveryLinkCreator::RecoveryLinkCreator() {
    std::random_device rd;
    rng_.seed(rd());
}

RecoveryLinkCreator::~RecoveryLinkCreator() {
    shutdown();
}

bool RecoveryLinkCreator::initialize(const RecoveryLinkConfig& config) {
    std::lock_guard<std::mutex> lock(creator_mutex_);
    config_ = config;
    current_step_ = RecoveryLinkStep::NOT_STARTED;
    return true;
}

void RecoveryLinkCreator::shutdown() {
    if (browser_) {
        browser_->stop();
    }
    creation_in_progress_ = false;
}

std::future<RecoveryLinkResult> RecoveryLinkCreator::create_recovery_link_async() {
    return std::async(std::launch::async, [this]() {
        return create_recovery_link_sync();
    });
}

RecoveryLinkResult RecoveryLinkCreator::create_recovery_link_sync() {
    if (creation_in_progress_.exchange(true)) {
        RecoveryLinkResult result;
        result.success = false;
        result.error_details = "Recovery link creation already in progress";
        return result;
    }

    auto start_time = std::chrono::system_clock::now();
    RecoveryLinkResult result = execute_creation_workflow();
    auto end_time = std::chrono::system_clock::now();

    result.total_creation_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    creation_in_progress_ = false;
    
    update_statistics(result.success, result.total_creation_time);
    return result;
}

RecoveryLinkResult RecoveryLinkCreator::execute_creation_workflow() {
    RecoveryLinkResult result;
    result.success = false;
    result.target_email = config_.target_email;

    try {
        // Step 1: Launch Browser
        auto launch_progress = launch_browser();
        if (!launch_progress.success) {
            result.error_details = launch_progress.error_message;
            return result;
        }

        // Step 2: Navigate to Recovery
        auto nav_progress = navigate_to_recovery_page();
        if (!nav_progress.success) {
            result.error_details = nav_progress.error_message;
            return result;
        }

        // Step 3: Enter Target Email
        auto email_progress = enter_target_email(config_.target_email);
        if (!email_progress.success) {
            result.error_details = email_progress.error_message;
            return result;
        }

        // Step 4: Select method (defaulting to primary in config)
        auto method_progress = select_recovery_method(config_.primary_method);
        if (!method_progress.success) {
            // Try fallbacks if enabled
            if (config_.enable_fallback_methods) {
                for (auto m : config_.preferred_methods) {
                    if (m == config_.primary_method) continue;
                    method_progress = select_recovery_method(m);
                    if (method_progress.success) break;
                }
            }
            if (!method_progress.success) {
                result.error_details = "Failed to select a valid recovery method";
                return result;
            }
        }

        // Final result construction
        result.success = true;
        result.recovery_link = extract_recovery_link_from_page();
        result.link_token = generate_link_token();
        result.created_at = std::chrono::system_clock::now();
        result.expires_at = result.created_at + config_.link_expiry;
        result.new_password = config_.generate_new_password ? generate_new_password() : config_.custom_password;

    } catch (const std::exception& e) {
        result.error_details = std::string("Exception during recovery link creation: ") + e.what();
    }

    return result;
}

RecoveryLinkProgress RecoveryLinkCreator::launch_browser() {
    update_progress(RecoveryLinkStep::LAUNCHING_BROWSER, "Launching browser for Recovery Link creation");
    
    if (!setup_browser_environment()) {
        update_progress_with_error(RecoveryLinkStep::LAUNCHING_BROWSER, "Failed to setup browser environment");
        return current_progress_;
    }

    current_progress_.success = true;
    return current_progress_;
}

RecoveryLinkProgress RecoveryLinkCreator::navigate_to_recovery_page() {
    update_progress(RecoveryLinkStep::NAVIGATING_TO_RECOVERY, "Navigating to Google account recovery page");
    
    if (!browser_->navigate_to("https://accounts.google.com/signin/recovery")) {
        update_progress_with_error(RecoveryLinkStep::NAVIGATING_TO_RECOVERY, "Failed to navigate to recovery page");
        return current_progress_;
    }

    current_progress_.success = true;
    return current_progress_;
}

RecoveryLinkProgress RecoveryLinkCreator::enter_target_email(const std::string& email) {
    update_progress(RecoveryLinkStep::ENTERING_EMAIL, "Entering target email: " + email);
    
    if (!wait_for_element_and_type("input[type='email']", email)) {
        update_progress_with_error(RecoveryLinkStep::ENTERING_EMAIL, "Failed to type email address");
        return current_progress_;
    }
    
    wait_for_element_and_click("#identifierNext");
    std::this_thread::sleep_for(std::chrono::seconds(2));

    current_progress_.success = true;
    return current_progress_;
}

RecoveryLinkProgress RecoveryLinkCreator::select_recovery_method(RecoveryMethod method) {
    update_progress(RecoveryLinkStep::SELECTING_RECOVERY_METHOD, "Selecting recovery method: " + method_to_string(method));
    
    // In a real implementation, we'd look for specific UI elements per method
    // For this rewrite, we simulate the selection and assume success if the element exists
    std::string selector;
    switch (method) {
        case RecoveryMethod::PHONE_VERIFICATION: selector = "[data-challengetype='1']"; break;
        case RecoveryMethod::EMAIL_VERIFICATION: selector = "[data-challengetype='12']"; break;
        case RecoveryMethod::SECURITY_QUESTIONS: selector = "[data-challengetype='13']"; break;
        default: selector = "div[role='link']"; break;
    }
    
    if (browser_->element_exists(selector)) {
        browser_->click_element(selector);
        current_progress_.success = true;
    } else {
        update_progress_with_error(RecoveryLinkStep::SELECTING_RECOVERY_METHOD, "Method selector not found");
    }

    return current_progress_;
}

// Helper implementations
bool RecoveryLinkCreator::setup_browser_environment() {
    BrowserConfig b_config;
    b_config.headless = config_.headless;
    browser_ = BrowserFactory::create_stealth_browser(b_config);
    return browser_ != nullptr;
}

bool RecoveryLinkCreator::wait_for_element_and_click(const std::string& selector, int timeout_ms) {
    return browser_->click_element(selector, timeout_ms);
}

bool RecoveryLinkCreator::wait_for_element_and_type(const std::string& selector, const std::string& text, bool clear_first, int timeout_ms) {
    return browser_->type_text(selector, text, clear_first);
}

void RecoveryLinkCreator::update_progress(RecoveryLinkStep step, const std::string& description) {
    std::lock_guard<std::mutex> lock(creator_mutex_);
    current_step_ = step;
    current_progress_.current_step = step;
    current_progress_.step_description = description;
    current_progress_.step_started_at = std::chrono::system_clock::now();
    current_progress_.success = false;
    progress_history_.push_back(current_progress_);
    
    spdlog::info("[RecoveryLinkCreator] {}", description);
}

void RecoveryLinkCreator::update_progress_with_error(RecoveryLinkStep step, const std::string& error) {
    std::lock_guard<std::mutex> lock(creator_mutex_);
    current_progress_.success = false;
    current_progress_.error_message = error;
    spdlog::error("[RecoveryLinkCreator] Error at step {}: {}", step_to_string(step), error);
}

void RecoveryLinkCreator::update_statistics(bool success, std::chrono::milliseconds total_time) {
    total_attempts_++;
    if (success) successful_creations_++;
    total_creation_time_ms_ += total_time.count();
}

std::string RecoveryLinkCreator::generate_link_token() {
    const std::string chars = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    std::string token;
    for (int i = 0; i < 32; ++i) {
        token += chars[std::uniform_int_distribution<int>(0, chars.size() - 1)(rng_)];
    }
    return token;
}

std::string RecoveryLinkCreator::generate_new_password() {
    const std::string chars = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz!@#$%^&*()";
    std::string pwd;
    for (int i = 0; i < config_.password_length; ++i) {
        pwd += chars[std::uniform_int_distribution<int>(0, chars.size() - 1)(rng_)];
    }
    return pwd;
}

std::string RecoveryLinkCreator::extract_recovery_link_from_page() {
    // Simulated extraction logic - in real would use regex on page source or get current URL
    return browser_->get_current_url();
}

std::string RecoveryLinkCreator::step_to_string(RecoveryLinkStep step) const {
    switch (step) {
        case RecoveryLinkStep::NOT_STARTED: return "NOT_STARTED";
        case RecoveryLinkStep::LAUNCHING_BROWSER: return "LAUNCHING_BROWSER";
        case RecoveryLinkStep::NAVIGATING_TO_RECOVERY: return "NAVIGATING_TO_RECOVERY";
        case RecoveryLinkStep::ENTERING_EMAIL: return "ENTERING_EMAIL";
        case RecoveryLinkStep::SELECTING_RECOVERY_METHOD: return "SELECTING_RECOVERY_METHOD";
        case RecoveryLinkStep::SUCCESS: return "SUCCESS";
        case RecoveryLinkStep::FAILED: return "FAILED";
        default: return "UNKNOWN";
    }
}

std::string RecoveryLinkCreator::method_to_string(RecoveryMethod method) const {
    switch (method) {
        case RecoveryMethod::PHONE_VERIFICATION: return "PHONE_VERIFICATION";
        case RecoveryMethod::EMAIL_VERIFICATION: return "EMAIL_VERIFICATION";
        case RecoveryMethod::SECURITY_QUESTIONS: return "SECURITY_QUESTIONS";
        case RecoveryMethod::BACKUP_CODES: return "BACKUP_CODES";
        case RecoveryMethod::RECOVERY_EMAIL: return "RECOVERY_EMAIL";
        case RecoveryMethod::ACCOUNT_RECOVERY_FORM: return "ACCOUNT_RECOVERY_FORM";
        default: return "UNKNOWN";
    }
}

// JSON implementations
nlohmann::json RecoveryLinkConfig::to_json() const {
    return nlohmann::json{
        {"target_email", target_email},
        {"recovery_email", recovery_email},
        {"primary_method", static_cast<int>(primary_method)}
    };
}

} // namespace gmail_infinity
