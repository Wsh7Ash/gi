#include "browser_controller.h"
#include "app/app_logger.h"

#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>

#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>

#include <iostream>
#include <sstream>
#include <fstream>
#include <thread>
#include <chrono>
#include <stdexcept>
#include <cstdlib>
#include <random>
#include <algorithm>
#include <future>

#ifdef _WIN32
#include <windows.h>
#include <winsock2.h>
#else
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#endif

namespace gmail_infinity {

using json = nlohmann::json;
namespace beast = boost::beast;
namespace http  = beast::http;
namespace ws    = beast::websocket;
namespace net   = boost::asio;
using tcp       = net::ip::tcp;

// ─────────────────────────────────────────────────────────
// CDPConnection
// ─────────────────────────────────────────────────────────

BrowserController::CDPConnection::CDPConnection() = default;
BrowserController::CDPConnection::~CDPConnection() { disconnect(); }

bool BrowserController::CDPConnection::connect(const std::string& websocket_url) {
    try {
        // Parse URL: ws://host:port/path
        std::string host, port, path;
        auto after_scheme = websocket_url.find("://");
        if (after_scheme == std::string::npos) return false;
        auto rest = websocket_url.substr(after_scheme + 3);
        auto colon_pos = rest.find(':');
        auto slash_pos = rest.find('/');
        if (colon_pos != std::string::npos && slash_pos != std::string::npos) {
            host = rest.substr(0, colon_pos);
            port = rest.substr(colon_pos + 1, slash_pos - colon_pos - 1);
            path = rest.substr(slash_pos);
        } else {
            host = rest.substr(0, slash_pos);
            port = "9222";
            path = (slash_pos != std::string::npos) ? rest.substr(slash_pos) : "/";
        }

        tcp::resolver resolver(io_context_);
        auto endpoints = resolver.resolve(host, port);

        auto stream_ptr = std::make_unique<ws::stream<beast::tcp_stream>>(io_context_);
        stream_ptr->next_layer().connect(*endpoints.begin());
        stream_ptr->handshake(host, path);
        ws_ = std::move(stream_ptr);

        connected_ = true;
        running_   = true;

        // Start message handler thread
        message_handler_thread_ = std::thread([this]() { message_handler_loop(); });
        return true;
    } catch (const std::exception& e) {
        spdlog::error("CDPConnection::connect failed: {}", e.what());
        return false;
    }
}

void BrowserController::CDPConnection::disconnect() {
    running_   = false;
    connected_ = false;
    try {
        if (ws_ && ws_->is_open()) ws_->close(ws::close_code::normal);
    } catch (...) {}
    if (message_handler_thread_.joinable()) message_handler_thread_.join();
}

bool BrowserController::CDPConnection::is_connected() const { return connected_; }

JavaScriptResult BrowserController::CDPConnection::send_command(
    const std::string& method, const json& params) {
    return send_command_internal(method, params, false);
}

JavaScriptResult BrowserController::CDPConnection::send_command_async(
    const std::string& method, const json& params) {
    return send_command_internal(method, params, true);
}

JavaScriptResult BrowserController::CDPConnection::send_command_internal(
    const std::string& method, const json& params, bool /*async*/) {
    if (!connected_ || !ws_) {
        JavaScriptResult r; r.error = "Not connected"; return r;
    }
    int id = next_id_++;
    json cmd;
    cmd["id"]     = id;
    cmd["method"] = method;
    cmd["params"] = params.empty() ? json::object() : params;

    // Use a shared promise/future for the response
    auto promise = std::make_shared<std::promise<json>>();
    auto future  = promise->get_future();

    {
        std::lock_guard<std::mutex> lock(handlers_mutex_);
        pending_responses_[id] = promise;
    }

    try {
        ws_->write(net::buffer(cmd.dump()));
    } catch (const std::exception& e) {
        JavaScriptResult r; r.error = e.what(); return r;
    }

    // Wait up to 30 s
    if (future.wait_for(std::chrono::seconds(30)) != std::future_status::ready) {
        {
            std::lock_guard<std::mutex> lock(handlers_mutex_);
            pending_responses_.erase(id);
        }
        JavaScriptResult r; r.error = "Timeout"; return r;
    }

    JavaScriptResult result;
    auto response = future.get();
    if (response.contains("error")) {
        result.error  = response["error"]["message"].get<std::string>();
    } else {
        result.success     = true;
        result.json_result = response.value("result", json::object());
        result.result      = result.json_result.dump();
    }
    return result;
}

void BrowserController::CDPConnection::register_event_handler(
    const std::string& event, std::function<void(const json&)> handler) {
    std::lock_guard<std::mutex> lock(handlers_mutex_);
    event_handlers_[event] = std::move(handler);
}

void BrowserController::CDPConnection::unregister_event_handler(const std::string& event) {
    std::lock_guard<std::mutex> lock(handlers_mutex_);
    event_handlers_.erase(event);
}

void BrowserController::CDPConnection::message_handler_loop() {
    while (running_ && ws_ && ws_->is_open()) {
        try {
            beast::flat_buffer buf;
            ws_->read(buf);
            auto msg = beast::buffers_to_string(buf.data());
            handle_message(msg);
        } catch (...) {
            if (running_) spdlog::debug("CDPConnection message loop ended");
            break;
        }
    }
}

void BrowserController::CDPConnection::handle_message(const std::string& message) {
    try {
        auto j = json::parse(message);
        std::lock_guard<std::mutex> lock(handlers_mutex_);
        if (j.contains("id")) {
            int id = j["id"].get<int>();
            auto it = pending_responses_.find(id);
            if (it != pending_responses_.end()) {
                it->second->set_value(j);
                pending_responses_.erase(it);
            }
        } else if (j.contains("method")) {
            auto method = j["method"].get<std::string>();
            auto eit = event_handlers_.find(method);
            if (eit != event_handlers_.end()) eit->second(j.value("params", json::object()));
        }
    } catch (...) {}
}

// ─────────────────────────────────────────────────────────
// ChromeProcess
// ─────────────────────────────────────────────────────────

BrowserController::ChromeProcess::ChromeProcess() = default;
BrowserController::ChromeProcess::~ChromeProcess() { stop(); }

bool BrowserController::ChromeProcess::start(const BrowserConfig& config) {
    debug_port_ = "9222";
    std::string cmd = build_chrome_command(config);

#ifdef _WIN32
    STARTUPINFOA si{};
    PROCESS_INFORMATION pi{};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    if (!CreateProcessA(nullptr, const_cast<char*>(cmd.c_str()),
                        nullptr, nullptr, FALSE, CREATE_NO_WINDOW,
                        nullptr, nullptr, &si, &pi)) {
        spdlog::error("Failed to start Chrome process");
        return false;
    }
    process_id_ = static_cast<int>(pi.dwProcessId);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
#else
    process_id_ = fork();
    if (process_id_ == 0) {
        // child
        execl("/bin/sh", "sh", "-c", cmd.c_str(), nullptr);
        _exit(1);
    } else if (process_id_ < 0) {
        spdlog::error("Failed to fork Chrome process");
        return false;
    }
#endif
    running_ = true;
    return wait_for_debug_port(std::chrono::seconds(30));
}

void BrowserController::ChromeProcess::stop() {
    if (!running_) return;
    running_ = false;
#ifdef _WIN32
    if (process_id_ > 0) {
        HANDLE h = OpenProcess(PROCESS_TERMINATE, FALSE, process_id_);
        if (h) { TerminateProcess(h, 0); CloseHandle(h); }
    }
#else
    if (process_id_ > 0) ::kill(process_id_, SIGTERM);
#endif
    process_id_ = -1;
}

bool BrowserController::ChromeProcess::is_running() const { return running_; }
int  BrowserController::ChromeProcess::get_pid()          const { return process_id_; }
std::string BrowserController::ChromeProcess::get_debug_port() const { return debug_port_; }

std::string BrowserController::ChromeProcess::build_chrome_command(const BrowserConfig& config) {
    std::string exe = config.chrome_path.empty() ? "chrome" : config.chrome_path;
    std::ostringstream oss;
    oss << "\"" << exe << "\"";
    oss << " --remote-debugging-port=" << debug_port_;
    oss << " --user-data-dir=\"" << config.user_data_dir << "\"";
    if (config.headless) oss << " --headless=new";
    if (config.disable_gpu) oss << " --disable-gpu";
    if (config.ignore_certificate_errors) oss << " --ignore-certificate-errors";
    oss << " --window-size=" << config.window_size;
    if (!config.user_agent.empty()) oss << " --user-agent=\"" << config.user_agent << "\"";
    if (!config.proxy_server.empty()) oss << " --proxy-server=" << config.proxy_server;
    oss << " --no-sandbox --disable-setuid-sandbox --disable-blink-features=AutomationControlled";
    for (auto& [k, v] : config.extra_flags) oss << " --" << k << "=" << v;
    return oss.str();
}

bool BrowserController::ChromeProcess::wait_for_debug_port(std::chrono::seconds timeout) {
    auto start    = std::chrono::steady_clock::now();
    auto deadline = start + timeout;
    while (std::chrono::steady_clock::now() < deadline) {
        // Try to connect to the debug port
        try {
            net::io_context ioc;
            tcp::resolver   resolver(ioc);
            auto endpoints = resolver.resolve("127.0.0.1", debug_port_);
            beast::tcp_stream stream(ioc);
            stream.connect(*endpoints.begin());
            stream.socket().close();
            return true;
        } catch (...) {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
    }
    spdlog::error("Timed out waiting for Chrome debug port {}", debug_port_);
    return false;
}

// ─────────────────────────────────────────────────────────
// BrowserController
// ─────────────────────────────────────────────────────────

BrowserController::BrowserController()
    : cdp_connection_(std::make_unique<CDPConnection>()),
      chrome_process_(std::make_unique<ChromeProcess>()) {}

BrowserController::~BrowserController() { stop(); }

bool BrowserController::start(const BrowserConfig& config) {
    std::lock_guard lock(controller_mutex_);
    config_ = config;
    status_ = BrowserStatus::STARTING;
    if (!chrome_process_->start(config)) { status_ = BrowserStatus::CRASHED; return false; }

    // Discover the first available page WS URL via /json/list
    if (!initialize_cdp_connection()) { status_ = BrowserStatus::CRASHED; return false; }

    setup_event_handlers();
    inject_stealth_scripts();
    status_ = BrowserStatus::RUNNING;
    return true;
}

void BrowserController::stop() {
    {
        std::lock_guard lock(controller_mutex_);
        status_ = BrowserStatus::STOPPED;
    }
    cdp_connection_->disconnect();
    chrome_process_->stop();
}

bool BrowserController::restart() { stop(); return start(config_); }
BrowserStatus BrowserController::get_status() const { return status_; }

bool BrowserController::initialize_cdp_connection() {
    // Query /json/list to get the first tab's webSocketDebuggerUrl
    try {
        net::io_context ioc;
        tcp::resolver   resolver(ioc);
        auto endpoints = resolver.resolve("127.0.0.1", chrome_process_->get_debug_port());
        beast::tcp_stream stream(ioc);
        stream.connect(*endpoints.begin());
        http::request<http::string_body> req(http::verb::get, "/json/list", 11);
        req.set(http::field::host, "127.0.0.1");
        req.set(http::field::user_agent, "gmail-infinity/1.0");
        http::write(stream, req);
        beast::flat_buffer buf;
        http::response<http::string_body> res;
        http::read(stream, buf, res);
        auto pages = json::parse(res.body());
        if (pages.empty()) { set_error("No browser pages found"); return false; }
        std::string ws_url = pages[0]["webSocketDebuggerUrl"].get<std::string>();
        return cdp_connection_->connect(ws_url);
    } catch (const std::exception& e) {
        set_error(e.what()); return false;
    }
}

void BrowserController::setup_event_handlers() {
    cdp_connection_->send_command("Page.enable");
    cdp_connection_->send_command("Network.enable");
    cdp_connection_->send_command("Console.enable");
    cdp_connection_->register_event_handler("Page.loadEventFired",
        [this](const json&) { navigation_state_ = NavigationState::LOADED; });
    cdp_connection_->register_event_handler("Page.frameNavigated",
        [this](const json&) { navigation_state_ = NavigationState::LOADING; });
}

bool BrowserController::navigate_to(const std::string& url, std::chrono::milliseconds timeout) {
    navigation_state_ = NavigationState::LOADING;
    navigation_start_time_ = std::chrono::steady_clock::now();
    json params; params["url"] = url;
    auto r = cdp_connection_->send_command("Page.navigate", params);
    if (!r.success) { set_error(r.error); return false; }
    // Wait for load
    auto deadline = std::chrono::steady_clock::now() + timeout;
    while (navigation_state_ != NavigationState::LOADED
           && std::chrono::steady_clock::now() < deadline) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    auto elapsed = std::chrono::steady_clock::now() - navigation_start_time_;
    last_page_load_time_ = static_cast<double>(
        std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count());
    return navigation_state_ == NavigationState::LOADED;
}

std::string BrowserController::get_current_url() const {
    auto r = cdp_connection_->send_command("Runtime.evaluate",
        json{{"expression", "window.location.href"}, {"returnByValue", true}});
    if (r.success && r.json_result.contains("result"))
        return r.json_result["result"].value("value", "");
    return "";
}

std::string BrowserController::get_page_title() const {
    auto r = cdp_connection_->send_command("Runtime.evaluate",
        json{{"expression", "document.title"}, {"returnByValue", true}});
    if (r.success && r.json_result.contains("result"))
        return r.json_result["result"].value("value", "");
    return "";
}

JavaScriptResult BrowserController::execute_javascript(const std::string& script) {
    json params;
    params["expression"]    = script;
    params["returnByValue"] = true;
    params["awaitPromise"]  = false;
    return cdp_connection_->send_command("Runtime.evaluate", params);
}

JavaScriptResult BrowserController::execute_javascript_async(const std::string& script) {
    json params;
    params["expression"]    = script;
    params["returnByValue"] = true;
    params["awaitPromise"]  = true;
    return cdp_connection_->send_command("Runtime.evaluate", params);
}

bool BrowserController::add_init_script(const std::string& script) {
    json params; params["source"] = script;
    auto r = cdp_connection_->send_command("Page.addScriptToEvaluateOnNewDocument", params);
    if (r.success) {
        init_scripts_.push_back(script);
        return true;
    }
    return false;
}

bool BrowserController::click_element(const std::string& selector, int timeout_ms) {
    auto script = click_element_script(selector);
    execute_javascript(wait_for_element_script(selector, timeout_ms));
    auto r = execute_javascript(script);
    return r.success;
}

bool BrowserController::type_text(const std::string& selector, const std::string& text, bool clear_first) {
    execute_javascript(wait_for_element_script(selector, 10000));
    auto script = type_text_script(selector, text, clear_first);
    auto r = execute_javascript(script);
    if (!r.success) return false;
    // Use Input.insertText for better simulation
    json params; params["text"] = text;
    cdp_connection_->send_command("Input.insertText", params);
    return true;
}

bool BrowserController::element_exists(const std::string& selector) {
    auto r = execute_javascript(
        "!!document.querySelector('" + selector + "')");
    return r.success && r.result.find("true") != std::string::npos;
}

bool BrowserController::element_is_visible(const std::string& selector) {
    auto script = R"(
        (function() {
            var el = document.querySelector(')" + selector + R"(');
            if (!el) return false;
            var style = window.getComputedStyle(el);
            return style.display !== 'none' && style.visibility !== 'hidden' && el.offsetWidth > 0;
        })()
    )";
    auto r = execute_javascript(script);
    return r.success && r.result.find("true") != std::string::npos;
}

std::string BrowserController::get_element_text(const std::string& selector) {
    auto r = execute_javascript(
        "document.querySelector('" + selector + "')?.innerText || ''");
    return r.success ? r.result : "";
}

std::string BrowserController::get_element_attribute(
    const std::string& selector, const std::string& attribute) {
    auto r = execute_javascript(
        "document.querySelector('" + selector + "')?.getAttribute('" + attribute + "') || ''");
    return r.success ? r.result : "";
}

bool BrowserController::take_screenshot(const std::string& filename) {
    auto r = cdp_connection_->send_command("Page.captureScreenshot",
        json{{"format", "png"}, {"quality", 90}});
    if (!r.success) return false;
    // base64 decode and write to file
    auto data = r.json_result.value("data", "");
    if (data.empty()) return false;
    // Simple base64 decode
    std::ofstream ofs(filename, std::ios::binary);
    if (!ofs) return false;
    // Use OpenSSL for base64
    static const std::string b64chars =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    auto decode = [&](const std::string& encoded) -> std::string {
        std::string out;
        int val = 0, bits = -8;
        for (unsigned char c : encoded) {
            if (c == '=') break;
            auto idx = b64chars.find(c);
            if (idx == std::string::npos) continue;
            val = (val << 6) + static_cast<int>(idx);
            bits += 6;
            if (bits >= 0) {
                out += static_cast<char>((val >> bits) & 0xFF);
                bits -= 8;
            }
        }
        return out;
    };
    ofs << decode(data);
    return true;
}

std::string BrowserController::get_page_source() {
    auto r = execute_javascript("document.documentElement.outerHTML");
    return r.success ? r.result : "";
}

bool BrowserController::set_cookie(
    const std::string& name, const std::string& value, const std::string& domain) {
    json params; params["name"] = name; params["value"] = value;
    if (!domain.empty()) params["domain"] = domain;
    auto r = cdp_connection_->send_command("Network.setCookie", params);
    return r.success;
}

bool BrowserController::clear_cookies() {
    auto r = cdp_connection_->send_command("Network.clearBrowserCookies");
    return r.success;
}

bool BrowserController::inject_stealth_scripts() {
    load_stealth_scripts();
    for (auto& [name, script] : stealth_scripts_) {
        add_init_script(script);
        execute_javascript(script); // also inject into current page
    }
    return true;
}

bool BrowserController::set_proxy(const std::string& proxy_server) {
    config_.proxy_server = proxy_server;
    return true; // proxy set at Chrome launch level
}

bool BrowserController::set_offline_mode(bool offline) {
    json params; params["offline"] = offline; params["latency"] = 0;
    params["downloadThroughput"] = offline ? 0 : -1;
    params["uploadThroughput"]   = offline ? 0 : -1;
    auto r = cdp_connection_->send_command("Network.emulateNetworkConditions", params);
    return r.success;
}

bool BrowserController::block_request(const std::string& url_pattern) {
    json params; params["urls"] = json::array({url_pattern});
    auto r = cdp_connection_->send_command("Network.setBlockedURLs", params);
    return r.success;
}

double BrowserController::get_page_load_time() {
    return last_page_load_time_.load();
}

std::string BrowserController::get_last_error() const {
    std::lock_guard lock(controller_mutex_);
    return last_error_;
}

void BrowserController::set_error(const std::string& error) {
    std::lock_guard lock(controller_mutex_);
    last_error_ = error;
    spdlog::error("BrowserController: {}", error);
}

void BrowserController::clear_error() {
    std::lock_guard lock(controller_mutex_);
    last_error_.clear();
}

bool BrowserController::has_errors() const {
    std::lock_guard lock(controller_mutex_);
    return !last_error_.empty();
}

void BrowserController::load_stealth_scripts() {
    stealth_scripts_["hide_webdriver"] = R"JS(
        Object.defineProperty(navigator,'webdriver',{get:()=>undefined,configurable:true});
    )JS";
    stealth_scripts_["chrome_runtime"] = R"JS(
        if(!window.chrome){window.chrome={runtime:{},loadTimes:function(){},csi:function(){}}}
    )JS";
    stealth_scripts_["canvas_noise"] = R"JS(
        (function(){
            const td=HTMLCanvasElement.prototype.toDataURL;
            HTMLCanvasElement.prototype.toDataURL=function(t){
                const ctx=this.getContext('2d');
                if(ctx){const d=ctx.getImageData(0,0,this.width,this.height);
                for(let i=0;i<d.data.length;i+=4){d.data[i]^=(Math.random()*3|0)-1;}
                ctx.putImageData(d,0,0);}return td.apply(this,arguments);};
        })();
    )JS";
    stealth_scripts_["webgl_vendor"] = R"JS(
        (function(){
            const g=WebGLRenderingContext.prototype.getParameter;
            WebGLRenderingContext.prototype.getParameter=function(p){
                if(p===37445)return 'Intel Inc.';if(p===37446)return 'Intel Iris OpenGL Engine';
                return g.call(this,p);};
        })();
    )JS";
}

