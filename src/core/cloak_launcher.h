#pragma once

#include <string>
#include <vector>
#include <memory>
#include <atomic>
#include <chrono>
#include <functional>
#include <map>

namespace gmail_infinity {

struct CloakProfile {
    std::string profile_id;
    std::string browser_type{"chrome"};
    std::string os_type{"windows"};
    std::string proxy_url;
    std::string user_agent;
    int screen_width{1920};
    int screen_height{1080};
    std::string timezone{"America/New_York"};
    std::string language{"en-US"};
    std::map<std::string, std::string> extra_args;
};

enum class CloakStatus {
    STOPPED,
    STARTING,
    RUNNING,
    CRASHED,
    TIMEOUT
};

class CloakLauncher {
public:
    CloakLauncher();
    ~CloakLauncher();

    // Browser lifecycle
    bool launch(const CloakProfile& profile);
    void close();
    bool is_running() const;
    CloakStatus get_status() const;

    // CDP access
    std::string get_cdp_url() const;
    uint16_t get_debug_port() const;

    // Profile management
    std::string create_profile(const CloakProfile& profile);
    bool delete_profile(const std::string& profile_id);
    std::vector<std::string> list_profiles() const;

    // Health
    bool ping_debug_port() const;
    std::string get_last_error() const;

    // Binary discovery
    static std::string find_cloak_binary();
    static std::string find_chrome_binary();
    static bool is_cloak_available();

private:
    std::string binary_path_;
    CloakProfile active_profile_;
    std::atomic<CloakStatus> status_{CloakStatus::STOPPED};
    uint16_t debug_port_{9222};
    int process_id_{-1};
    std::string last_error_;

    bool start_process(const std::vector<std::string>& args);
    bool wait_for_port(std::chrono::seconds timeout = std::chrono::seconds(30));
    std::vector<std::string> build_chrome_args(const CloakProfile& profile) const;
    uint16_t find_free_port() const;

#ifdef _WIN32
    void* process_handle_{nullptr};
#endif
};

} // namespace gmail_infinity
