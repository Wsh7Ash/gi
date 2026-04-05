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

#include <curl/curl.h>
#include <nlohmann/json.hpp>

namespace gmail_infinity {

enum class CaptchaProvider {
    TWO_CAPTCHA,
    ANTI_CAPTCHA,
    CAPSOLVER,
    CAP_MONSTER,
    AZcaptcha,
    DECAPTCHA,
    DEATHBYCAPTCHA,
    IMAGE_TYPERZ,
    CAPTCHA_GURU,
    RU_CAPTCHA
};

enum class CaptchaType {
    TEXT_CAPTCHA,        // Simple text captcha
    IMAGE_CAPTCHA,       // Image-based captcha
    RECAPTCHA_V2,        // Google reCAPTCHA v2
    RECAPTCHA_V3,        // Google reCAPTCHA v3
    HCAPTCHA,           // hCaptcha
    FUNCAPTCHA,         // FunCaptcha
    GEETEST,            // GeeTest
    KEYCAPTCHA,         // KeyCaptcha
    CAPYPTCHA,          // CapyCaptcha
    LEETSPAWN,          // LeetSpeak
    YANDEX,             // Yandex captcha
    PROXY_RECAPTCHA,    // reCAPTCHA with proxy
    GRID_RECAPTCHA,     // Grid reCAPTCHA
    CANVAS_RECAPTCHA    // Canvas-based reCAPTCHA
};

enum class CaptchaStatus {
    PENDING,
    PROCESSING,
    SOLVED,
    FAILED,
    TIMEOUT,
    ERROR,
    INSUFFICIENT_BALANCE
};

struct CaptchaTask {
    std::string id;
    CaptchaProvider provider;
    CaptchaType type;
    std::string image_data;  // Base64 encoded image or URL
    std::string website_url;
    std::string website_key;
    std::string proxy;
    std::map<std::string, std::string> parameters;
    CaptchaStatus status;
    std::string solution;
    std::chrono::system_clock::time_point created_at;
    std::chrono::system_clock::time_point solved_at;
    std::chrono::milliseconds processing_time;
    double cost;
    std::string error_message;
    
    nlohmann::json to_json() const;
    void from_json(const nlohmann::json& j);
    bool is_completed() const;
    int get_elapsed_seconds() const;
};

struct CaptchaProviderConfig {
    std::string api_key;
    std::string api_url;
    std::string api_secret;
    int timeout_seconds = 120;
    int max_retries = 3;
    std::map<std::string, std::string> headers;
    std::map<std::string, std::string> parameters;
    double cost_per_solve = 0.001;
    
    nlohmann::json to_json() const;
    void from_json(const nlohmann::json& j);
};

class CaptchaProviderInterface {
public:
    virtual ~CaptchaProviderInterface() = default;
    
    // Core functionality
    virtual bool initialize(const CaptchaProviderConfig& config) = 0;
    virtual std::shared_ptr<CaptchaTask> solve_captcha(const CaptchaType type,
                                                     const std::map<std::string, std::string>& params) = 0;
    virtual CaptchaStatus check_status(const std::string& task_id) = 0;
    virtual std::string get_solution(const std::string& task_id) = 0;
    virtual bool cancel_task(const std::string& task_id) = 0;
    
    // Provider information
    virtual std::string get_provider_name() const = 0;
    virtual std::vector<CaptchaType> get_supported_types() const = 0;
    virtual double get_cost_per_solve(CaptchaType type) const = 0;
    virtual std::map<std::string, std::string> get_provider_info() const = 0;
    
    // Health and connectivity
    virtual bool test_connection() = 0;
    virtual double get_balance() const = 0;
    virtual std::map<std::string, double> get_statistics() const = 0;

protected:
    CaptchaProviderConfig config_;
    mutable std::mutex provider_mutex_;
    
    // HTTP helpers
    std::string make_http_request(const std::string& url, const std::string& method = "GET",
                                 const std::map<std::string, std::string>& params = {},
                                 const std::map<std::string, std::string>& headers = {});
    std::string make_post_request(const std::string& url, const std::string& data,
                                 const std::map<std::string, std::string>& headers = {});
    nlohmann::json parse_json_response(const std::string& response);
    