// Helper script builders
std::string BrowserController::wait_for_element_script(
    const std::string& selector, int timeout_ms) const {
    return "(function(){return new Promise((res,rej)=>{const st=Date.now();"
           "const iv=setInterval(()=>{if(document.querySelector('" + selector + "')){clearInterval(iv);res(true);}"
           "else if(Date.now()-st>" + std::to_string(timeout_ms) + "){clearInterval(iv);rej('timeout');}},100);});})()";
}

std::string BrowserController::click_element_script(const std::string& selector) const {
    return "document.querySelector('" + selector + "')?.click()";
}

std::string BrowserController::type_text_script(
    const std::string& selector, const std::string& text, bool clear_first) const {
    std::string s = "var el=document.querySelector('" + selector + "');if(el){el.focus();";
    if (clear_first) s += "el.value='';";
    s += "el.value='" + text + "';"
         "el.dispatchEvent(new Event('input',{bubbles:true}));"
         "el.dispatchEvent(new Event('change',{bubbles:true}));}";
    return s;
}

std::string BrowserController::generate_random_user_agent() const {
    static const std::vector<std::string> uas = {
        "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 Chrome/122.0.0.0 Safari/537.36",
        "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 Chrome/121.0.0.0 Safari/537.36",
        "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/537.36 Chrome/122.0.0.0 Safari/537.36",
    };
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, static_cast<int>(uas.size()) - 1);
    return uas[dis(gen)];
}

