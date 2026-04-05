#include "proxy_manager.h"
#include "app/app_logger.h"
#include <yaml-cpp/yaml.h>
#include <fstream>
#include <sstream>
#include <random>
#include <algorithm>
#include <curl/curl.h>

namespace gmail_infinity {

// Proxy implementation
double Proxy::get_health_score() const {
    if (failure_count.load() == 0 && success_count.load() == 0) {
        return 50.0; // Neutral score for unused proxies
    }
    
    int total_attempts = success_count.load() + failure_count.load() + timeout_count.load();
    if (total_attempts == 0) return 50.0;
    
    double success_rate = static_cast<double>(success_count.load()) / total_attempts * 100.0;
    
    // Penalize timeouts heavily
    double timeout_penalty = static_cast<double>(timeout_count.load()) / total_attempts * 50.0;
    
    // Factor in response time (lower is better)
    double response_time_score = std::max(0.0, 100.0 - (response_time.count() / 1000.0) * 10.0);
    
    // Weighted average
    double final_score = (success_rate * 0.6) + (response_time_score * 0.3) - timeout_penalty;
    
    return std::max(0.0, std::min(100.0, final_score));
}

std::string Proxy::to_string() const {
    std::ostringstream ss;
    ss << host << ":" << port;
    if (!username.empty()) {
        ss << " (" << username << ":***)";
    }
    ss << " [" << proxy_type_to_string(type) << "] - " 
       << "Health: " << std::fixed << std::setprecision(1) << get_health_score() << "%";
    return ss.str();
}

bool Proxy::is_available() const {
    return status == ProxyStatus::WORKING && 
           !locked_proxies_.count(id) &&
           failure_count.load() < max_failures_before_ban_;
}

nlohmann::json Proxy::to_json() const {
    nlohmann::json j;
    j["id"] = id;
    j["host"] = host;
    j["port"] = port;
    j["username"] = username;
    j["password"] = password; // In production, this should be encrypted or omitted
    j["type"] = proxy_type_to_string(type);
    j["country"] = country;
    j["provider"] = provider;
    j["status"] = static_cast<int>(status);
    j["last_checked"] = std::chrono::duration_cast<std::chrono::seconds>(
        last_checked.time_since_epoch()).count();
    j["last_used"] = std::chrono::duration_cast<std::chrono::seconds>(
        last_used.time_since_epoch()).count();
    j["success_count"] = success_count.load();
    j["failure_count"] = failure_count.load();
    j["timeout_count"] = timeout_count.load();
    j["response_time_ms"] = response_time.count();
    j["metadata"] = metadata;
    return j;
}

void Proxy::from_json(const nlohmann::json& j) {
    if (j.contains("id")) id = j["id"].get<std::string>();
    if (j.contains("host")) host = j["host"].get<std::string>();
    if (j.contains("port")) port = j["port"].get<uint16_t>();
    if (j.contains("username")) username = j["username"].get<std::string>();
    if (j.contains("password")) password = j["password"].get<std::string>();
    if (j.contains("type")) type = parse_proxy_type(j["type"].get<std::string>());
    if (j.contains("country")) country = j["country"].get<std::string>();
    if (j.contains("provider")) provider = j["provider"].get<std::string>();
    if (j.contains("status")) status = static_cast<ProxyStatus>(j["status"].get<int>());
    if (j.contains("last_checked")) {
        last_checked = std::chrono::system_clock::from_time_t(j["last_checked"].get<int64_t>());
    }
    if (j.contains("last_used")) {
        last_used = std::chrono::system_clock::from_time_t(j["last_used"].get<int64_t>());
    }
    if (j.contains("success_count")) success_count = j["success_count"].get<int>();
    if (j.contains("failure_count")) failure_count = j["failure_count"].get<int>();
    if (j.contains("timeout_count")) timeout_count = j["timeout_count"].get<int>();
    if (j.contains("response_time_ms")) response_time = std::chrono::milliseconds(j["response_time_ms"].get<int>());
    if (j.contains("metadata")) metadata = j["metadata"].get<std::map<std::string, std::string>>();
}

// ProxyManager implementation
ProxyManager::ProxyManager() {
    curl_global_init(CURL_GLOBAL_DEFAULT);
    thread_pool_ = std::make_unique<boost::asio::thread_pool>(4);
}

ProxyManager::~ProxyManager() {
    shutdown();
    curl_global_cleanup();
}

bool ProxyManager::initialize(const std::string& config_file) {
    try {
        YAML::Node config = YAML::LoadFile(config_file);
        
        if (config["proxies"]) {
            auto proxy_config = config["proxies"];
            
            if (proxy_config["residential_file"]) {
                load_from_file(proxy_config["residential_file"].as<std::string>());
            }
            
            if (proxy_config["health_check_interval"]) {
                health_check_interval_ = std::chrono::seconds(
                    proxy_config["health_check_interval"].as<int>());
            }
            
            if (proxy_config["max_failures_before_ban"]) {
                max_failures_before_ban_ = proxy_config["max_failures_before_ban"].as<int>();
            }
            
            if (proxy_config["proxy_timeout_ms"]) {
                proxy_timeout_ms_ = proxy_config["proxy_timeout_ms"].as<int>();
            }
            
            if (proxy_config["rotation_strategy"]) {
                rotation_strategy_ = proxy_config["rotation_strategy"].as<std::string>();
            }
        }
        
        log_proxy_info("Proxy manager initialized with " + std::to_string(proxies_.size()) + " proxies");
        return true;
        
    } catch (const std::exception& e) {
        log_proxy_error("Failed to initialize proxy manager: " + std::string(e.what()));
        return false;
    }
}

void ProxyManager::shutdown() {
    stop_health_checking();
    
    if (thread_pool_) {
        thread_pool_->join();
        thread_pool_.reset();
    }
    
    log_proxy_info("Proxy manager shutdown complete");
}

bool ProxyManager::load_from_file(const std::string& filename) {
    std::lock_guard<std::mutex> lock(proxies_mutex_);
    
    try {
        std::ifstream file(filename);
        if (!file.is_open()) {
            log_proxy_error("Failed to open proxy file: " + filename);
            return false;
        }
        
        std::string line;
        while (std::getline(file, line)) {
            // Skip empty lines and comments
            if (line.empty() || line[0] == '#') continue;
            
            // Parse proxy format: ip:port:username:password:type:country
            std::istringstream ss(line);
            std::string token;
            std::vector<std::string> tokens;
            
            while (std::getline(ss, token, ':')) {
                tokens.push_back(token);
            }
            
            if (tokens.size() >= 2) {
                Proxy proxy;
                proxy.id = generate_proxy_id();
                proxy.host = tokens[0];
                proxy.port = static_cast<uint16_t>(std::stoul(tokens[1]));
                
                if (tokens.size() > 2) proxy.username = tokens[2];
                if (tokens.size() > 3) proxy.password = tokens[3];
                if (tokens.size() > 4) proxy.type = parse_proxy_type(tokens[4]);
                if (tokens.size() > 5) proxy.country = tokens[5];
                
                proxy.status = ProxyStatus::UNKNOWN;
                proxy.last_checked = std::chrono::system_clock::now();
                proxy.last_used = std::chrono::system_clock::now();
                
                proxies_[proxy.id] = std::make_shared<Proxy>(proxy);
            }
        }
        
        file.close();
        rebuild_rotation_queue();
        
        log_proxy_info("Loaded " + std::to_string(proxies_.size()) + " proxies from " + filename);
        return true;
        
    } catch (const std::exception& e) {
        log_proxy_error("Failed to load proxies from file: " + std::string(e.what()));
        return false;
    }
}

bool ProxyManager::add_proxy(const Proxy& proxy) {
    std::lock_guard<std::mutex> lock(proxies_mutex_);
    
    Proxy new_proxy = proxy;
    if (new_proxy.id.empty()) {
        new_proxy.id = generate_proxy_id();
    }
    
    new_proxy.last_checked = std::chrono::system_clock::now();
    new_proxy.last_used = std::chrono::system_clock::now();
    
    proxies_[new_proxy.id] = std::make_shared<Proxy>(new_proxy);
    rebuild_rotation_queue();
    
    log_proxy_info("Added proxy: " + new_proxy.to_string());
    return true;
}

bool ProxyManager::remove_proxy(const std::string& proxy_id) {
    std::lock_guard<std::mutex> lock(proxies_mutex_);
    
    if (proxies_.erase(proxy_id) > 0) {
        rebuild_rotation_queue();
        log_proxy_info("Removed proxy: " + proxy_id);
        return true;
    }
    
    log_proxy_error("Proxy not found for removal: " + proxy_id);
    return false;
}

std::shared_ptr<Proxy> ProxyManager::get_next_proxy() {
    std::lock_guard<std::mutex> lock(proxies_mutex_);
    
    if (proxy_rotation_queue_.empty()) {
        rebuild_rotation_queue();
    }
    
    while (!proxy_rotation_queue_.empty()) {
        std::string proxy_id = proxy_rotation_queue_.front();
        proxy_rotation_queue_.pop();
        
        auto it = proxies_.find(proxy_id);
        if (it != proxies_.end() && it->second->is_available()) {
            return it->second;
        }
    }
    
    log_proxy_warn("No available proxies found");
    return nullptr;
}

std::shared_ptr<Proxy> ProxyManager::get_healthiest_proxy() {
    std::lock_guard<std::mutex> lock(proxies_mutex_);
    
    std::shared_ptr<Proxy> best_proxy = nullptr;
    double best_score = -1.0;
    
    for (const auto& [id, proxy] : proxies_) {
        if (proxy->is_available()) {
            double score = proxy->get_health_score();
            if (score > best_score) {
                best_score = score;
                best_proxy = proxy;
            }
        }
    }
    
    return best_proxy;
}

void ProxyManager::mark_proxy_used(const std::string& proxy_id, bool success) {
    std::lock_guard<std::mutex> lock(proxies_mutex_);
    
    auto it = proxies_.find(proxy_id);
    if (it != proxies_.end()) {
        auto& proxy = it->second;
        proxy->last_used = std::chrono::system_clock::now();
        
        if (success) {
            proxy->success_count++;
            proxy->status = ProxyStatus::WORKING;
        } else {
            proxy->failure_count++;
            if (proxy->failure_count.load() >= max_failures_before_ban_) {
                proxy->status = ProxyStatus::BANNED;
            }
        }
        
        log_proxy_debug("Marked proxy " + proxy_id + " as " + (success ? "success" : "failed"));
    }
}

void ProxyManager::start_health_checking() {
    if (health_checking_running_.exchange(true)) {
        return; // Already running
    }
    
    health_check_thread_ = std::make_unique<std::thread>(&ProxyManager::health_check_loop, this);
    log_proxy_info("Started proxy health checking");
}

void ProxyManager::stop_health_checking() {
    if (!health_checking_running_.exchange(false)) {
        return; // Not running
    }
    
    health_check_cv_.notify_all();
    
    if (health_check_thread_ && health_check_thread_->joinable()) {
        health_check_thread_->join();
    }
    
    health_check_thread_.reset();
    log_proxy_info("Stopped proxy health checking");
}

void ProxyManager::health_check_loop() {
    while (health_checking_running_) {
        try {
            check_all_proxies();
            
            std::unique_lock<std::mutex> lock(health_check_mutex_);
            health_check_cv_.wait_for(lock, health_check_interval_, 
                [this] { return !health_checking_running_; });
            
        } catch (const std::exception& e) {
            log_proxy_error("Health check loop error: " + std::string(e.what()));
            std::this_thread::sleep_for(std::chrono::seconds(30));
        }
    }
}

void ProxyManager::check_all_proxies() {
    std::vector<std::string> proxy_ids;
    
    {
        std::lock_guard<std::mutex> lock(proxies_mutex_);
        proxy_ids.reserve(proxies_.size());
        
        for (const auto& [id, proxy] : proxies_) {
            proxy_ids.push_back(id);
        }
    }
    
    // Check proxies in parallel
    for (const std::string& proxy_id : proxy_ids) {
        boost::asio::post(*thread_pool_, [this, proxy_id]() {
            check_proxy_health(proxy_id);
        });
    }
}

void ProxyManager::check_proxy_health(const std::string& proxy_id) {
    auto start_time = std::chrono::steady_clock::now();
    
    std::shared_ptr<Proxy> proxy;
    {
        std::lock_guard<std::mutex> lock(proxies_mutex_);
        auto it = proxies_.find(proxy_id);
        if (it == proxies_.end()) return;
        proxy = it->second;
    }
    
    bool success = test_proxy_connection(*proxy);
    auto end_time = std::chrono::steady_clock::now();
    auto response_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    update_proxy_health(proxy_id, success, response_time);
}

bool ProxyManager::test_proxy_connection(const Proxy& proxy) {
    return test_proxy_with_curl(proxy, "http://httpbin.org/ip");
}

bool ProxyManager::test_proxy_with_curl(const Proxy& proxy, const std::string& test_url) {
    CURL* curl = curl_easy_init();
    if (!curl) return false;
    
    std::string response_string;
    std::string header_string;
    
    // Set proxy
    std::string proxy_url;
    switch (proxy.type) {
        case ProxyType::HTTP:
        case ProxyType::HTTPS:
            proxy_url = "http://" + proxy.host + ":" + std::to_string(proxy.port);
            break;
        case ProxyType::SOCKS4:
            proxy_url = "socks4://" + proxy.host + ":" + std::to_string(proxy.port);
            break;
        case ProxyType::SOCKS5:
            proxy_url = "socks5://" + proxy.host + ":" + std::to_string(proxy.port);
            break;
        default:
            proxy_url = "http://" + proxy.host + ":" + std::to_string(proxy.port);
            break;
    }
    
    curl_easy_setopt(curl, CURLOPT_PROXY, proxy_url.c_str());
    
    if (!proxy.username.empty()) {
        std::string userpwd = proxy.username + ":" + proxy.password;
        curl_easy_setopt(curl, CURLOPT_PROXYUSERPWD, userpwd.c_str());
    }
    
    curl_easy_setopt(curl, CURLOPT_URL, test_url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, [](void* contents, size_t size, size_t nmemb, std::string* userp) {
        userp->append((char*)contents, size * nmemb);
        return size * nmemb;
    });
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_string);
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, &header_string);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, proxy_timeout_ms_.load() / 1000);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
    
    CURLcode res = curl_easy_perform(curl);
    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    
    curl_easy_cleanup(curl);
    
    bool success = (res == CURLE_OK) && (http_code == 200);
    
    log_proxy_debug("Proxy test " + proxy.to_string() + " - " + 
                   (success ? "SUCCESS" : "FAILED") + " (HTTP " + std::to_string(http_code) + ")");
    
    return success;
}

