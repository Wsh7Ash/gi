#include "web_creator.h"
#include "app/app_logger.h"

#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>
#include <thread>
#include <chrono>
#include <random>
#include <filesystem>

namespace gmail_infinity {

using json = nlohmann::json;
namespace fs = std::filesystem;

WebCreator::WebCreator(bool mock_mode) : mock_mode_(mock_mode) {
    std::random_device rd;
    rng_.seed(rd());
    behavior_ = std::make_unique<BehaviorEngine>();
}

WebCreator::~WebCreator() = default;

void WebCreator::set_proxy_manager(std::shared_ptr<ProxyManager> pm) {
    proxy_manager_ = std::move(pm);
}

void WebCreator::set_sms_manager(std::shared_ptr<SMSVerificationManager> sm) {
    sms_manager_ = std::move(sm);
}

WebCreationResult WebCreator::create_account(const WebCreationConfig& cfg) {
    WebCreationResult result;

    if (mock_mode_) {
        result.success = true;
        result.email   = "web.mock@gmail.com";
        result.message = "Mock web creation";
        return result;
    }

    try {
        // WebCreator delegates to GmailCreator for the actual flow
        GmailCreator creator(false);
        creator.set_proxy_manager(proxy_manager_);
        creator.set_sms_manager(sms_manager_);

        GmailCreationConfig gmail_cfg;
        gmail_cfg.persona       = cfg.persona;
        gmail_cfg.password      = cfg.password;
        gmail_cfg.phone_country = cfg.phone_country;
        gmail_cfg.recovery_email = cfg.recovery_email;
        gmail_cfg.browser_executable = cfg.browser_executable;

        auto creation_result = creator.create_account(gmail_cfg);
        result.success  = creation_result.success;
        result.email    = creation_result.email;
        result.password = creation_result.password;
        result.message  = creation_result.message;

    } catch (const std::exception& e) {
        result.message = "WebCreator exception: " + std::string(e.what());
        spdlog::error("WebCreator: {}", e.what());
    }
    return result;
}

} // namespace gmail_infinity
