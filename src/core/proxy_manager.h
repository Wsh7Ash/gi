#pragma once

#include <string>
#include <vector>
#include <memory>
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <map>
#include <chrono>

#include <boost/asio/thread_pool.hpp>
#include <boost/asio/post.hpp>
#include <curl/curl.h>

namespace gmail_infinity {

enum class ProxyType {
    HTTP,
    HTTPS,
    SOCKS4,
    SOCKS5,
    RESIDENTIAL,
    MOBILE,
    IPv6
};

enum class ProxyStatus {
    UNKNOWN,
    WORKING,
    FAILED,
    BANNED,
    TIMEOUT
};

struct Proxy {
    std::string id;
    std::string host;
    uint16_t port;
    std::string username;
    std::string password;
    ProxyType type;
    std::string country;
    std::string provider;
    ProxyStatus status;
    std::chrono::system_clock::time_point last_checked;
    std::chrono::system_clock::time_point last_used;
    std::atomic<int> success_count{0};
    std::atomic<int> failure_count{0};
    std::atomic<int> timeout_count{0};
    std::chrono::milliseconds response_time{0};
    std::map<std::string, std::string> metadata;
    
    double get_health_score() const;
    std::string to_string() const;
    bool is_available() const;
    
    nlohmann::json to_json() const;
    void from_json(const nlohmann::json& j);
};

class ProxyManager {
public:
    ProxyManager();
    ~ProxyManager();
    
    // Initialization
    bool initialize(const std::string& config_file = "config/settings.yaml");
    void shutdown();
    
    // Proxy loading
    bool load_from_file(const std::string& filename);
    bool load_from_api(const std::string& api_url, const std::string& api_key);
    bool add_proxy(const Proxy& proxy);
    bool remove_proxy(const std::string& proxy_id);
    
    // Proxy selection
    std::shared_ptr<Proxy> get_next_proxy();
    std::shared_ptr<Proxy> get_proxy_by_country(const std::string& country);
    std::shared_ptr<Proxy> get_proxy_by_type(ProxyType type);
    std::shared_ptr<Proxy> get_healthiest_proxy();
    
    // Proxy rotation
    void mark_proxy_used(const std::string& proxy_id, bool success);
    void mark_proxy_failed(const std::string& proxy_id, const std::string& reason);
    void rotate_proxy_strategy(const std::string& strategy);
    
    // Health checking
    void start_health_checking();
    void stop_health_checking();
    void check_proxy_health(const std::string& proxy_id);
    void check_all_proxies();
    
    // Statistics
    size_t get_total_proxies() const;
    size_t get_working_proxies() const;
    size_t get_failed_proxies() const;
    double get_average_health_score() const;
    std::vector<std::shared_ptr<Proxy>> get_all_proxies() const;
    
    // Proxy anonymization
    std::shared_ptr<Proxy> get_anonymized_proxy();
    bool anonymize_proxy(const std::string& proxy_id);
    
    // Proxy fetching from APIs
    bool fetch_residential_proxies(const std::string& provider, const std::string& api_key);
    bool fetch_mobile_proxies(const std::string& provider, const std::string& api_key);
    bool fetch_ipv6_proxies(const std::string& provider, const std::string& api_key);
    
    // Configuration
    void set_health_check_interval(std::chrono::seconds interval);
    void set_max_failures_before_ban(int max_failures);
    void set_proxy_timeout(std::chrono::milliseconds timeout);
    
    // Export/Import
    bool export_proxies(const std::string& filename) const;
    bool import_proxies(const std::string& filename);
    
    // Thread safety
    void lock_proxy_for_use(const std::string& proxy_id);
    void unlock_proxy(const std::string& proxy_id);
    bool is_proxy_locked(const std::string& proxy_id) const;

private:
    // Internal storage
    mutable std::mutex proxies_mutex_;
    std::map<std::string, std::shared_ptr<Proxy>> proxies_;
    std::queue<std::string> proxy_rotation_queue_;
    std::map<std::string, bool> locked_proxies_;
    
