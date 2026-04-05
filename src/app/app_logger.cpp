#include "app_logger.h"
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/async.h>
#include <regex>
#include <filesystem>

namespace gmail_infinity {

std::shared_ptr<spdlog::logger> setup_logging(const std::string& level) {
    try {
        // Create logs directory if it doesn't exist
        std::filesystem::create_directories("logs");
        
        // Create async logging thread pool
        spdlog::init_thread_pool(8192, 1);
        
        // Create console sink
        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        console_sink->set_level(spdlog::level::debug);
        console_sink->set_pattern("[%H:%M:%S.%e] [%^%l%$] [thread %t] %v");
        
        // Create rotating file sink
        auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
            "logs/gmail_infinity.log", 
            1024 * 1024 * 100, // 100MB
            10 // 10 files
        );
        file_sink->set_level(spdlog::level::trace);
        file_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] [thread %t] [%@] %v");
        
        // Create async logger with both sinks
        std::vector<spdlog::sink_ptr> sinks = {console_sink, file_sink};
        auto logger = std::make_shared<spdlog::async_logger>(
            "gmail_infinity", 
            sinks.begin(), 
            sinks.end(),
            spdlog::thread_pool(),
            spdlog::async_overflow_policy::block
        );
        
        // Set log level
        spdlog::level::level_enum log_level;
        if (level == "trace") {
            log_level = spdlog::level::trace;
        } else if (level == "debug") {
            log_level = spdlog::level::debug;
        } else if (level == "info") {
            log_level = spdlog::level::info;
        } else if (level == "warn") {
            log_level = spdlog::level::warn;
        } else if (level == "error") {
            log_level = spdlog::level::error;
        } else {
            log_level = spdlog::level::info;
        }
        
        logger->set_level(log_level);
        logger->flush_on(spdlog::level::warn);
        
        // Register logger
        spdlog::register_logger(logger);
        spdlog::set_default_logger(logger);
        
        return logger;
        
    } catch (const std::exception& e) {
        std::cerr << "Failed to setup logging: " << e.what() << std::endl;
        return nullptr;
    }
}

std::shared_ptr<spdlog::logger> get_logger() {
    return spdlog::get("gmail_infinity");
}

std::string mask_sensitive(const std::string& input) {
    try {
        std::string masked = input;
        
        // Mask email addresses
        std::regex email_regex(R"((\w{1,2})[\w.-]*@(\w{1,2})[\w.]*)");
        masked = std::regex_replace(masked, email_regex, "$1***@$2***");
        
        // Mask phone numbers
        std::regex phone_regex(R"((\d{3})\d{3,4}(\d{4}))");
        masked = std::regex_replace(masked, phone_regex, "$1****$2");
        
        // Mask API keys and tokens
        std::regex api_key_regex(R"(([A-Za-z0-9_-]{4})[A-Za-z0-9_-]{20,}([A-Za-z0-9_-]{4}))");
        masked = std::regex_replace(masked, api_key_regex, "$1***$2");
        
        // Mask passwords
        std::regex password_regex(R"(password[\"']?\s*[:=]\s*[\"']?([^\"'\s,}]{6,}))");
        masked = std::regex_replace(masked, password_regex, "password: \"******\"");
        
        // Mask IP addresses (partial)
        std::regex ip_regex(R"((\d{1,3}\.\d{1,3}\.)\d{1,3}\.\d{1,3})");
        masked = std::regex_replace(masked, ip_regex, "$1***.***");
        
        return masked;
        
    } catch (const std::exception& e) {
        // If regex fails, return original string
        return input;
    }
}

} // namespace gmail_infinity
