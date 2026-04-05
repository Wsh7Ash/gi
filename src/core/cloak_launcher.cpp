#include "cloak_launcher.h"
#include "app/app_logger.h"

#include <spdlog/spdlog.h>
#include <sstream>
#include <fstream>
#include <thread>
#include <chrono>
#include <random>
#include <stdexcept>
#include <filesystem>

#ifdef _WIN32
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#endif

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core/tcp_stream.hpp>

namespace gmail_infinity {

namespace fs = std::filesystem;
namespace net = boost::asio;
using tcp = net::ip::tcp;

CloakLauncher::CloakLauncher() {
    binary_path_ = find_cloak_binary();
    if (binary_path_.empty()) {
        binary_path_ = find_chrome_binary();
        spdlog::info("CloakBrowser not found, falling back to Chrome: {}", binary_path_);
    } else {
        spdlog::info("Using CloakBrowser: {}", binary_path_);
    }
}

CloakLauncher::~CloakLauncher() { close(); }

std::string CloakLauncher::find_cloak_binary() {
    // Common CloakBrowser install locations
    std::vector<std::string> candidates = {
        "C:/Program Files/CloakBrowser/CloakBrowser.exe",
        "C:/Program Files (x86)/CloakBrowser/CloakBrowser.exe",
        "/usr/bin/cloak-browser",
        "/opt/cloak-browser/cloak-browser",
    };
    for (auto& p : candidates) {
        if (fs::exists(p)) return p;
    }
    return "";
}

std::string CloakLauncher::find_chrome_binary() {
    std::vector<std::string> candidates = {
        "C:/Program Files/Google/Chrome/Application/chrome.exe",
        "C:/Program Files (x86)/Google/Chrome/Application/chrome.exe",
        "/usr/bin/google-chrome",
        "/usr/bin/chromium-browser",
        "/usr/bin/chromium",
        "/Applications/Google Chrome.app/Contents/MacOS/Google Chrome",
    };
    for (auto& p : candidates) {
        if (fs::exists(p)) return p;
    }
    return "chrome"; // fallback: hope it's on PATH
}

bool CloakLauncher::is_cloak_available() {
    return !find_cloak_binary().empty();
}

bool CloakLauncher::launch(const CloakProfile& profile) {
    if (status_ == CloakStatus::RUNNING) {
        spdlog::warn("CloakLauncher: already running");
        return true;
    }
    active_profile_ = profile;
    debug_port_ = find_free_port();
    status_     = CloakStatus::STARTING;

    auto args = build_chrome_args(profile);
    if (!start_process(args)) {
        status_      = CloakStatus::CRASHED;
        last_error_  = "Failed to start browser process";
        return false;
    }

    if (!wait_for_port(std::chrono::seconds(30))) {
        status_     = CloakStatus::TIMEOUT;
        last_error_ = "Timed out waiting for debug port";
        return false;
    }

    status_ = CloakStatus::RUNNING;
    spdlog::info("CloakLauncher: browser started on port {}", debug_port_);
    return true;
}

void CloakLauncher::close() {
    if (status_ == CloakStatus::STOPPED) return;
    status_ = CloakStatus::STOPPED;

#ifdef _WIN32
    if (process_handle_) {
        HANDLE h = static_cast<HANDLE>(process_handle_);
        TerminateProcess(h, 0);
        CloseHandle(h);
        process_handle_ = nullptr;
    }
#else
    if (process_id_ > 0) {
        ::kill(process_id_, SIGTERM);
        ::waitpid(process_id_, nullptr, 0);
        process_id_ = -1;
    }
#endif
    spdlog::info("CloakLauncher: browser closed");
}

bool CloakLauncher::is_running() const { return status_ == CloakStatus::RUNNING; }
CloakStatus CloakLauncher::get_status() const { return status_; }
std::string CloakLauncher::get_cdp_url() const {
    return "ws://127.0.0.1:" + std::to_string(debug_port_) + "/json";
}
uint16_t CloakLauncher::get_debug_port() const { return debug_port_; }
std::string CloakLauncher::get_last_error() const { return last_error_; }

bool CloakLauncher::ping_debug_port() const {
    try {
        net::io_context ioc;
        tcp::resolver resolver(ioc);
        auto endpoints = resolver.resolve("127.0.0.1", std::to_string(debug_port_));
        boost::beast::tcp_stream stream(ioc);
        stream.connect(*endpoints.begin());
        stream.socket().close();
        return true;
    } catch (...) {
        return false;
    }
}

std::vector<std::string> CloakLauncher::build_chrome_args(const CloakProfile& profile) const {
    std::vector<std::string> args;
    args.push_back(binary_path_);
    args.push_back("--remote-debugging-port=" + std::to_string(debug_port_));

    if (!profile.proxy_url.empty())
        args.push_back("--proxy-server=" + profile.proxy_url);
    if (!profile.user_agent.empty())
        args.push_back("--user-agent=" + profile.user_agent);

    args.push_back("--window-size=" + std::to_string(profile.screen_width)
                    + "," + std::to_string(profile.screen_height));
    args.push_back("--no-sandbox");
    args.push_back("--disable-setuid-sandbox");
    args.push_back("--disable-blink-features=AutomationControlled");
    args.push_back("--disable-infobars");
    args.push_back("--headless=new");
    args.push_back("--disable-gpu");

    for (auto& [k, v] : profile.extra_args) {
        args.push_back("--" + k + (v.empty() ? "" : "=" + v));
    }
    return args;
}

bool CloakLauncher::start_process(const std::vector<std::string>& args) {
    if (args.empty()) return false;

    // Build command string
    std::string cmd;
    for (auto& a : args) {
        if (a.find(' ') != std::string::npos) cmd += "\"" + a + "\" ";
        else cmd += a + " ";
    }

#ifdef _WIN32
    STARTUPINFOA si{};
    PROCESS_INFORMATION pi{};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    if (!CreateProcessA(nullptr, const_cast<char*>(cmd.c_str()),
                        nullptr, nullptr, FALSE, CREATE_NO_WINDOW,
                        nullptr, nullptr, &si, &pi)) {
        last_error_ = "CreateProcess failed: " + std::to_string(GetLastError());
        return false;
    }
    process_id_     = static_cast<int>(pi.dwProcessId);
    process_handle_ = pi.hProcess;
    CloseHandle(pi.hThread);
#else
    process_id_ = fork();
    if (process_id_ == 0) {
        std::vector<const char*> argv;
        for (auto& a : args) argv.push_back(a.c_str());
        argv.push_back(nullptr);
        execv(args[0].c_str(), const_cast<char* const*>(argv.data()));
        _exit(1);
    } else if (process_id_ < 0) {
        last_error_ = "fork() failed";
        return false;
    }
#endif
    return true;
}

bool CloakLauncher::wait_for_port(std::chrono::seconds timeout) {
    auto deadline = std::chrono::steady_clock::now() + timeout;
    while (std::chrono::steady_clock::now() < deadline) {
        if (ping_debug_port()) return true;
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
    return false;
}

uint16_t CloakLauncher::find_free_port() const {
    // Bind to port 0 and let the OS assign a free port
#ifdef _WIN32
    WSADATA wd; WSAStartup(MAKEWORD(2,2), &wd);
    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in addr{}; addr.sin_family = AF_INET; addr.sin_port = 0;
    addr.sin_addr.s_addr = INADDR_ANY;
    bind(sock, (sockaddr*)&addr, sizeof(addr));
    int len = sizeof(addr);
    getsockname(sock, (sockaddr*)&addr, &len);
    uint16_t port = ntohs(addr.sin_port);
    closesocket(sock);
    return port;
#else
    int sock = ::socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in addr{}; addr.sin_family = AF_INET; addr.sin_port = 0;
    addr.sin_addr.s_addr = INADDR_ANY;
    ::bind(sock, (sockaddr*)&addr, sizeof(addr));
    socklen_t len = sizeof(addr);
    ::getsockname(sock, (sockaddr*)&addr, &len);
    uint16_t port = ntohs(addr.sin_port);
    ::close(sock);
    return port ? port : 9222;
#endif
}

std::string CloakLauncher::create_profile(const CloakProfile& profile) {
    return profile.profile_id.empty() ? "default" : profile.profile_id;
}

bool CloakLauncher::delete_profile(const std::string& /*profile_id*/) { return true; }
std::vector<std::string> CloakLauncher::list_profiles() const { return {"default"}; }

} // namespace gmail_infinity