    // Health checking
    std::unique_ptr<std::thread> health_check_thread_;
    std::atomic<bool> health_checking_running_{false};
    std::chrono::seconds health_check_interval_{300}; // 5 minutes
    std::condition_variable health_check_cv_;
    std::mutex health_check_mutex_;
    
    // Thread pool for async operations
    std::unique_ptr<boost::asio::thread_pool> thread_pool_;
    
    // Configuration
    std::atomic<int> max_failures_before_ban_{5};
    std::atomic<std::chrono::milliseconds::rep> proxy_timeout_ms_{30000};
    std::string rotation_strategy_{"round_robin"};
    std::string last_country_used_;
    
    // Private methods
    void health_check_loop();
    bool test_proxy_connection(const Proxy& proxy);
    bool test_proxy_with_curl(const Proxy& proxy, const std::string& test_url);
    void update_proxy_health(const std::string& proxy_id, bool success, 
                           std::chrono::milliseconds response_time);
    void rebuild_rotation_queue();
    std::string generate_proxy_id() const;
    ProxyType parse_proxy_type(const std::string& type_str) const;
    std::string proxy_type_to_string(ProxyType type) const;
    
    // API helpers
    std::vector<Proxy> parse_residential_api_response(const std::string& response);
    std::vector<Proxy> parse_mobile_api_response(const std::string& response);
    std::vector<Proxy> parse_ipv6_api_response(const std::string& response);
    
    // Utility methods
    void log_proxy_info(const std::string& message) const;
    void log_proxy_error(const std::string& message) const;
    void log_proxy_debug(const std::string& message) const;
};

// Proxy health checker class for dedicated health checking
class ProxyHealthChecker {
public:
    explicit ProxyHealthChecker(std::shared_ptr<ProxyManager> manager);
    ~ProxyHealthChecker();
    
    void start();
    void stop();
    void check_proxy(const std::string& proxy_id);
    
private:
    std::shared_ptr<ProxyManager> manager_;
    std::unique_ptr<std::thread> checker_thread_;
    std::atomic<bool> running_{false};
    std::queue<std::string> check_queue_;
    std::mutex queue_mutex_;
    std::condition_variable queue_cv_;
    
    void checker_loop();
};

// Residential proxy fetcher
class ResidentialProxyFetcher {
public:
    explicit ResidentialProxyFetcher(const std::string& provider, const std::string& api_key);
    std::vector<Proxy> fetch_proxies();
    
private:
    std::string provider_;
    std::string api_key_;
    
    std::string make_api_request(const std::string& endpoint);
    std::vector<Proxy> parse_response(const std::string& response);
};

// Mobile proxy fetcher
class MobileProxyFetcher {
public:
    explicit MobileProxyFetcher(const std::string& provider, const std::string& api_key);
    std::vector<Proxy> fetch_proxies();
    
private:
    std::string provider_;
    std::string api_key_;
    
    std::string make_api_request(const std::string& endpoint);
    std::vector<Proxy> parse_response(const std::string& response);
};

// IPv6 proxy rotator
class IPv6Rotator {
public:
    explicit IPv6Rotator(const std::string& provider, const std::string& api_key);
    std::vector<Proxy> get_ipv6_pool();
    bool rotate_ipv6_address(const std::string& proxy_id);
    
private:
    std::string provider_;
    std::string api_key_;
    std::map<std::string, std::vector<std::string>> ipv6_pools_;
    
    bool fetch_ipv6_pool();
    std::string make_api_request(const std::string& endpoint);
};

// Proxy anonymizer
class ProxyAnonymizer {
public:
    explicit ProxyAnonymizer();
    std::shared_ptr<Proxy> anonymize_proxy(std::shared_ptr<Proxy> original_proxy);
    
private:
    std::vector<std::string> user_agents_;
    std::vector<std::string> referers_;
    
    void load_user_agents();
    void load_referers();
    std::string get_random_user_agent() const;
    std::string get_random_referer() const;
};

} // namespace gmail_infinity