    // Utility methods
    std::string type_to_string(CaptchaType type) const;
    CaptchaType string_to_type(const std::string& type_str) const;
    std::string status_to_string(CaptchaStatus status) const;
    CaptchaStatus string_to_status(const std::string& status_str) const;
    std::string encode_base64(const std::string& data) const;
};

// 2Captcha provider implementation
class TwoCaptchaProvider : public CaptchaProviderInterface {
public:
    bool initialize(const CaptchaProviderConfig& config) override;
    std::shared_ptr<CaptchaTask> solve_captcha(const CaptchaType type,
                                               const std::map<std::string, std::string>& params) override;
    CaptchaStatus check_status(const std::string& task_id) override;
    std::string get_solution(const std::string& task_id) override;
    bool cancel_task(const std::string& task_id) override;
    
    std::string get_provider_name() const override { return "2Captcha"; }
    std::vector<CaptchaType> get_supported_types() const override;
    double get_cost_per_solve(CaptchaType type) const override;
    std::map<std::string, std::string> get_provider_info() const override;
    
    bool test_connection() override;
    double get_balance() const override;
    std::map<std::string, double> get_statistics() const override;

private:
    std::string build_api_url(const std::string& endpoint) const;
    std::map<std::string, std::string> get_default_params() const;
    std::string build_captcha_params(const CaptchaType type, 
                                     const std::map<std::string, std::string>& params) const;
};

// Anti-Captcha provider implementation
class AntiCaptchaProvider : public CaptchaProviderInterface {
public:
    bool initialize(const CaptchaProviderConfig& config) override;
    std::shared_ptr<CaptchaTask> solve_captcha(const CaptchaType type,
                                               const std::map<std::string, std::string>& params) override;
    CaptchaStatus check_status(const std::string& task_id) override;
    std::string get_solution(const std::string& task_id) override;
    bool cancel_task(const std::string& task_id) override;
    
    std::string get_provider_name() const override { return "Anti-Captcha"; }
    std::vector<CaptchaType> get_supported_types() const override;
    double get_cost_per_solve(CaptchaType type) const override;
    std::map<std::string, std::string> get_provider_info() const override;
    
    bool test_connection() override;
    double get_balance() const override;
    std::map<std::string, double> get_statistics() const override;

private:
    std::string build_api_url(const std::string& endpoint) const;
    nlohmann::json build_task_payload(const CaptchaType type,
                                    const std::map<std::string, std::string>& params) const;
};

// CapSolver provider implementation
class CapSolverProvider : public CaptchaProviderInterface {
public:
    bool initialize(const CaptchaProviderConfig& config) override;
    std::shared_ptr<CaptchaTask> solve_captcha(const CaptchaType type,
                                               const std::map<std::string, std::string>& params) override;
    CaptchaStatus check_status(const std::string& task_id) override;
    std::string get_solution(const std::string& task_id) override;
    bool cancel_task(const std::string& task_id) override;
    
    std::string get_provider_name() const override { return "CapSolver"; }
    std::vector<CaptchaType> get_supported_types() const override;
    double get_cost_per_solve(CaptchaType type) const override;
    std::map<std::string, std::string> get_provider_info() const override;
    
    bool test_connection() override;
    double get_balance() const override;
    std::map<std::string, double> get_statistics() const override;

private:
    std::string build_api_url(const std::string& endpoint) const;
    nlohmann::json build_capsolver_task(const CaptchaType type,
                                       const std::map<std::string, std::string>& params) const;
};

class CaptchaSolver {
public:
    CaptchaSolver();
    ~CaptchaSolver();
    
    // Initialization
    bool initialize(const std::string& config_file = "config/captcha_solver.yaml");
    void shutdown();
    
    // Provider management
    bool register_provider(std::unique_ptr<CaptchaProviderInterface> provider);
    bool set_primary_provider(CaptchaProvider provider);
    std::vector<CaptchaProvider> get_available_providers();
    
    // Captcha solving operations
    std::shared_ptr<CaptchaTask> solve_captcha(const CaptchaType type,
                                              const std::map<std::string, std::string>& params);
    std::shared_ptr<CaptchaTask> solve_image_captcha(const std::string& image_data,
                                                    const std::string& website_url = "");
    std::shared_ptr<CaptchaTask> solve_recaptcha_v2(const std::string& website_url,
                                                   const std::string& website_key,
                                                   const std::string& proxy = "");
    std::shared_ptr<CaptchaTask> solve_recaptcha_v3(const std::string& website_url,
                                                   const std::string& website_key,
                                                   const std::string& action = "verify",
                                                   const std::string& proxy = "");
    std::shared_ptr<CaptchaTask> solve_hcaptcha(const std::string& website_url,
                                               const std::string& website_key,
                                               const std::string& proxy = "");
    std::shared_ptr<CaptchaTask> solve_funcaptcha(const std::string& website_url,
                                                 const std::string& website_key,
                                                 const std::string& proxy = "");
    
