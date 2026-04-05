#include "metrics.h"
#include "app_logger.h"
#include <fstream>
#include <iomanip>
#include <sstream>

namespace gmail_infinity {

CreationMetrics::CreationMetrics() {
    creation_start_time_ = std::chrono::steady_clock::now();
}

void CreationMetrics::increment_total() {
    total_created_++;
}

void CreationMetrics::increment_successful() {
    successful_count_++;
}

void CreationMetrics::increment_failed() {
    failed_count_++;
}

void CreationMetrics::increment_skipped() {
    skipped_count_++;
}

size_t CreationMetrics::get_total_created() const {
    return total_created_.load();
}

size_t CreationMetrics::get_successful_count() const {
    return successful_count_.load();
}

size_t CreationMetrics::get_failed_count() const {
    return failed_count_.load();
}

size_t CreationMetrics::get_skipped_count() const {
    return skipped_count_.load();
}

double CreationMetrics::get_success_rate() const {
    size_t total = total_created_.load();
    if (total == 0) return 0.0;
    return (static_cast<double>(successful_count_.load()) / static_cast<double>(total)) * 100.0;
}

void CreationMetrics::start_creation_timer() {
    creation_start_time_ = std::chrono::steady_clock::now();
}

void CreationMetrics::end_creation_timer(bool success) {
    auto end_time = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - creation_start_time_);
    
    update_creation_timing(duration);
    
    if (success) {
        increment_successful();
    } else {
        increment_failed();
    }
    
    increment_total();
}

std::chrono::milliseconds CreationMetrics::get_average_creation_time() const {
    size_t completed = completed_creations_.load();
    if (completed == 0) return std::chrono::milliseconds(0);
    
    auto total_ms = total_creation_time_ms_.load();
    return std::chrono::milliseconds(total_ms / completed);
}

std::chrono::milliseconds CreationMetrics::get_last_creation_time() const {
    return std::chrono::milliseconds(last_creation_time_ms_.load());
}

void CreationMetrics::set_active_threads(size_t count) {
    active_threads_ = count;
}

size_t CreationMetrics::get_active_threads() const {
    return active_threads_.load();
}

void CreationMetrics::increment_proxy_errors() {
    proxy_error_count_++;
}

size_t CreationMetrics::get_proxy_error_count() const {
    return proxy_error_count_.load();
}

void CreationMetrics::set_proxy_pool_size(size_t size) {
    proxy_pool_size_ = size;
}

size_t CreationMetrics::get_proxy_pool_size() const {
    return proxy_pool_size_.load();
}

void CreationMetrics::increment_sms_attempts() {
    sms_attempts_++;
}

void CreationMetrics::increment_sms_successes() {
    sms_successes_++;
}

void CreationMetrics::increment_captcha_attempts() {
    captcha_attempts_++;
}

void CreationMetrics::increment_captcha_successes() {
    captcha_successes_++;
}

size_t CreationMetrics::get_sms_attempts() const {
    return sms_attempts_.load();
}

size_t CreationMetrics::get_sms_successes() const {
    return sms_successes_.load();
}

double CreationMetrics::get_sms_success_rate() const {
    size_t attempts = sms_attempts_.load();
    if (attempts == 0) return 0.0;
    return (static_cast<double>(sms_successes_.load()) / static_cast<double>(attempts)) * 100.0;
}

size_t CreationMetrics::get_captcha_attempts() const {
    return captcha_attempts_.load();
}

size_t CreationMetrics::get_captcha_successes() const {
    return captcha_successes_.load();
}

double CreationMetrics::get_captcha_success_rate() const {
    size_t attempts = captcha_attempts_.load();
    if (attempts == 0) return 0.0;
    return (static_cast<double>(captcha_successes_.load()) / static_cast<double>(attempts)) * 100.0;
}

bool CreationMetrics::save(const std::string& filename) const {
    try {
        std::lock_guard<std::mutex> lock(mutex_);
        
        std::ofstream file(filename);
        if (!file.is_open()) {
            LOG_ERROR("Failed to open metrics file for writing: {}", filename);
            return false;
        }
        
        auto json_data = to_json();
        file << json_data.dump(4);
        
        LOG_INFO("Metrics saved to {}", filename);
        return true;
        
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to save metrics: {}", e.what());
        return false;
    }
}

bool CreationMetrics::load(const std::string& filename) {
    try {
        std::lock_guard<std::mutex> lock(mutex_);
        
        std::ifstream file(filename);
        if (!file.is_open()) {
            LOG_WARN("Metrics file not found, starting with empty metrics: {}", filename);
            return false;
        }
        
        nlohmann::json json_data;
        file >> json_data;
        
        from_json(json_data);
        
        LOG_INFO("Metrics loaded from {}", filename);
        return true;
        
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to load metrics: {}", e.what());
        return false;
    }
}