// ─────────────────────────────────────────────────────────
// StealthScriptInjector
// ─────────────────────────────────────────────────────────

StealthScriptInjector::StealthScriptInjector(std::shared_ptr<BrowserController> controller)
    : controller_(std::move(controller)) { load_stealth_scripts(); }

bool StealthScriptInjector::inject_all_stealth_scripts() {
    return inject_webdriver_script() && inject_canvas_fingerprint_script()
        && inject_webgl_fingerprint_script() && inject_audio_fingerprint_script()
        && inject_navigator_script() && inject_permissions_script();
}

bool StealthScriptInjector::inject_webdriver_script() {
    return controller_->add_init_script(get_webdriver_script());
}

bool StealthScriptInjector::inject_canvas_fingerprint_script() {
    return controller_->add_init_script(get_canvas_script());
}

bool StealthScriptInjector::inject_webgl_fingerprint_script() {
    return controller_->add_init_script(get_webgl_script());
}

bool StealthScriptInjector::inject_audio_fingerprint_script() {
    return controller_->add_init_script(get_audio_script());
}

bool StealthScriptInjector::inject_navigator_script() {
    return controller_->add_init_script(get_navigator_script());
}

bool StealthScriptInjector::inject_permissions_script() {
    return controller_->add_init_script(get_permissions_script());
}