    // Async solving
    std::future<std::shared_ptr<CaptchaTask>> solve_captcha_async(const CaptchaType type,
                                                                 const std::map<std::string, std::string>& params);
    
    // Batch operations
    std::vector<std::shared_ptr<CaptchaTask>> solve_multiple_captchas(
        const std::vector<std::pair<CaptchaType, std::map<std::string, std::string>>>& tasks);
    
    // Task management
    CaptchaStatus check_task_status(const std::string& task_id);
    std::string get_task_solution(const std::string& task_id);
    bool cancel_task(const std::string& task_id);
    std::shared_ptr<CaptchaTask> get_task_info(const std::string& task_id);
    
    // Provider rotation and load balancing
    void enable_provider_rotation(bool enable);
    void set_rotation_strategy(const std::string& strategy);
    CaptchaProvider get_next_provider();
    
    // Monitoring and statistics
    std::map<CaptchaProvider, double> get_provider_balances();
    std::map<CaptchaProvider, size_t> get_provider_usage_stats();
    std::map<CaptchaType, size_t> get_type_distribution();
    std::map<CaptchaStatus, size_t> get_status_distribution();
    double get_total_cost() const;
    double get_success_rate() const;
    std::chrono::milliseconds get_average_solve_time() const;
    
    // Configuration
    void set_default_timeout(int seconds);
    void set_max_retries(int retries);
    void set_retry_delay(std::chrono::seconds delay);
    void set_cost_limit(double daily_limit);
    
    // Queue management
    void enqueue_task(const CaptchaType type, const std::map<std::string, std::string>& params);
    std::shared_ptr<CaptchaTask> dequeue_task();
    size_t get_queue_size() const;
    void clear_queue();
    
    // History and logging
    std::vector<std::shared_ptr<CaptchaTask>> get_recent_tasks(size_t limit = 100);
    bool export_history(const std::string& filename) const;
    bool import_history(const std::string& filename);
    
    // Health checking
    void start_health_checking();
    void stop_health_checking();
    std::map<CaptchaProvider, bool> get_provider_health_status();
    
    // Cost management
    double get_daily_cost() const;
    bool is_within_cost_limit() const;
    std::vector<std::shared_ptr<CaptchaTask>> get_cost_breakdown() const;

private:
    // Provider management
    std::map<CaptchaProvider, std::unique_ptr<CaptchaProviderInterface>> providers_;
    CaptchaProvider primary_provider_{CaptchaProvider::TWO_CAPTCHA};
    std::vector<CaptchaProvider> provider_rotation_order_;
    std::map<CaptchaProvider, bool> provider_health_;
    std::map<CaptchaProvider, std::chrono::system_clock::time_point> last_provider_check_;
    
    // Configuration
    int default_timeout_{120};
    int max_retries_{3};
    std::chrono::seconds retry_delay_{10};
    double cost_limit_{50.0}; // Daily cost limit
    
    // Statistics
    std::map<CaptchaProvider, size_t> provider_usage_;
    std::map<CaptchaType, size_t> type_counts_;
    std::map<CaptchaStatus, size_t> status_counts_;
    std::atomic<double> total_cost_{0.0};
    std::atomic<size_t> total_tasks_{0};
    std::atomic<size_t> successful_tasks_{0};
    std::atomic<std::chrono::milliseconds::rep> total_solve_time_ms_{0};
    
    // Task queue
    std::queue<std::pair<CaptchaType, std::map<std::string, std::string>>> task_queue_;
    mutable std::mutex queue_mutex_;
    std::condition_variable queue_cv_;
    
    // Task history
    std::vector<std::shared_ptr<CaptchaTask>> task_history_;
    mutable std::mutex history_mutex_;
    
    // Active tasks
    std::map<std::string, std::shared_ptr<CaptchaTask>> active_tasks_;
    mutable std::mutex active_mutex_;
    
    // Health checking
    std::unique_ptr<std::thread> health_check_thread_;
    std::atomic<bool> health_checking_running_{false};
    std::chrono::seconds health_check_interval_{300}; // 5 minutes
    
    // Thread safety
    mutable std::mutex solver_mutex_;
    std::atomic<bool> provider_rotation_enabled_{false};
    std::string rotation_strategy_{"round_robin"};
    std::atomic<size_t> current_provider_index_{0};
    
