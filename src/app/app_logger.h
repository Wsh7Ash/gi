#pragma once

#include <memory>
#include <string>
#include <spdlog/spdlog.h>

namespace gmail_infinity {

// Application logger setup
std::shared_ptr<spdlog::logger> setup_logging(const std::string& level = "info");

// Get the application logger instance
std::shared_ptr<spdlog::logger> get_logger();

// Mask sensitive information in log messages
std::string mask_sensitive(const std::string& input);

// Log macros for convenience
#define LOG_TRACE(...) ::gmail_infinity::get_logger()->trace(__VA_ARGS__)
#define LOG_DEBUG(...) ::gmail_infinity::get_logger()->debug(__VA_ARGS__)
#define LOG_INFO(...) ::gmail_infinity::get_logger()->info(__VA_ARGS__)
#define LOG_WARN(...) ::gmail_infinity::get_logger()->warn(__VA_ARGS__)
#define LOG_ERROR(...) ::gmail_infinity::get_logger()->error(__VA_ARGS__)
#define LOG_CRITICAL(...) ::gmail_infinity::get_logger()->critical(__VA_ARGS__)

} // namespace gmail_infinity