bool StealthScriptInjector::inject_screen_script() {
    return controller_->add_init_script(get_screen_script());
}

bool StealthScriptInjector::inject_custom_script(
    const std::string& /*name*/, const std::string& content) {
    return controller_->add_init_script(content);
}

void StealthScriptInjector::load_stealth_scripts() {
    stealth_scripts_["webdriver"] = get_webdriver_script();
    stealth_scripts_["canvas"]    = get_canvas_script();
    stealth_scripts_["webgl"]     = get_webgl_script();
    stealth_scripts_["audio"]     = get_audio_script();
    stealth_scripts_["navigator"] = get_navigator_script();
}

std::string StealthScriptInjector::get_webdriver_script() const {
    return "Object.defineProperty(navigator,'webdriver',{get:()=>undefined,configurable:true});";
}

std::string StealthScriptInjector::get_canvas_script() const {
    return R"JS((function(){const td=HTMLCanvasElement.prototype.toDataURL;
        HTMLCanvasElement.prototype.toDataURL=function(t){
        const ctx=this.getContext('2d');if(ctx){const d=ctx.getImageData(0,0,this.width,this.height);
        for(let i=0;i<d.data.length;i+=4){d.data[i]^=(Math.random()*3|0)-1;}ctx.putImageData(d,0,0);}
        return td.apply(this,arguments);};})();)JS";
}