    // Private methods
    bool load_configuration(const std::string& config_file);
    void initialize_default_providers();
    CaptchaProviderInterface* get_current_provider();
    CaptchaProviderInterface* get_provider_by_enum(CaptchaProvider provider);
    
    // Task management
    std::shared_ptr<CaptchaTask> execute_task(const CaptchaType type,
                                             const std::map<std::string, std::string>& params,
                                             CaptchaProviderInterface* provider);
    bool retry_task(std::shared_ptr<CaptchaTask> task, CaptchaProviderInterface* provider);
    void update_statistics(CaptchaStatus status, double cost, std::chrono::milliseconds solve_time);
    
    // Provider rotation
    void build_rotation_order();
    CaptchaProvider select_next_provider();
    bool is_provider_healthy(CaptchaProvider provider);
    
    // Health checking
    void health_check_loop();
    void check_provider_health(CaptchaProvider provider);
    
    // Queue processing
    void process_queue();
    std::thread queue_processor_;
    std::atomic<bool> queue_processing_running_{false};
    
    // Cost tracking
    void track_cost(double cost, CaptchaProvider provider);
    void reset_daily_cost();
    std::chrono::system_clock::time_point last_cost_reset_;
    
    // Utility methods
    std::string generate_task_id() const;
    void log_captcha_info(const std::string& message) const;
    void log_captcha_error(const std::string& message) const;
    void log_captcha_debug(const std::string& message) const;
};

// Captcha solving result
struct CaptchaSolveResult {
    bool success;
    std::string solution;
    CaptchaProvider provider;
    CaptchaType type;
    std::chrono::milliseconds solve_time;
    double cost;
    std::string error_message;
    
    nlohmann::json to_json() const;
    void from_json(const nlohmann::json& j);
};

// Captcha solving orchestrator
class CaptchaSolvingOrchestrator {
public:
    CaptchaSolvingOrchestrator(std::shared_ptr<CaptchaSolver> solver);
    ~CaptchaSolvingOrchestrator() = default;
    
    // High-level solving workflows
    CaptchaSolveResult solve_captcha_with_retry(const CaptchaType type,
                                                const std::map<std::string, std::string>& params,
                                                int max_retries = 3,
                                                std::chrono::seconds retry_delay = std::chrono::seconds(30));
    
    // Async solving
    std::future<CaptchaSolveResult> solve_captcha_async(const CaptchaType type,
                                                        const std::map<std::string, std::string>& params);
    
    // Batch solving
    std::vector<CaptchaSolveResult> solve_multiple_captchas(
        const std::vector<std::pair<CaptchaType, std::map<std::string, std::string>>>& tasks);

private:
    std::shared_ptr<CaptchaSolver> solver_;
    
    CaptchaSolveResult execute_solving_workflow(const CaptchaType type,
                                               const std::map<std::string, std::string>& params);
    bool wait_for_solution(std::shared_ptr<CaptchaTask> task,
                          std::chrono::seconds timeout,
                          std::string& solution);
};

// Captcha detector for automatic captcha detection
class CaptchaDetector {
public:
    CaptchaDetector();
    ~CaptchaDetector() = default;
    
    // Detection methods
    CaptchaType detect_captcha_type(const std::string& page_content);
    CaptchaType detect_from_url(const std::string& url);
    std::map<std::string, std::string> extract_captcha_params(const std::string& page_content);
    
    // reCAPTCHA detection
    bool has_recaptcha_v2(const std::string& page_content);
    bool has_recaptcha_v3(const std::string& page_content);
    std::string extract_recaptcha_site_key(const std::string& page_content);
    
    // hCaptcha detection
    bool has_hcaptcha(const std::string& page_content);
    std::string extract_hcaptcha_site_key(const std::string& page_content);
    
    // FunCaptcha detection
    bool has_funcaptcha(const std::string& page_content);
    std::string extract_funcaptcha_public_key(const std::string& page_content);
    
    // Image captcha detection
    bool has_image_captcha(const std::string& page_content);
    std::string extract_image_captcha_url(const std::string& page_content);

private:
    std::vector<std::regex> captcha_patterns_;
    std::vector<std::regex> recaptcha_patterns_;
    std::vector<std::regex> hcaptcha_patterns_;
    std::vector<std::regex> funcaptcha_patterns_;
    
    void initialize_patterns();
    bool matches_patterns(const std::string& content, const std::vector<std::regex>& patterns);
    std::string extract_with_regex(const std::string& content, const std::regex& pattern, int group = 1);
};

} // namespace gmail_infinity