void ProxyManager::update_proxy_health(const std::string& proxy_id, bool success, 
                                     std::chrono::milliseconds response_time) {
    std::lock_guard<std::mutex> lock(proxies_mutex_);
    
    auto it = proxies_.find(proxy_id);
    if (it == proxies_.end()) return;
    
    auto& proxy = it->second;
    proxy->last_checked = std::chrono::system_clock::now();
    proxy->response_time = response_time;
    
    if (success) {
        proxy->success_count++;
        proxy->status = ProxyStatus::WORKING;
    } else {
        proxy->failure_count++;
        if (response_time.count() >= proxy_timeout_ms_.load()) {
            proxy->timeout_count++;
        }
        
        if (proxy->failure_count.load() >= max_failures_before_ban_) {
            proxy->status = ProxyStatus::BANNED;
        } else {
            proxy->status = ProxyStatus::FAILED;
        }
    }
}

size_t ProxyManager::get_total_proxies() const {
    std::lock_guard<std::mutex> lock(proxies_mutex_);
    return proxies_.size();
}

size_t ProxyManager::get_working_proxies() const {
    std::lock_guard<std::mutex> lock(proxies_mutex_);
    
    size_t count = 0;
    for (const auto& [id, proxy] : proxies_) {
        if (proxy->status == ProxyStatus::WORKING) count++;
    }
    
    return count;
}