std::string StealthScriptInjector::get_webgl_script() const {
    return R"JS((function(){const g=WebGLRenderingContext.prototype.getParameter;
        WebGLRenderingContext.prototype.getParameter=function(p){
        if(p===37445)return 'Intel Inc.';if(p===37446)return 'Intel Iris OpenGL Engine';
        return g.call(this,p);};})();)JS";
}

std::string StealthScriptInjector::get_audio_script() const {
    return R"JS((function(){const o=AudioBuffer.prototype.getChannelData;
        AudioBuffer.prototype.getChannelData=function(){const d=o.apply(this,arguments);
        for(let i=0;i<d.length;i+=100){d[i]+=(Math.random()-0.5)*0.0001;}return d;};})();)JS";
}

std::string StealthScriptInjector::get_navigator_script() const {
    return R"JS(Object.defineProperty(navigator,'languages',{get:()=>['en-US','en'],configurable:true});
        Object.defineProperty(navigator,'language',{get:()=>'en-US',configurable:true});
        Object.defineProperty(navigator,'hardwareConcurrency',{get:()=>8,configurable:true});
        Object.defineProperty(navigator,'deviceMemory',{get:()=>8,configurable:true});)JS";
}

std::string StealthScriptInjector::get_screen_script() const {
    return R"JS(Object.defineProperty(screen,'width',{get:()=>1920});
        Object.defineProperty(screen,'height',{get:()=>1080});
        Object.defineProperty(screen,'colorDepth',{get:()=>24});)JS";
}

