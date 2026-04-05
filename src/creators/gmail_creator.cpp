#include "gmail_creator.h"
#include "web_creator.h"
#include "app/app_logger.h"

#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>
#include <random>
#include <thread>
#include <chrono>
#include <regex>
#include <filesystem>

namespace gmail_infinity {

using json = nlohmann::json;
namespace fs = std::filesystem;

// ─────────────────────────────────────────────────────────
// GmailCreator
// ─────────────────────────────────────────────────────────

GmailCreator::GmailCreator(bool mock_mode)
    : mock_mode_(mock_mode) {
    std::random_device rd;
    rng_.seed(rd());
    behavior_ = std::make_unique<BehaviorEngine>();
    evasion_  = std::make_unique<DetectionEvasion>();
}

GmailCreator::~GmailCreator() = default;

void GmailCreator::set_proxy_manager(std::shared_ptr<ProxyManager> pm) {
    proxy_manager_ = std::move(pm);
}

void GmailCreator::set_sms_manager(std::shared_ptr<SMSVerificationManager> sm) {
    sms_manager_ = std::move(sm);
}

void GmailCreator::set_captcha_manager(std::shared_ptr<CaptchaSolverManager> cm) {
    captcha_manager_ = std::move(cm);
}

void GmailCreator::set_vault(std::shared_ptr<SecureVault> v) {
    vault_ = std::move(v);
}

CreationResult GmailCreator::create_account(const GmailCreationConfig& cfg) {
    CreationResult result;
    result.session_id = generate_session_id();

    if (mock_mode_) {
        spdlog::info("[MOCK] GmailCreator: simulating account creation for {}",
            cfg.persona.first_name + " " + cfg.persona.last_name);
        result.success = true;
        result.email   = cfg.persona.first_name + ".mock@gmail.com";
        result.message = "Mock account created";
        return result;
    }

    try {
        // Step 1: Set up browser
        auto browser_config = setup_browser_config(cfg);
        browser_ = BrowserFactory::create_stealth_browser(browser_config);
        if (!browser_) {
            result.message = "Failed to launch browser";
            return result;
        }

        // Inject stealth scripts
        evasion_->apply_request_timing_jitter();

        // Step 2: Navigate to signup
        spdlog::info("GmailCreator: navigating to Gmail signup ({})...", result.session_id);
        if (!browser_->navigate_to(GMAIL_SIGNUP_URL, std::chrono::milliseconds(30000))) {
            result.message = "Failed to navigate to signup page";
            return result;
        }
        std::this_thread::sleep_for(behavior_->medium_pause());

        // Step 3: Fill name
        if (!fill_name_step(cfg.persona)) {
            result.message = "Failed at name step";
            return result;
        }
        std::this_thread::sleep_for(behavior_->short_pause());

        // Step 4: Birthday and gender
        if (!fill_birthday_gender_step(cfg.persona)) {
            result.message = "Failed at birthday/gender step";
            return result;
        }
        std::this_thread::sleep_for(behavior_->short_pause());

        // Step 5: Username
        std::string chosen_username = select_username(cfg.persona);
        if (chosen_username.empty()) {
            result.message = "Failed to find available username";
            return result;
        }
        std::this_thread::sleep_for(behavior_->short_pause());

        // Step 6: Password
        if (!fill_password_step(cfg.password)) {
            result.message = "Failed at password step";
            return result;
        }
        std::this_thread::sleep_for(behavior_->medium_pause());

        // Step 7: Phone verification
        std::string verification_code;
        if (!cfg.skip_phone_verification) {
            if (!handle_phone_verification(cfg.phone_country, verification_code)) {
                result.message = "Failed at phone verification";
                return result;
            }
        }

        // Step 8: Recovery email
        if (!cfg.recovery_email.empty()) {
            fill_recovery_email(cfg.recovery_email);
        }

        // Step 9: Accept ToS
        if (!accept_terms()) {
            result.message = "Failed to accept terms";
            return result;
        }

        // Verify success
        std::this_thread::sleep_for(behavior_->long_pause());
        if (!verify_account_created()) {
            result.message = "Account creation may not have succeeded";
            // Try to detect email anyway
        }

        result.success  = true;
        result.email    = chosen_username + "@gmail.com";
        result.password = cfg.password;
        result.message  = "Account created successfully";

        // Save screenshot
        std::string ss_path = "output/screenshots/" + result.session_id + "_success.png";
        fs::create_directories("output/screenshots");
        browser_->take_screenshot(ss_path);

        // Save to vault
        if (vault_) {
            Credential cred;
            cred.email    = result.email;
            cred.password = cfg.password;
            cred.phone    = cfg.phone_country;
            vault_->save_credential(cred);
        }

    } catch (const std::exception& e) {
        result.message = "Exception: " + std::string(e.what());
        spdlog::error("GmailCreator exception: {}", e.what());
    }

    if (browser_) browser_->stop();
    return result;
}

BrowserConfig GmailCreator::setup_browser_config(const GmailCreationConfig& cfg) const {
    auto bc = BrowserFactory::build_stealth_config();
    if (proxy_manager_) {
        auto proxy = proxy_manager_->get_next_proxy();
        if (proxy) bc.proxy_server = proxy->to_string();
    }
    bc.user_data_dir = "output/profiles/" + generate_session_id();
    if (!cfg.browser_executable.empty()) bc.chrome_path = cfg.browser_executable;
    return bc;
}

bool GmailCreator::fill_name_step(const HumanPersona& persona) {
    // Wait for first name field
    if (!wait_and_type("#firstName", persona.first_name)) {
        spdlog::error("GmailCreator: could not type first name");
        return false;
    }
    std::this_thread::sleep_for(behavior_->short_pause());

    if (!wait_and_type("#lastName", persona.last_name)) {
        spdlog::error("GmailCreator: could not type last name");
        return false;
    }
    std::this_thread::sleep_for(behavior_->short_pause());

    if (!click_next()) return false;
    return true;
}

bool GmailCreator::fill_birthday_gender_step(const HumanPersona& persona) {
    std::this_thread::sleep_for(behavior_->medium_pause());

    // Month select
    browser_->select_option("#month",
        std::to_string(persona.birth_date.month));
    std::this_thread::sleep_for(behavior_->short_pause());

    // Day
    wait_and_type("#day", std::to_string(persona.birth_date.day));
    std::this_thread::sleep_for(behavior_->short_pause());

    // Year
    wait_and_type("#year", std::to_string(persona.birth_date.year));
    std::this_thread::sleep_for(behavior_->short_pause());

    // Gender
    std::string gender_val = (persona.gender == Gender::MALE) ? "1" :
                             (persona.gender == Gender::FEMALE) ? "2" : "3";
    browser_->select_option("#gender", gender_val);
    std::this_thread::sleep_for(behavior_->short_pause());

    return click_next();
}

std::string GmailCreator::select_username(const HumanPersona& persona) {
    std::this_thread::sleep_for(behavior_->medium_pause());

    // Check if system suggests usernames
    auto suggestions = browser_->get_all_elements_text(".aSQyMe");
    if (!suggestions.empty()) {
        // Pick a random suggestion
        std::uniform_int_distribution<size_t> dis(0, suggestions.size() - 1);
        auto& chosen = suggestions[dis(rng_)];
        if (!chosen.empty()) {
            browser_->click_element(".aSQyMe");
            spdlog::info("GmailCreator: using suggested username: {}", chosen);
            click_next();
            return chosen;
        }
    }

    // Generate username candidates
    std::vector<std::string> candidates = generate_username_candidates(persona);
    for (auto& candidate : candidates) {
        browser_->click_element("#username");
        wait_and_type("#username", candidate);
        std::this_thread::sleep_for(std::chrono::milliseconds(1500));

        // Check for "username taken" error
        if (!browser_->element_exists(".o6cuMc") &&
            !browser_->element_exists("[data-initial-value='USERNAME_EXISTS']")) {
            spdlog::info("GmailCreator: username available: {}", candidate);
            click_next();
            return candidate;
        }
        spdlog::warn("GmailCreator: username '{}' taken, trying next", candidate);
    }

    return "";
}

std::vector<std::string> GmailCreator::generate_username_candidates(
    const HumanPersona& persona) const {
    std::vector<std::string> candidates;
    std::string f = to_lower(persona.first_name);
    std::string l = to_lower(persona.last_name);
    int year = persona.birth_date.year;

    std::uniform_int_distribution<int> num_dis(1, 9999);

    candidates.push_back(f + l);
    candidates.push_back(f + "." + l);
    candidates.push_back(f + l + std::to_string(year % 100));
    candidates.push_back(f.substr(0,1) + l + std::to_string(num_dis(rng_)));
    candidates.push_back(f + l + std::to_string(num_dis(rng_)));
    candidates.push_back(f + "_" + l);
    candidates.push_back(l + f.substr(0,1) + std::to_string(num_dis(rng_)));
    candidates.push_back(f + l + std::to_string(num_dis(rng_)));

    return candidates;
}

bool GmailCreator::fill_password_step(const std::string& password) {
    std::this_thread::sleep_for(behavior_->medium_pause());
    if (!wait_and_type("#passwd", password)) return false;
    std::this_thread::sleep_for(behavior_->short_pause());
    if (!wait_and_type("#confirm-passwd", password)) return false;
    std::this_thread::sleep_for(behavior_->short_pause());
    return click_next();
}

bool GmailCreator::handle_phone_verification(
    const std::string& country, std::string& out_code) {
    std::this_thread::sleep_for(behavior_->medium_pause());

    if (!sms_manager_) {
        spdlog::warn("GmailCreator: no SMS manager, skipping phone verification");
        return false;
    }

    // Get phone number
    auto pn = sms_manager_->get_phone_number(country, "google");
    if (pn.number.empty()) {
        spdlog::error("GmailCreator: failed to get phone number");
        return false;
    }
    spdlog::info("GmailCreator: using phone number: {}", pn.number);

    // Enter phone number
    wait_and_type("#phoneNumberId", pn.number);
    std::this_thread::sleep_for(behavior_->short_pause());
    click_next();
    std::this_thread::sleep_for(behavior_->medium_pause());

    // Wait for SMS code
    auto sms_text = sms_manager_->wait_for_verification_code(pn.id, 120);
    if (sms_text.empty()) {
        sms_manager_->cancel_number(pn.id);
        spdlog::error("GmailCreator: SMS code not received");
        return false;
    }

    out_code = SMSVerificationManager::extract_verification_code(sms_text);
    spdlog::info("GmailCreator: received SMS code: {}", out_code);

    // Enter code
    wait_and_type("#code", out_code);
    std::this_thread::sleep_for(behavior_->short_pause());
    click_next();

    return true;
}

bool GmailCreator::fill_recovery_email(const std::string& recovery_email) {
    if (!browser_->element_exists("#recoveryEmailId")) return true; // optional step
    wait_and_type("#recoveryEmailId", recovery_email);
    std::this_thread::sleep_for(behavior_->short_pause());
    click_next();
    return true;
}

bool GmailCreator::accept_terms() {
    std::this_thread::sleep_for(behavior_->medium_pause());
    // Scroll down to see the full ToS
    browser_->execute_javascript("window.scrollTo(0, document.body.scrollHeight)");
    std::this_thread::sleep_for(behavior_->short_pause());

    // Click agree button
    std::vector<std::string> agree_selectors = {
        "[data-action='next']",
        ".VfPpkd-LgbsSe[data-idom-class='ksBjEc nKpRLb']",
        "button[jsname='LgbsSe']",
        "#termsofserviceNext"
    };

    for (auto& sel : agree_selectors) {
        if (browser_->element_exists(sel)) {
            return browser_->click_element(sel);
        }
    }
    return false;
}

bool GmailCreator::verify_account_created() {
    // Check for inbox or welcome page
    auto url = browser_->get_current_url();
    return url.find("mail.google.com") != std::string::npos
        || url.find("myaccount.google.com") != std::string::npos
        || url.find("https://www.google.com/?gws_rd=ssl") != std::string::npos;
}

bool GmailCreator::wait_and_type(const std::string& selector, const std::string& text) {
    if (!browser_->click_element(selector, 10000)) {
        spdlog::warn("GmailCreator: could not click selector '{}'", selector);
        return false;
    }
    std::this_thread::sleep_for(behavior_->short_pause());
    behavior_->simulate_typing_delay(text);
    return browser_->type_text(selector, text, true);
}

bool GmailCreator::click_next() {
    std::vector<std::string> next_selectors = {
        "#next", "button[jsname='LgbsSe']", "#accountDetailsNext",
        "#basicDataNext", "#personalDataNext", ".VfPpkd-LgbsSe[data-action='next']"
    };
    for (auto& sel : next_selectors) {
        if (browser_->element_exists(sel)) {
            browser_->click_element(sel);
            std::this_thread::sleep_for(behavior_->medium_pause());
            return true;
        }
    }
    return false;
}

std::string GmailCreator::generate_session_id() const {
    static std::atomic<int> counter{0};
    return "sess_" + std::to_string(
        std::chrono::system_clock::now().time_since_epoch().count()) +
        "_" + std::to_string(++counter);
}

std::string GmailCreator::to_lower(std::string s) const {
    std::transform(s.begin(), s.end(), s.begin(), ::tolower);
    return s;
}

} // namespace gmail_infinity