double ProxyManager::get_average_health_score() const {
    std::lock_guard<std::mutex> lock(proxies_mutex_);
    
    if (proxies_.empty()) return 0.0;
    
    double total_score = 0.0;
    for (const auto& [id, proxy] : proxies_) {
        total_score += proxy->get_health_score();
    }
    
    return total_score / proxies_.size();
}

void ProxyManager::rebuild_rotation_queue() {
    proxy_rotation_queue_ = std::queue<std::string>();
    
    std::vector<std::shared_ptr<Proxy>> available_proxies;
    for (const auto& [id, proxy] : proxies_) {
        if (proxy->is_available()) {
            available_proxies.push_back(proxy);
        }
    }
    
    // Sort by health score (descending)
    std::sort(available_proxies.begin(), available_proxies.end(),
        [](const std::shared_ptr<Proxy>& a, const std::shared_ptr<Proxy>& b) {
            return a->get_health_score() > b->get_health_score();
        });
    
    // Build rotation queue
    for (const auto& proxy : available_proxies) {
        proxy_rotation_queue_.push(proxy->id);
    }
}

std::string ProxyManager::generate_proxy_id() const {
    static std::atomic<int> counter{0};
    return "proxy_" + std::to_string(++counter);
}

ProxyType ProxyManager::parse_proxy_type(const std::string& type_str) const {
    if (type_str == "http" || type_str == "HTTP") return ProxyType::HTTP;
    if (type_str == "https" || type_str == "HTTPS") return ProxyType::HTTPS;
    if (type_str == "socks4" || type_str == "SOCKS4") return ProxyType::SOCKS4;
    if (type_str == "socks5" || type_str == "SOCKS5") return ProxyType::SOCKS5;
    if (type_str == "residential" || type_str == "RESIDENTIAL") return ProxyType::RESIDENTIAL;
    if (type_str == "mobile" || type_str == "MOBILE") return ProxyType::MOBILE;
    if (type_str == "ipv6" || type_str == "IPv6") return ProxyType::IPv6;
    
    return ProxyType::HTTP; // Default
}

