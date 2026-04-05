#include "captcha_solver.h"
#include "app/app_logger.h"

#include <spdlog/spdlog.h>
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <chrono>
#include <thread>
#include <sstream>
#include <regex>

namespace gmail_infinity {

using json = nlohmann::json;

static size_t cap_write_cb(void* ptr, size_t sz, size_t nmb, std::string* out) {
    out->append(static_cast<char*>(ptr), sz * nmb); return sz * nmb;
}

static std::string cap_http_get(const std::string& url) {
    CURL* c = curl_easy_init();
    if (!c) return "";
    std::string resp;
    curl_easy_setopt(c, CURLOPT_URL, url.c_str());
    curl_easy_setopt(c, CURLOPT_WRITEFUNCTION, cap_write_cb);
    curl_easy_setopt(c, CURLOPT_WRITEDATA, &resp);
    curl_easy_setopt(c, CURLOPT_TIMEOUT, 30L);
    curl_easy_perform(c);
    curl_easy_cleanup(c);
    return resp;
}

static std::string cap_http_post(const std::string& url, const std::string& body,
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
    curl_easy_setopt(c, CURLOPT_WRITEFUNCTION, cap_write_cb);
    curl_easy_setopt(c, CURLOPT_WRITEDATA, &resp);
    curl_easy_setopt(c, CURLOPT_TIMEOUT, 60L);
    curl_easy_perform(c);
    if (hdrs) curl_slist_free_all(hdrs);
    curl_easy_cleanup(c);
    return resp;
}

// ─────────────────────────────────────────────────────────
// TwoCaptchaProvider
// ─────────────────────────────────────────────────────────

TwoCaptchaProvider::TwoCaptchaProvider(const std::string& api_key)
    : api_key_(api_key) {}

CaptchaResult TwoCaptchaProvider::solve_recaptcha_v2(
    const std::string& site_key, const std::string& url) {
    // Submit task
    std::string submit_url = "http://2captcha.com/in.php?key=" + api_key_
        + "&method=userrecaptcha&googlekey=" + site_key
        + "&pageurl=" + url + "&json=1";
    auto submit_resp = cap_http_get(submit_url);
    CaptchaResult result;
    try {
        auto j = json::parse(submit_resp);
        if (j.value("status", 0) != 1) {
            result.error = j.value("request", "unknown error"); return result;
        }
        result.task_id = j.value("request", "");
    } catch (...) { result.error = "Parse error"; return result; }

    return poll_result(result.task_id);
}

CaptchaResult TwoCaptchaProvider::solve_recaptcha_v3(
    const std::string& site_key, const std::string& url, const std::string& action, double /*min_score*/) {
    std::string submit_url = "http://2captcha.com/in.php?key=" + api_key_
        + "&method=userrecaptcha&version=v3&googlekey=" + site_key
        + "&pageurl=" + url + "&action=" + action + "&min_score=0.7&json=1";
    auto submit_resp = cap_http_get(submit_url);
    CaptchaResult result;
    try {
        auto j = json::parse(submit_resp);
        if (j.value("status", 0) != 1) { result.error = j.value("request","error"); return result; }
        result.task_id = j.value("request","");
    } catch (...) { result.error = "Parse error"; return result; }
    return poll_result(result.task_id);
}

CaptchaResult TwoCaptchaProvider::solve_hcaptcha(
    const std::string& site_key, const std::string& url) {
    std::string submit_url = "http://2captcha.com/in.php?key=" + api_key_
        + "&method=hcaptcha&sitekey=" + site_key
        + "&pageurl=" + url + "&json=1";
    auto submit_resp = cap_http_get(submit_url);
    CaptchaResult result;
    try {
        auto j = json::parse(submit_resp);
        if (j.value("status", 0) != 1) { result.error = j.value("request","error"); return result; }
        result.task_id = j.value("request","");
    } catch (...) { result.error = "Parse error"; return result; }
    return poll_result(result.task_id);
}

CaptchaResult TwoCaptchaProvider::poll_result(const std::string& task_id, int timeout_seconds) {
    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(timeout_seconds);
    std::this_thread::sleep_for(std::chrono::seconds(20)); // Initial wait

    while (std::chrono::steady_clock::now() < deadline) {
        std::string check_url = "http://2captcha.com/res.php?key=" + api_key_
            + "&action=get&id=" + task_id + "&json=1";
        auto resp = cap_http_get(check_url);
        CaptchaResult result;
        result.task_id = task_id;
        try {
            auto j = json::parse(resp);
            if (j.value("status", 0) == 1) {
                result.token    = j.value("request", "");
                result.success  = true;
                result.cost     = 0.002;
                return result;
            }
            auto req = j.value("request", "");
            if (req != "CAPCHA_NOT_READY") {
                result.error = req; return result;
            }
        } catch (...) {}
        std::this_thread::sleep_for(std::chrono::seconds(5));
    }
    CaptchaResult r; r.task_id = task_id; r.error = "Timeout"; return r;
}

double TwoCaptchaProvider::get_balance() {
    auto resp = cap_http_get("http://2captcha.com/res.php?key=" + api_key_ + "&action=getbalance");
    try { return std::stod(resp); } catch (...) { return 0.0; }
}

// ─────────────────────────────────────────────────────────
// AntiCaptchaProvider
// ─────────────────────────────────────────────────────────

AntiCaptchaProvider::AntiCaptchaProvider(const std::string& api_key)
    : api_key_(api_key) {}

CaptchaResult AntiCaptchaProvider::solve_recaptcha_v2(
    const std::string& site_key, const std::string& url) {
    json task = {
        {"type",          "NoCaptchaTaskProxyless"},
        {"websiteURL",    url},
        {"websiteKey",    site_key}
    };
    return submit_and_poll(task);
}

CaptchaResult AntiCaptchaProvider::solve_recaptcha_v3(
    const std::string& site_key, const std::string& url,
    const std::string& action, double min_score) {
    json task = {
        {"type",          "RecaptchaV3TaskProxyless"},
        {"websiteURL",    url},
        {"websiteKey",    site_key},
        {"pageAction",    action},
        {"minScore",      min_score}
    };
    return submit_and_poll(task);
}

CaptchaResult AntiCaptchaProvider::solve_hcaptcha(
    const std::string& site_key, const std::string& url) {
    json task = {
        {"type",         "HCaptchaTaskProxyless"},
        {"websiteURL",   url},
        {"websiteKey",   site_key}
    };
    return submit_and_poll(task);
}

CaptchaResult AntiCaptchaProvider::submit_and_poll(const json& task, int timeout_seconds) {
    json payload = {
        {"clientKey", api_key_},
        {"task",      task}
    };
    auto resp = cap_http_post("https://api.anti-captcha.com/createTask",
        payload.dump(), {"Content-Type: application/json"});
    CaptchaResult result;
    int task_id = 0;
    try {
        auto j = json::parse(resp);
        if (j.value("errorId", 1) != 0) {
            result.error = j.value("errorDescription", "Anti-Captcha API error"); return result;
        }
        task_id = j.value("taskId", 0);
    } catch (...) { result.error = "Parse error"; return result; }

    std::this_thread::sleep_for(std::chrono::seconds(20));
    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(timeout_seconds);
    while (std::chrono::steady_clock::now() < deadline) {
        json poll_payload = {{"clientKey", api_key_}, {"taskId", task_id}};
        auto poll_resp = cap_http_post("https://api.anti-captcha.com/getTaskResult",
            poll_payload.dump(), {"Content-Type: application/json"});
        try {
            auto j = json::parse(poll_resp);
            if (j.value("status","") == "ready") {
                result.token   = j["solution"].value("gRecaptchaResponse","");
                result.success = true;
                result.task_id = std::to_string(task_id);
                return result;
            }
        } catch (...) {}
        std::this_thread::sleep_for(std::chrono::seconds(5));
    }
    result.error = "Timeout"; return result;
}

double AntiCaptchaProvider::get_balance() {
    json payload = {{"clientKey", api_key_}};
    auto resp = cap_http_post("https://api.anti-captcha.com/getBalance",
        payload.dump(), {"Content-Type: application/json"});
    try { return json::parse(resp).value("balance", 0.0); }
    catch (...) { return 0.0; }
}

// ─────────────────────────────────────────────────────────
// CaptchaSolverManager
// ─────────────────────────────────────────────────────────

CaptchaSolverManager::CaptchaSolverManager() {
    curl_global_init(CURL_GLOBAL_DEFAULT);
}

CaptchaSolverManager::~CaptchaSolverManager() {
    curl_global_cleanup();
}

void CaptchaSolverManager::add_provider(
    const std::string& name,
    std::unique_ptr<CaptchaSolverProvider> provider,
    int priority) {
    std::lock_guard lock(mutex_);
    providers_[name] = {std::move(provider), priority};
}

CaptchaResult CaptchaSolverManager::solve_recaptcha_v2(
    const std::string& site_key, const std::string& url) {
    return with_best_provider([&](CaptchaSolverProvider* p) {
        return p->solve_recaptcha_v2(site_key, url);
    });
}

CaptchaResult CaptchaSolverManager::solve_recaptcha_v3(
    const std::string& site_key, const std::string& url,
    const std::string& action, double min_score) {
    return with_best_provider([&](CaptchaSolverProvider* p) {
        return p->solve_recaptcha_v3(site_key, url, action, min_score);
    });
}

CaptchaResult CaptchaSolverManager::solve_hcaptcha(
    const std::string& site_key, const std::string& url) {
    return with_best_provider([&](CaptchaSolverProvider* p) {
        return p->solve_hcaptcha(site_key, url);
    });
}

CaptchaResult CaptchaSolverManager::with_best_provider(
    std::function<CaptchaResult(CaptchaSolverProvider*)> fn) {
    std::lock_guard lock(mutex_);
    // sort by priority
    std::vector<std::pair<std::string,int>> sorted;
    for (auto& [n, e] : providers_) sorted.emplace_back(n, e.priority);
    std::sort(sorted.begin(), sorted.end(),
        [](auto& a, auto& b){ return a.second < b.second; });

    for (auto& [name, _] : sorted) {
        try {
            auto& entry = providers_[name];
            if (entry.provider->get_balance() < 0.10) continue;
            auto result = fn(entry.provider.get());
            if (result.success) return result;
            spdlog::warn("CaptchaSolverManager: provider '{}' failed: {}", name, result.error);
        } catch (const std::exception& e) {
            spdlog::warn("CaptchaSolverManager: provider '{}' exception: {}", name, e.what());
        }
    }
    CaptchaResult r; r.error = "All providers failed"; return r;
}

} // namespace gmail_infinity