std::string StealthScriptInjector::get_timezone_script() const { return ""; }
std::string StealthScriptInjector::get_language_script() const { return get_navigator_script(); }
std::string StealthScriptInjector::get_permissions_script() const { return ""; }

// ─────────────────────────────────────────────────────────
// BrowserFactory
// ─────────────────────────────────────────────────────────

std::shared_ptr<BrowserController> BrowserFactory::create_browser(const BrowserConfig& config) {
    auto bc = std::make_shared<BrowserController>();
    if (!bc->start(config)) return nullptr;
    return bc;
}

std::shared_ptr<BrowserController> BrowserFactory::create_stealth_browser(const BrowserConfig& config) {
    auto cfg = config;
    apply_stealth_settings(cfg);
    auto bc = std::make_shared<BrowserController>();
    if (!bc->start(cfg)) return nullptr;
    StealthScriptInjector injector(bc);
    injector.inject_all_stealth_scripts();
    return bc;
}

std::shared_ptr<BrowserController> BrowserFactory::create_mobile_browser(const BrowserConfig& config) {
    auto cfg = config;
    apply_mobile_settings(cfg);
    auto bc = std::make_shared<BrowserController>();
    if (!bc->start(cfg)) return nullptr;
    return bc;
}

std::shared_ptr<BrowserController> BrowserFactory::create_tablet_browser(const BrowserConfig& config) {
    auto cfg = config;
    cfg.window_size = "1024,768";
    auto bc = std::make_shared<BrowserController>();
    if (!bc->start(cfg)) return nullptr;
    return bc;
}

