#include "sms_providers.h"
#include "app/app_logger.h"

#include <spdlog/spdlog.h>
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <chrono>
#include <thread>
#include <sstream>
#include <random>

namespace gmail_infinity {

using json = nlohmann::json;

static size_t write_cb(void* ptr, size_t sz, size_t nmb, std::string* out) {
    out->append(static_cast<char*>(ptr), sz * nmb); return sz * nmb;
}

static std::string http_get(const std::string& url,
    const std::vector<std::string>& headers = {}) {
    CURL* c = curl_easy_init();
    if (!c) return "";
    std::string resp;
    struct curl_slist* hdrs = nullptr;
    for (auto& h : headers) hdrs = curl_slist_append(hdrs, h.c_str());
    curl_easy_setopt(c, CURLOPT_URL, url.c_str());
    curl_easy_setopt(c, CURLOPT_HTTPHEADER, hdrs);
    curl_easy_setopt(c, CURLOPT_WRITEFUNCTION, write_cb);
    curl_easy_setopt(c, CURLOPT_WRITEDATA, &resp);
    curl_easy_setopt(c, CURLOPT_TIMEOUT, 30L);
    curl_easy_setopt(c, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_perform(c);
    if (hdrs) curl_slist_free_all(hdrs);
    curl_easy_cleanup(c);
    return resp;
}

static std::string http_post(const std::string& url, const std::string& body,
    const std::vector<std::string>& headers = {}) {
    CURL* c = curl_easy_init();
    if (!c) return "";
    std::string resp;
    struct curl_slist* hdrs = nullptr;
    for (auto& h : headers) hdrs = curl_slist_append(hdrs, h.c_str());
    curl_easy_setopt(c, CURLOPT_URL, url.c_str());
    curl_easy_setopt(c, CURLOPT_HTTPHEADER, hdrs);
    curl_easy_setopt(c, CURLOPT_POST, 1L);
    curl_easy_setopt(c, CURLOPT_POSTFIELDS, body.c_str());
    curl_easy_setopt(c, CURLOPT_WRITEFUNCTION, write_cb);
    curl_easy_setopt(c, CURLOPT_WRITEDATA, &resp);
    curl_easy_setopt(c, CURLOPT_TIMEOUT, 30L);
    curl_easy_perform(c);
    if (hdrs) curl_slist_free_all(hdrs);
    curl_easy_cleanup(c);
    return resp;
}

// ─────────────────────────────────────────────────────────
// 5sim provider
// ─────────────────────────────────────────────────────────

FiveSimProvider::FiveSimProvider(const std::string& api_key)
    : api_key_(api_key) {}

PhoneNumber FiveSimProvider::get_phone_number(const std::string& country,
    const std::string& /*service*/, const std::string& /*operator_name*/) {
    std::string url = "https://5sim.net/v1/user/buy/activation/"
                     + country + "/any/google";
    auto resp = http_get(url, {"Authorization: Bearer " + api_key_,
                                "Accept: application/json"});
    try {
        auto j = json::parse(resp);
        PhoneNumber pn;
        pn.id     = std::to_string(j.value("id", 0));
        pn.number = j.value("phone", "");
        pn.country = country;
        pn.provider = "5sim";
        pn.status  = "active";
        return pn;
    } catch (...) {
        return PhoneNumber{};
    }
}

std::string FiveSimProvider::wait_for_sms(const std::string& number_id, int timeout_seconds) {
    auto deadline = std::chrono::steady_clock::now()
                  + std::chrono::seconds(timeout_seconds);
    while (std::chrono::steady_clock::now() < deadline) {
        std::string url = "https://5sim.net/v1/user/check/" + number_id;
        auto resp = http_get(url, {"Authorization: Bearer " + api_key_,
                                    "Accept: application/json"});
        try {
            auto j = json::parse(resp);
            auto sms = j.value("sms", json::array());
            if (!sms.empty()) {
                return sms[0].value("text", "");
            }
        } catch (...) {}
        std::this_thread::sleep_for(std::chrono::seconds(5));
    }
    return "";
}

bool FiveSimProvider::cancel_number(const std::string& number_id) {
    auto resp = http_get("https://5sim.net/v1/user/cancel/" + number_id,
        {"Authorization: Bearer " + api_key_});
    return resp.find("CANCELED") != std::string::npos;
}

bool FiveSimProvider::report_number(const std::string& number_id) {
    auto resp = http_get("https://5sim.net/v1/user/ban/" + number_id,
        {"Authorization: Bearer " + api_key_});
    return !resp.empty();
}

double FiveSimProvider::get_balance() {
    auto resp = http_get("https://5sim.net/v1/user/profile",
        {"Authorization: Bearer " + api_key_,
         "Accept: application/json"});
    try {
        return json::parse(resp).value("balance", 0.0);
    } catch (...) { return 0.0; }
}

std::vector<std::string> FiveSimProvider::get_available_countries() {
    auto resp = http_get("https://5sim.net/v1/guest/countries",
        {"Accept: application/json"});
    try {
        auto j = json::parse(resp);
        std::vector<std::string> countries;
        for (auto& el : j.items()) countries.push_back(el.key());
        return countries;
    } catch (...) { return {}; }
}

// ─────────────────────────────────────────────────────────
// sms-activate.org provider
// ─────────────────────────────────────────────────────────

SmsActivateProvider::SmsActivateProvider(const std::string& api_key)
    : api_key_(api_key) {}

PhoneNumber SmsActivateProvider::get_phone_number(
    const std::string& country, const std::string& /*service*/, const std::string& /*op*/) {
    std::string url = "https://api.sms-activate.org/stubs/handler_api.php"
                     "?api_key=" + api_key_ + "&action=getNumber&service=go&country=" + country;
    auto resp = http_get(url);
    // Response format: ACCESS_NUMBER:id:number
    PhoneNumber pn;
    if (resp.find("ACCESS_NUMBER:") != std::string::npos) {
        auto parts = resp.substr(14); // remove "ACCESS_NUMBER:"
        auto c1 = parts.find(':');
        pn.id     = parts.substr(0, c1);
        pn.number = parts.substr(c1 + 1);
        pn.country = country;
        pn.provider = "sms-activate";
        pn.status  = "active";
    }
    return pn;
}

std::string SmsActivateProvider::wait_for_sms(const std::string& number_id, int timeout_seconds) {
    auto deadline = std::chrono::steady_clock::now()
                  + std::chrono::seconds(timeout_seconds);
    while (std::chrono::steady_clock::now() < deadline) {
        std::string url = "https://api.sms-activate.org/stubs/handler_api.php"
                         "?api_key=" + api_key_ + "&action=getStatus&id=" + number_id;
        auto resp = http_get(url);
        if (resp.find("STATUS_OK:") != std::string::npos) {
            return resp.substr(10); // code after "STATUS_OK:"
        }
        if (resp == "STATUS_CANCEL") return "";
        std::this_thread::sleep_for(std::chrono::seconds(5));
    }
    return "";
}

bool SmsActivateProvider::cancel_number(const std::string& number_id) {
    std::string url = "https://api.sms-activate.org/stubs/handler_api.php"
                     "?api_key=" + api_key_ + "&action=setStatus&status=8&id=" + number_id;
    return http_get(url).find("ACCESS_CANCEL") != std::string::npos;
}

bool SmsActivateProvider::report_number(const std::string& number_id) {
    cancel_number(number_id);
    return true;
}

double SmsActivateProvider::get_balance() {
    auto resp = http_get("https://api.sms-activate.org/stubs/handler_api.php"
                          "?api_key=" + api_key_ + "&action=getBalance");
    if (resp.find("ACCESS_BALANCE:") != std::string::npos)
        return std::stod(resp.substr(15));
    return 0.0;
}

std::vector<std::string> SmsActivateProvider::get_available_countries() {
    return {"us","ru","gb","de","fr","es","in","br","ua","kz"};
}

// ─────────────────────────────────────────────────────────
// SMSVerificationManager
// ─────────────────────────────────────────────────────────

SMSVerificationManager::SMSVerificationManager() {
    std::random_device rd;
    rng_.seed(rd());
    curl_global_init(CURL_GLOBAL_DEFAULT);
}

SMSVerificationManager::~SMSVerificationManager() {
    curl_global_cleanup();
}

void SMSVerificationManager::add_provider(
    const std::string& name,
    std::unique_ptr<SMSProvider> provider,
    int priority) {
    std::lock_guard lock(mutex_);
    providers_[name] = {std::move(provider), priority};
    spdlog::info("SMSVerificationManager: added provider '{}' with priority {}", name, priority);
}

PhoneNumber SMSVerificationManager::get_phone_number(
    const std::string& country, const std::string& service) {
    std::lock_guard lock(mutex_);
    if (providers_.empty()) {
        spdlog::error("SMSVerificationManager: no providers configured");
        return PhoneNumber{};
    }

    // Sort by priority
    std::vector<std::pair<std::string, int>> sorted;
    for (auto& [name, entry] : providers_)
        sorted.emplace_back(name, entry.priority);
    std::sort(sorted.begin(), sorted.end(),
        [](auto& a, auto& b){ return a.second < b.second; });

    for (auto& [name, _] : sorted) {
        try {
            auto& entry = providers_[name];
            if (entry.provider->get_balance() < 0.5) {
                spdlog::warn("SMSVerificationManager: provider '{}' low balance, skipping", name);
                continue;
            }
            auto pn = entry.provider->get_phone_number(country, service, "");
            if (!pn.number.empty()) {
                active_numbers_[pn.id] = name;
                return pn;
            }
        } catch (const std::exception& e) {
            spdlog::warn("SMSVerificationManager: provider '{}' error: {}", name, e.what());
        }
    }
    return PhoneNumber{};
}

std::string SMSVerificationManager::wait_for_verification_code(
    const std::string& number_id, int timeout_seconds) {
    std::string provider_name;
    {
        std::lock_guard lock(mutex_);
        auto it = active_numbers_.find(number_id);
        if (it == active_numbers_.end()) return "";
        provider_name = it->second;
    }
    auto it = providers_.find(provider_name);
    if (it == providers_.end()) return "";
    return it->second.provider->wait_for_sms(number_id, timeout_seconds);
}

bool SMSVerificationManager::cancel_number(const std::string& number_id) {
    std::string provider_name;
    {
        std::lock_guard lock(mutex_);
        auto it = active_numbers_.find(number_id);
        if (it == active_numbers_.end()) return false;
        provider_name = it->second;
        active_numbers_.erase(it);
    }
    auto it = providers_.find(provider_name);
    if (it == providers_.end()) return false;
    return it->second.provider->cancel_number(number_id);
}

std::string SMSVerificationManager::extract_verification_code(const std::string& sms_text) {
    // Extract 6-digit code from SMS
    std::regex code_regex(R"(\b(\d{6})\b)");
    std::smatch match;
    if (std::regex_search(sms_text, match, code_regex)) return match[1];
    // Try 4-digit
    std::regex code4_regex(R"(\b(\d{4})\b)");
    if (std::regex_search(sms_text, match, code4_regex)) return match[1];
    return "";
}

} // namespace gmail_infinity
