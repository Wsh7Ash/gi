#include "email_recovery.h"
#include "app/app_logger.h"

#include <spdlog/spdlog.h>
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <chrono>
#include <thread>
#include <regex>
#include <random>

namespace gmail_infinity {

using json = nlohmann::json;

static size_t er_write_cb(void* ptr, size_t sz, size_t nmb, std::string* out) {
    out->append(static_cast<char*>(ptr), sz * nmb); return sz * nmb;
}

static std::string er_http_get(const std::string& url,
    const std::vector<std::string>& hdrs = {}) {
    CURL* c = curl_easy_init(); if (!c) return "";
    std::string resp;
    struct curl_slist* h = nullptr;
    for (auto& s : hdrs) h = curl_slist_append(h, s.c_str());
    curl_easy_setopt(c, CURLOPT_URL, url.c_str());
    curl_easy_setopt(c, CURLOPT_HTTPHEADER, h);
    curl_easy_setopt(c, CURLOPT_WRITEFUNCTION, er_write_cb);
    curl_easy_setopt(c, CURLOPT_WRITEDATA, &resp);
    curl_easy_setopt(c, CURLOPT_TIMEOUT, 30L);
    curl_easy_setopt(c, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_perform(c);
    if (h) curl_slist_free_all(h);
    curl_easy_cleanup(c);
    return resp;
}

static std::string er_http_post(const std::string& url, const std::string& body,
    const std::vector<std::string>& hdrs = {}) {
    CURL* c = curl_easy_init(); if (!c) return "";
    std::string resp;
    struct curl_slist* h = nullptr;
    for (auto& s : hdrs) h = curl_slist_append(h, s.c_str());
    curl_easy_setopt(c, CURLOPT_URL, url.c_str());
    curl_easy_setopt(c, CURLOPT_HTTPHEADER, h);
    curl_easy_setopt(c, CURLOPT_POST, 1L);
    curl_easy_setopt(c, CURLOPT_POSTFIELDS, body.c_str());
    curl_easy_setopt(c, CURLOPT_WRITEFUNCTION, er_write_cb);
    curl_easy_setopt(c, CURLOPT_WRITEDATA, &resp);
    curl_easy_setopt(c, CURLOPT_TIMEOUT, 30L);
    curl_easy_perform(c);
    if (h) curl_slist_free_all(h);
    curl_easy_cleanup(c);
    return resp;
}

// ─────────────────────────────────────────────────────────
// MailTmProvider
// ─────────────────────────────────────────────────────────

MailTmProvider::MailTmProvider() {
    curl_global_init(CURL_GLOBAL_DEFAULT);
}

MailTmProvider::~MailTmProvider() {
    curl_global_cleanup();
}

TempEmailAccount MailTmProvider::create_account() {
    // 1. Get available domain
    auto domains_resp = er_http_get("https://api.mail.tm/domains",
        {"Accept: application/json"});
    std::string domain = "mail.tm";
    try {
        auto dj = json::parse(domains_resp);
        if (dj.contains("hydra:member") && !dj["hydra:member"].empty())
            domain = dj["hydra:member"][0].value("domain","mail.tm");
    } catch (...) {}

    // 2. Generate random address
    std::random_device rd;
    std::mt19937 rng(rd());
    std::uniform_int_distribution<int> num_dis(1000, 99999);
    const std::string chars = "abcdefghijklmnopqrstuvwxyz";
    std::uniform_int_distribution<size_t> char_dis(0, chars.size()-1);
    std::string user;
    for (int i = 0; i < 8; ++i) user += chars[char_dis(rng)];
    user += std::to_string(num_dis(rng));
    std::string password = "Temp@" + std::to_string(num_dis(rng)) + "!Xyz";
    std::string address = user + "@" + domain;

    // 3. Create account
    json create_payload = {{"address", address}, {"password", password}};
    auto create_resp = er_http_post("https://api.mail.tm/accounts",
        create_payload.dump(), {"Content-Type: application/json", "Accept: application/json"});

    TempEmailAccount account;
    try {
        auto cj = json::parse(create_resp);
        account.id       = cj.value("id","");
        account.address  = address;
        account.password = password;
        account.provider = "mail.tm";
        account.created_at = std::chrono::system_clock::now();
    } catch (...) {}

    if (account.id.empty()) {
        spdlog::warn("MailTmProvider: failed to create account, response: {}", create_resp);
        return account;
    }

    // 4. Get token
    json login_payload = {{"address", address}, {"password", password}};
    auto token_resp = er_http_post("https://api.mail.tm/token",
        login_payload.dump(), {"Content-Type: application/json", "Accept: application/json"});
    try {
        auth_token_ = json::parse(token_resp).value("token","");
    } catch (...) {}

    spdlog::info("MailTmProvider: created account {}", address);
    return account;
}

std::vector<EmailMessage> MailTmProvider::get_messages(const std::string& account_id) {
    (void)account_id;
    auto resp = er_http_get("https://api.mail.tm/messages?page=1",
        {"Authorization: Bearer " + auth_token_, "Accept: application/json"});
    std::vector<EmailMessage> msgs;
    try {
        auto j = json::parse(resp);
        for (auto& m : j.value("hydra:member", json::array())) {
            EmailMessage msg;
            msg.id      = m.value("id","");
            msg.subject = m.value("subject","");
            msg.from    = m.contains("from") ? m["from"].value("address","") : "";
            msg.body    = m.value("text","");
            msgs.push_back(msg);
        }
    } catch (...) {}
    return msgs;
}

std::string MailTmProvider::wait_for_verification_email(
    const std::string& account_id, const std::string& /*from_filter*/,
    int timeout_seconds) {
    auto deadline = std::chrono::steady_clock::now()
                  + std::chrono::seconds(timeout_seconds);
    while (std::chrono::steady_clock::now() < deadline) {
        auto msgs = get_messages(account_id);
        for (auto& m : msgs) {
            auto code = extract_verification_code(m.body + " " + m.subject);
            if (!code.empty()) return code;
        }
        std::this_thread::sleep_for(std::chrono::seconds(10));
    }
    return "";
}

bool MailTmProvider::delete_account(const std::string& account_id) {
    // mail.tm delete requires DELETE request
    CURL* c = curl_easy_init(); if (!c) return false;
    struct curl_slist* h = nullptr;
    h = curl_slist_append(h, ("Authorization: Bearer " + auth_token_).c_str());
    curl_easy_setopt(c, CURLOPT_URL,
        ("https://api.mail.tm/accounts/" + account_id).c_str());
    curl_easy_setopt(c, CURLOPT_CUSTOMREQUEST, "DELETE");
    curl_easy_setopt(c, CURLOPT_HTTPHEADER, h);
    CURLcode res = curl_easy_perform(c);
    curl_slist_free_all(h);
    curl_easy_cleanup(c);
    return res == CURLE_OK;
}

std::string MailTmProvider::extract_verification_code(const std::string& text) const {
    std::regex code6(R"(\b(\d{6})\b)");
    std::smatch m;
    if (std::regex_search(text, m, code6)) return m[1];
    std::regex code8(R"(\b([A-Z0-9]{8})\b)");
    if (std::regex_search(text, m, code8)) return m[1];
    return "";
}

// ─────────────────────────────────────────────────────────
// EmailRecoveryManager
// ─────────────────────────────────────────────────────────

EmailRecoveryManager::EmailRecoveryManager()
    : provider_(std::make_unique<MailTmProvider>()) {
    curl_global_init(CURL_GLOBAL_DEFAULT);
}

EmailRecoveryManager::~EmailRecoveryManager() {
    curl_global_cleanup();
}

TempEmailAccount EmailRecoveryManager::get_temp_email() {
    auto account = provider_->create_account();
    if (!account.id.empty()) {
        std::lock_guard lock(mutex_);
        active_accounts_[account.id] = account;
    }
    return account;
}

std::string EmailRecoveryManager::wait_for_verification(
    const std::string& account_id, int timeout_seconds) {
    return provider_->wait_for_verification_email(account_id, "", timeout_seconds);
}

bool EmailRecoveryManager::cleanup_account(const std::string& account_id) {
    bool ok = provider_->delete_account(account_id);
    {
        std::lock_guard lock(mutex_);
        active_accounts_.erase(account_id);
    }
    return ok;
}

} // namespace gmail_infinity