BrowserConfig BrowserFactory::create_base_config() {
    BrowserConfig cfg;
    cfg.headless = true;
    cfg.disable_gpu = true;
    cfg.ignore_certificate_errors = true;
    cfg.window_size = "1920,1080";
    return cfg;
}

void BrowserFactory::apply_desktop_settings(BrowserConfig& cfg) {
    cfg.window_size = "1920,1080";
}

void BrowserFactory::apply_mobile_settings(BrowserConfig& cfg) {
    cfg.window_size = "375,812";
    cfg.user_agent = "Mozilla/5.0 (iPhone; CPU iPhone OS 17_0 like Mac OS X) "
                     "AppleWebKit/605.1.15 (KHTML, like Gecko) Version/17.0 Mobile/15E148 Safari/604.1";
}

void BrowserFactory::apply_stealth_settings(BrowserConfig& cfg) {
    cfg.extra_flags["disable-blink-features"] = "AutomationControlled";
    cfg.extra_flags["excludeSwitches"]         = "enable-automation";
}

BrowserConfig BrowserFactory::build_desktop_config() {
    auto cfg = create_base_config(); apply_desktop_settings(cfg); return cfg;
}
BrowserConfig BrowserFactory::build_mobile_config() {
    auto cfg = create_base_config(); apply_mobile_settings(cfg); return cfg;
}
BrowserConfig BrowserFactory::build_stealth_config() {
    auto cfg = create_base_config(); apply_stealth_settings(cfg); return cfg;
}
BrowserConfig BrowserFactory::build_config_with_proxy(const std::string& proxy) {
    auto cfg = build_stealth_config(); cfg.proxy_server = proxy; return cfg;
}

// Stubs for remaining interface methods
bool BrowserController::wait_for_navigation(std::chrono::milliseconds timeout) {
    auto deadline = std::chrono::steady_clock::now() + timeout;
    while (navigation_state_ != NavigationState::LOADED
           && std::chrono::steady_clock::now() < deadline) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    return navigation_state_ == NavigationState::LOADED;
}

