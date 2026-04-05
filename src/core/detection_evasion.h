#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <chrono>

namespace gmail_infinity {

// All stealth JavaScript payloads as constexpr string_view
struct StealthScripts {
    static constexpr const char* HIDE_WEBDRIVER = R"JS(
        Object.defineProperty(navigator, 'webdriver', {
            get: () => undefined,
            configurable: true
        });
        delete navigator.__proto__.webdriver;
    )JS";

    static constexpr const char* CHROME_RUNTIME = R"JS(
        if (!window.chrome) {
            window.chrome = { runtime: {}, loadTimes: function(){}, csi: function(){} };
        }
    )JS";

    static constexpr const char* PERMISSIONS_API = R"JS(
        const originalQuery = window.navigator.permissions.query;
        window.navigator.permissions.query = (parameters) => (
            parameters.name === 'notifications'
                ? Promise.resolve({ state: Notification.permission })
                : originalQuery(parameters)
        );
    )JS";

    static constexpr const char* PLUGINS_ARRAY = R"JS(
        Object.defineProperty(navigator, 'plugins', {
            get: () => {
                const plugins = [
                    { name: 'Chrome PDF Plugin', filename: 'internal-pdf-viewer',
                      description: 'Portable Document Format' },
                    { name: 'Chrome PDF Viewer', filename: 'mhjfbmdgcfjbbpaeojofohoefgiehjai',
                      description: '' },
                    { name: 'Native Client', filename: 'internal-nacl-plugin',
                      description: '' }
                ];
                plugins.__proto__ = PluginArray.prototype;
                return plugins;
            }
        });
    )JS";

    static constexpr const char* LANGUAGE_OVERRIDE = R"JS(
        Object.defineProperty(navigator, 'languages', {
            get: () => ['en-US', 'en'],
            configurable: true
        });
        Object.defineProperty(navigator, 'language', {
            get: () => 'en-US',
            configurable: true
        });
    )JS";

    static constexpr const char* WEBGL_VENDOR = R"JS(
        (function() {
            const getParam = WebGLRenderingContext.prototype.getParameter;
            WebGLRenderingContext.prototype.getParameter = function(param) {
                if (param === 37445) return 'Intel Inc.';
                if (param === 37446) return 'Intel Iris OpenGL Engine';
                return getParam.call(this, param);
            };
            const getParam2 = WebGL2RenderingContext.prototype.getParameter;
            WebGL2RenderingContext.prototype.getParameter = function(param) {
                if (param === 37445) return 'Intel Inc.';
                if (param === 37446) return 'Intel Iris OpenGL Engine';
                return getParam2.call(this, param);
            };
        })();
    )JS";

    static constexpr const char* CANVAS_NOISE = R"JS(
        (function() {
            const toDataURL = HTMLCanvasElement.prototype.toDataURL;
            HTMLCanvasElement.prototype.toDataURL = function(type) {
                const ctx = this.getContext('2d');
                if (ctx) {
                    const imageData = ctx.getImageData(0,0,this.width,this.height);
                    for (let i = 0; i < imageData.data.length; i += 4) {
                        imageData.data[i]   ^= Math.floor(Math.random()*3)-1;
                        imageData.data[i+1] ^= Math.floor(Math.random()*3)-1;
                        imageData.data[i+2] ^= Math.floor(Math.random()*3)-1;
                    }
                    ctx.putImageData(imageData,0,0);
                }
                return toDataURL.apply(this, arguments);
            };
        })();
    )JS";

    static constexpr const char* WEBRTC_BLOCK = R"JS(
        (function() {
            const origRTCPeerConnection = window.RTCPeerConnection;
            window.RTCPeerConnection = function(config) {
                if (config && config.iceServers) {
                    config.iceServers = [];
                }
                return new origRTCPeerConnection(config);
            };
            window.RTCPeerConnection.prototype = origRTCPeerConnection.prototype;
        })();
    )JS";

    static constexpr const char* AUDIO_NOISE = R"JS(
        (function() {
            const origGetChannelData = AudioBuffer.prototype.getChannelData;
            AudioBuffer.prototype.getChannelData = function() {
                const data = origGetChannelData.apply(this, arguments);
                for (let i = 0; i < data.length; i += 100) {
                    data[i] += (Math.random() - 0.5) * 0.0001;
                }
                return data;
            };
        })();
    )JS";

    static constexpr const char* HARDWARE_CONCURRENCY = R"JS(
        Object.defineProperty(navigator, 'hardwareConcurrency', {
            get: () => 8,
            configurable: true
        });
    )JS";

    static constexpr const char* DEVICE_MEMORY = R"JS(
        Object.defineProperty(navigator, 'deviceMemory', {
            get: () => 8,
            configurable: true
        });
    )JS";
};

class DetectionEvasion {
public:
    DetectionEvasion();
    ~DetectionEvasion() = default;

    // Get combined stealth script for injection
    std::string get_full_stealth_script() const;

    // Individual scripts
    std::string get_webdriver_script() const;
    std::string get_chrome_runtime_script() const;
    std::string get_permissions_script() const;
    std::string get_plugins_script() const;
    std::string get_language_script() const;
    std::string get_webgl_script() const;
    std::string get_canvas_script() const;
    std::string get_webrtc_script() const;
    std::string get_audio_script() const;
    std::string get_hardware_script() const;

    // Dynamic overrides
    std::string get_timezone_script(const std::string& timezone) const;
    std::string get_screen_script(int width, int height, int depth = 24) const;
    std::string get_ua_script(const std::string& user_agent) const;

    // HTTP header manipulation
    std::map<std::string, std::string> get_stealth_headers(
        const std::string& user_agent,
        const std::string& origin_url = "") const;

    // Risk assessment
    double calculate_detection_risk(
        const std::map<std::string, std::string>& browser_properties) const;

    // Timing attack mitigation
    std::string get_timing_jitter_script() const;
    void apply_request_timing_jitter();

private:
    std::string combine_scripts(const std::vector<std::string>& scripts) const;
    std::string wrap_in_iife(const std::string& script) const;

    mutable std::mt19937 rng_;
};

} // namespace gmail_infinity
