#include "detection_evasion.h"

#include <spdlog/spdlog.h>
#include <random>
#include <sstream>
#include <thread>
#include <chrono>

namespace gmail_infinity {

DetectionEvasion::DetectionEvasion() {
    std::random_device rd;
    rng_.seed(rd());
}

std::string DetectionEvasion::get_full_stealth_script() const {
    return combine_scripts({
        get_webdriver_script(),
        get_chrome_runtime_script(),
        get_permissions_script(),
        get_plugins_script(),
        get_language_script(),
        get_webgl_script(),
        get_canvas_script(),
        get_webrtc_script(),
        get_audio_script(),
        get_hardware_script(),
        get_timing_jitter_script()
    });
}

std::string DetectionEvasion::get_webdriver_script() const {
    return StealthScripts::HIDE_WEBDRIVER;
}

std::string DetectionEvasion::get_chrome_runtime_script() const {
    return StealthScripts::CHROME_RUNTIME;
}

std::string DetectionEvasion::get_permissions_script() const {
    return StealthScripts::PERMISSIONS_API;
}

std::string DetectionEvasion::get_plugins_script() const {
    return StealthScripts::PLUGINS_ARRAY;
}

std::string DetectionEvasion::get_language_script() const {
    return StealthScripts::LANGUAGE_OVERRIDE;
}

std::string DetectionEvasion::get_webgl_script() const {
    return StealthScripts::WEBGL_VENDOR;
}

std::string DetectionEvasion::get_canvas_script() const {
    return StealthScripts::CANVAS_NOISE;
}

std::string DetectionEvasion::get_webrtc_script() const {
    return StealthScripts::WEBRTC_BLOCK;
}

std::string DetectionEvasion::get_audio_script() const {
    return StealthScripts::AUDIO_NOISE;
}

std::string DetectionEvasion::get_hardware_script() const {
    return std::string(StealthScripts::HARDWARE_CONCURRENCY)
         + std::string(StealthScripts::DEVICE_MEMORY);
}

std::string DetectionEvasion::get_timezone_script(const std::string& timezone) const {
    return R"JS((function(){
        const tz = ')JS" + timezone + R"JS(';
        const origDTF = Intl.DateTimeFormat;
        Intl.DateTimeFormat = function(locale, options) {
            if (!options) options = {};
            options.timeZone = tz;
            return new origDTF(locale, options);
        };
        Intl.DateTimeFormat.prototype = origDTF.prototype;
    })();)JS";
}

std::string DetectionEvasion::get_screen_script(int width, int height, int depth) const {
    return "(function(){"
           "Object.defineProperty(screen,'width',{get:()=>" + std::to_string(width) + "});"
           "Object.defineProperty(screen,'height',{get:()=>" + std::to_string(height) + "});"
           "Object.defineProperty(screen,'colorDepth',{get:()=>" + std::to_string(depth) + "});"
           "})();";
}

std::string DetectionEvasion::get_ua_script(const std::string& user_agent) const {
    return "Object.defineProperty(navigator,'userAgent',{get:()=>'" + user_agent + "',configurable:true});";
}

std::map<std::string, std::string> DetectionEvasion::get_stealth_headers(
    const std::string& user_agent, const std::string& /*origin_url*/) const {
    return {
        {"User-Agent",      user_agent},
        {"Accept-Language", "en-US,en;q=0.9"},
        {"Accept-Encoding", "gzip, deflate, br"},
        {"Accept",          "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8"},
        {"DNT",             "1"},
        {"Upgrade-Insecure-Requests", "1"},
        {"Sec-Fetch-Site",  "none"},
        {"Sec-Fetch-Mode",  "navigate"},
        {"Sec-Fetch-User",  "?1"},
        {"Sec-Fetch-Dest",  "document"},
    };
}

double DetectionEvasion::calculate_detection_risk(
    const std::map<std::string, std::string>& props) const {
    double risk = 0.0;
    // Simple heuristic: flag known automation indicators
    auto check = [&](const std::string& key, const std::string& bad_value) {
        auto it = props.find(key);
        if (it != props.end() && it->second.find(bad_value) != std::string::npos) risk += 0.2;
    };
    check("webdriver", "true");
    check("user_agent", "HeadlessChrome");
    check("plugins",    "0");
    return std::min(1.0, risk);
}

std::string DetectionEvasion::get_timing_jitter_script() const {
    return R"JS((function(){
        const orig = Date.now;
        Date.now = function() {
            return orig() + (Math.random() - 0.5) * 2;
        };
        const origPerf = performance.now.bind(performance);
        performance.now = function() {
            return origPerf() + (Math.random() - 0.5) * 0.5;
        };
    })();)JS";
}

void DetectionEvasion::apply_request_timing_jitter() {
    std::uniform_int_distribution<int> delay_ms(50, 300);
    std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms(rng_)));
}

std::string DetectionEvasion::combine_scripts(const std::vector<std::string>& scripts) const {
    std::ostringstream oss;
    oss << "(function(){\n'use strict';\n";
    for (auto& s : scripts) oss << s << "\n";
    oss << "})();\n";
    return oss.str();
}

std::string DetectionEvasion::wrap_in_iife(const std::string& script) const {
    return "(function(){\n" + script + "\n})();\n";
}

} // namespace gmail_infinity
