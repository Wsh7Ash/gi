#pragma once

#include <string>
#include <atomic>
#include <chrono>
#include <memory>
#include <mutex>

#include <nlohmann/json.hpp>

namespace gmail_infinity {

class CreationMetrics {
public:
    CreationMetrics();
    ~CreationMetrics() = default;
    
    // Counters
    void increment_total();
    void increment_successful();
    void increment_failed();
    void increment_skipped();
    
    // Getters
    size_t get_total_created() const;
    size_t get_successful_count() const;
    size_t get_failed_count() const;
    size_t get_skipped_count() const;
    
    // Success rate
    double get_success_rate() const;
    
    // Timing
    void start_creation_timer();
    void end_creation_timer(bool success);
    std::chrono::milliseconds get_average_creation_time() const;
    std::chrono::milliseconds get_last_creation_time() const;
    
    // Thread activity
    void set_active_threads(size_t count);
    size_t get_active_threads() const;
    
    // Proxy statistics
    void increment_proxy_errors();
    size_t get_proxy_error_count() const;
    void set_proxy_pool_size(size_t size);
    size_t get_proxy_pool_size() const;
    
    // Verification statistics
    void increment_sms_attempts();
    void increment_sms_successes();
    void increment_captcha_attempts();
    void increment_captcha_successes();
    
    size_t get_sms_attempts() const;
    size_t get_sms_successes() const;
    double get_sms_success_rate() const;
    
    size_t get_captcha_attempts() const;
    size_t get_captcha_successes() const;
    double get_captcha_success_rate() const;
    
    // Serialization
    bool save(const std::string& filename) const;
    bool load(const std::string& filename);
    
    // JSON export
    nlohmann::json to_json() const;
    void from_json(const nlohmann::json& j);
    
    // Reset
    void reset();
    
    // Statistics summary
    std::string get_summary() const;

private:
    // Counters
    std::atomic<size_t> total_created_{0};
    std::atomic<size_t> successful_count_{0};
    std::atomic<size_t> failed_count_{0};
    std::atomic<size_t> skipped_count_{0};
    
    // Timing
    std::chrono::steady_clock::time_point creation_start_time_;
    std::atomic<std::chrono::milliseconds::rep> total_creation_time_ms_{0};
    std::atomic<std::chrono::milliseconds::rep> last_creation_time_ms_{0};
    std::atomic<size_t> completed_creations_{0};
    
    // Thread activity
    std::atomic<size_t> active_threads_{0};
    
    // Proxy statistics
    std::atomic<size_t> proxy_error_count_{0};
    std::atomic<size_t> proxy_pool_size_{0};
    
    // Verification statistics
    std::atomic<size_t> sms_attempts_{0};
    std::atomic<size_t> sms_successes_{0};
    std::atomic<size_t> captcha_attempts_{0};
    std::atomic<size_t> captcha_successes_{0};
    
    // Mutex for non-atomic operations
    mutable std::mutex mutex_;
    
    // Helper methods
    void update_creation_timing(std::chrono::milliseconds duration);
};

} // namespace gmail_infinity