std::string ProxyManager::proxy_type_to_string(ProxyType type) const {
    switch (type) {
        case ProxyType::HTTP: return "http";
        case ProxyType::HTTPS: return "https";
        case ProxyType::SOCKS4: return "socks4";
        case ProxyType::SOCKS5: return "socks5";
        case ProxyType::RESIDENTIAL: return "residential";
        case ProxyType::MOBILE: return "mobile";
        case ProxyType::IPv6: return "ipv6";
        default: return "unknown";
    }
}

void ProxyManager::log_proxy_info(const std::string& message) const {
    LOG_INFO("ProxyManager: {}", message);
}

void ProxyManager::log_proxy_error(const std::string& message) const {
    LOG_ERROR("ProxyManager: {}", message);
}

void ProxyManager::log_proxy_debug(const std::string& message) const {
    LOG_DEBUG("ProxyManager: {}", message);
}

void ProxyManager::log_proxy_warn(const std::string& message) const {
    LOG_WARN("ProxyManager: {}", message);
}

// ProxyHealthChecker implementation
ProxyHealthChecker::ProxyHealthChecker(std::shared_ptr<ProxyManager> manager)
    : manager_(manager) {}

ProxyHealthChecker::~ProxyHealthChecker() {
    stop();
}

void ProxyHealthChecker::start() {
    if (running_.exchange(true)) return;
    
    checker_thread_ = std::make_unique<std::thread>(&ProxyHealthChecker::checker_loop, this);
}

void ProxyHealthChecker::stop() {
    if (!running_.exchange(false)) return;
    
    queue_cv_.notify_all();
    
    if (checker_thread_ && checker_thread_->joinable()) {
        checker_thread_->join();
    }
}

void ProxyHealthChecker::check_proxy(const std::string& proxy_id) {
    {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        check_queue_.push(proxy_id);
    }
    queue_cv_.notify_one();
}

void ProxyHealthChecker::checker_loop() {
    while (running_) {
        std::unique_lock<std::mutex> lock(queue_mutex_);
        queue_cv_.wait(lock, [this] { return !check_queue_.empty() || !running_; });
        
        if (!running_) break;
        
        while (!check_queue_.empty()) {
            std::string proxy_id = check_queue_.front();
            check_queue_.pop();
            lock.unlock();
            
            if (manager_) {
                manager_->check_proxy_health(proxy_id);
            }
            
            lock.lock();
        }
    }
}

} // namespace gmail_infinity