nlohmann::json CreationMetrics::to_json() const {
    nlohmann::json j;
    
    j["total_created"] = total_created_.load();
    j["successful_count"] = successful_count_.load();
    j["failed_count"] = failed_count_.load();
    j["skipped_count"] = skipped_count_.load();
    j["success_rate"] = get_success_rate();
    
    j["average_creation_time_ms"] = get_average_creation_time().count();
    j["last_creation_time_ms"] = get_last_creation_time().count();
    
    j["active_threads"] = active_threads_.load();
    j["proxy_error_count"] = proxy_error_count_.load();
    j["proxy_pool_size"] = proxy_pool_size_.load();
    
    j["sms_attempts"] = sms_attempts_.load();
    j["sms_successes"] = sms_successes_.load();
    j["sms_success_rate"] = get_sms_success_rate();
    
    j["captcha_attempts"] = captcha_attempts_.load();
    j["captcha_successes"] = captcha_successes_.load();
    j["captcha_success_rate"] = get_captcha_success_rate();
    
    // Add timestamp
    j["timestamp"] = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
    
    return j;
}

void CreationMetrics::from_json(const nlohmann::json& j) {
    if (j.contains("total_created")) {
        total_created_ = j["total_created"].get<size_t>();
    }
    
    if (j.contains("successful_count")) {
        successful_count_ = j["successful_count"].get<size_t>();
    }
    
    if (j.contains("failed_count")) {
        failed_count_ = j["failed_count"].get<size_t>();
    }
    
    if (j.contains("skipped_count")) {
        skipped_count_ = j["skipped_count"].get<size_t>();
    }
    
    if (j.contains("active_threads")) {
        active_threads_ = j["active_threads"].get<size_t>();
    }
    
    if (j.contains("proxy_error_count")) {
        proxy_error_count_ = j["proxy_error_count"].get<size_t>();
    }
    
    if (j.contains("proxy_pool_size")) {
        proxy_pool_size_ = j["proxy_pool_size"].get<size_t>();
    }
    
    if (j.contains("sms_attempts")) {
        sms_attempts_ = j["sms_attempts"].get<size_t>();
    }
    
    if (j.contains("sms_successes")) {
        sms_successes_ = j["sms_successes"].get<size_t>();
    }
    
    if (j.contains("captcha_attempts")) {
        captcha_attempts_ = j["captcha_attempts"].get<size_t>();
    }
    
    if (j.contains("captcha_successes")) {
        captcha_successes_ = j["captcha_successes"].get<size_t>();
    }
    
    if (j.contains("total_creation_time_ms")) {
        total_creation_time_ms_ = j["total_creation_time_ms"].get<std::chrono::milliseconds::rep>();
    }
    
    if (j.contains("completed_creations")) {
        completed_creations_ = j["completed_creations"].get<size_t>();
    }
}

void CreationMetrics::reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    total_created_ = 0;
    successful_count_ = 0;
    failed_count_ = 0;
    skipped_count_ = 0;
    
    total_creation_time_ms_ = 0;
    last_creation_time_ms_ = 0;
    completed_creations_ = 0;
    
    active_threads_ = 0;
    proxy_error_count_ = 0;
    proxy_pool_size_ = 0;
    
    sms_attempts_ = 0;
    sms_successes_ = 0;
    captcha_attempts_ = 0;
    captcha_successes_ = 0;
    
    creation_start_time_ = std::chrono::steady_clock::now();
}

std::string CreationMetrics::get_summary() const {
    std::ostringstream ss;
    
    ss << "=== Gmail Infinity Metrics ===\n";
    ss << "Total Accounts: " << get_total_created() << "\n";
    ss << "Successful: " << get_successful_count() << " (" << std::fixed << std::setprecision(1) << get_success_rate() << "%)\n";
    ss << "Failed: " << get_failed_count() << "\n";
    ss << "Skipped: " << get_skipped_count() << "\n";
    ss << "Average Creation Time: " << get_average_creation_time().count() << "ms\n";
    ss << "Active Threads: " << get_active_threads() << "\n";
    ss << "Proxy Errors: " << get_proxy_error_count() << "\n";
    ss << "Proxy Pool Size: " << get_proxy_pool_size() << "\n";
    ss << "SMS Success Rate: " << std::fixed << std::setprecision(1) << get_sms_success_rate() << "%\n";
    ss << "CAPTCHA Success Rate: " << std::fixed << std::setprecision(1) << get_captcha_success_rate() << "%\n";
    
    return ss.str();
}

void CreationMetrics::update_creation_timing(std::chrono::milliseconds duration) {
    last_creation_time_ms_ = duration.count();
    total_creation_time_ms_ += duration.count();
    completed_creations_++;
}

} // namespace gmail_infinity