bool BrowserController::select_option(const std::string& selector, const std::string& value) {
    auto script = "var el=document.querySelector('" + selector + "');"
                  "if(el){el.value='" + value + "';"
                  "el.dispatchEvent(new Event('change',{bubbles:true}));}";
    return execute_javascript(script).success;
}

bool BrowserController::scroll_to_element(const std::string& selector) {
    return execute_javascript("document.querySelector('" + selector + "')?.scrollIntoView()").success;
}

bool BrowserController::hover_element(const std::string& selector) {
    auto script = "var el=document.querySelector('" + selector + "');"
                  "if(el){el.dispatchEvent(new MouseEvent('mouseover',{bubbles:true}));}";
    return execute_javascript(script).success;
}

bool BrowserController::element_is_enabled(const std::string& selector) {
    auto r = execute_javascript("!document.querySelector('" + selector + "')?.disabled");
    return r.success && r.result.find("true") != std::string::npos;
}

std::vector<std::string> BrowserController::get_all_elements_text(const std::string& selector) {
    auto r = execute_javascript(
        "Array.from(document.querySelectorAll('" + selector + "')).map(e=>e.innerText)");
    if (!r.success) return {};
    try {
        auto arr = json::parse(r.result);
        std::vector<std::string> out;
        for (auto& el : arr) out.push_back(el.get<std::string>());
        return out;
    } catch (...) { return {}; }
}

bool BrowserController::remove_init_script(const std::string& /*script_id*/) { return true; }
std::string BrowserController::get_console_logs(int /*limit*/) { return "[]"; }
std::vector<std::string> BrowserController::get_network_requests(int /*limit*/) { return {}; }
std::string BrowserController::get_cookie(const std::string& /*name*/) { return ""; }
bool BrowserController::clear_browser_data() {
    cdp_connection_->send_command("Storage.clearDataForOrigin",
        json{{"origin", "*"}, {"storageTypes", "all"}});
    return true;
}
bool BrowserController::export_storage_state(const std::string& /*filename*/) { return true; }
bool BrowserController::set_random_user_agent() {
    config_.user_agent = generate_random_user_agent();
    return true;
}
bool BrowserController::randomize_viewport() { return true; }
bool BrowserController::inject_canvas_noise()  { return true; }
bool BrowserController::inject_webgl_noise()   { return true; }
bool BrowserController::inject_audio_noise()   { return true; }
bool BrowserController::hide_automation_indicators() { return inject_stealth_scripts(); }
bool BrowserController::rotate_proxy() { return true; }
std::string BrowserController::get_current_proxy() { return config_.proxy_server; }
bool BrowserController::set_network_conditions(
    const std::string&, const std::string&, int) { return true; }
bool BrowserController::unblock_request(const std::string&) { return true; }
std::map<std::string, double> BrowserController::get_performance_metrics() { return {}; }
std::string BrowserController::get_memory_usage() { return "0"; }
void BrowserController::clear_error_internal() { last_error_.clear(); }
void BrowserController::log_browser_info(const std::string& msg) const { spdlog::info("[browser] {}", msg); }
void BrowserController::log_browser_error(const std::string& msg) const { spdlog::error("[browser] {}", msg); }
void BrowserController::log_browser_debug(const std::string& msg) const { spdlog::debug("[browser] {}", msg); }
void BrowserController::handle_page_load_events()  {}
void BrowserController::handle_console_events()    {}
void BrowserController::handle_network_events()    {}
std::string BrowserController::get_stealth_script(const std::string& n) const {
    auto it = stealth_scripts_.find(n);
    return it != stealth_scripts_.end() ? it->second : "";
}
JavaScriptResult BrowserController::navigate_to_internal(const std::string& url) {
    json p; p["url"] = url;
    return cdp_connection_->send_command("Page.navigate", p);
}
JavaScriptResult BrowserController::click_element_internal(const std::string& sel) {
    return execute_javascript(click_element_script(sel));
}
JavaScriptResult BrowserController::type_text_internal(
    const std::string& sel, const std::string& text, bool clear) {
    return execute_javascript(type_text_script(sel, text, clear));
}
JavaScriptResult BrowserController::execute_script_internal(
    const std::string& script, bool /*async*/) {
    return execute_javascript(script);
}

} // namespace gmail_infinity
