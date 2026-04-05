#include "activity_simulator.h"
#include "app/app_logger.h"

#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>
#include <thread>
#include <chrono>
#include <random>
#include <future>
#include <algorithm>

namespace gmail_infinity {

using json = nlohmann::json;

ActivitySimulator::ActivitySimulator() {
    std::random_device rd;
    rng_.seed(rd());
}

ActivitySimulator::~ActivitySimulator() {
    shutdown();
}

bool ActivitySimulator::initialize(const WarmingConfig& config) {
    std::lock_guard<std::mutex> lock(simulator_mutex_);
    config_ = config;
    load_activity_configs();
    return true;
}

void ActivitySimulator::shutdown() {
    shutdown_requested_ = true;
    std::lock_guard<std::mutex> lock(simulator_mutex_);
    for (auto& [email, thread] : warming_threads_) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    warming_threads_.clear();
}

std::future<bool> ActivitySimulator::warm_account_async(const CreationResult& account_result) {
    return std::async(std::launch::async, [this, account_result]() {
        return warm_account(account_result);
    });
}

bool ActivitySimulator::warm_account(const CreationResult& account_result) {
    spdlog::info("ActivitySimulator: warming account {}", account_result.email);
    
    WarmingSchedule schedule = create_warming_schedule(account_result.email);
    return warm_account_with_schedule(account_result.email, schedule);
}

bool ActivitySimulator::warm_account_with_schedule(const std::string& email, const WarmingSchedule& schedule) {
    std::lock_guard<std::mutex> lock(simulator_mutex_);
    warming_schedules_[email] = schedule;
    
    // Start automated warming thread
    warming_thread_flags_[email] = true;
    warming_threads_[email] = std::thread(&ActivitySimulator::automated_warming_loop, this, email);
    
    return true;
}

void ActivitySimulator::automated_warming_loop(const std::string& email) {
    spdlog::info("ActivitySimulator: starting automated warming loop for {}", email);
    
    while (!shutdown_requested_ && warming_thread_flags_[email]) {
        if (should_perform_activity(email) && is_active_hours(email)) {
            ActivityType next_activity = select_next_activity(email);
            
            ActivitySession session;
            switch (next_activity) {
                case ActivityType::GOOGLE_SEARCH:
                    session = execute_google_search(email, generate_search_query(next_activity));
                    break;
                case ActivityType::YOUTUBE_WATCH:
                    session = execute_youtube_watch(email, "https://www.youtube.com/watch?v=dQw4w9WgXcQ");
                    break;
                case ActivityType::GMAIL_CHECK:
                    session = execute_gmail_check(email);
                    break;
                default:
                    // Just navigate to the service for others in this simplified implementation
                    session.activity_type = next_activity;
                    session.successful = true;
                    break;
            }
            
            record_activity_session(session);
            update_warming_schedule(email, session);
        }
        
        // Wait for next activity
        auto delay = calculate_activity_delay(email);
        std::this_thread::sleep_for(delay);
    }
    
    spdlog::info("ActivitySimulator: stopped automated warming loop for {}", email);
}

ActivitySession ActivitySimulator::execute_google_search(const std::string& email, const std::string& query) {
    ActivitySession session;
    session.id = generate_session_id();
    session.account_email = email;
    session.activity_type = ActivityType::GOOGLE_SEARCH;
    session.started_at = std::chrono::system_clock::now();
    
    spdlog::info("ActivitySimulator: performing Google Search for {} with query: {}", email, query);
    
    try {
        if (!setup_browser_for_account(email)) throw std::runtime_error("Browser setup failed");
        
        navigate_to_url("https://www.google.com");
        wait_for_element_and_type("textarea[name='q']", query);
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        browser_->press_key("Enter");
        
        wait_for_page_load();
        simulate_human_behavior();
        
        session.successful = true;
    } catch (const std::exception& e) {
        session.successful = false;
        session.error_message = e.what();
    }
    
    session.ended_at = std::chrono::system_clock::now();
    session.duration = std::chrono::duration_cast<std::chrono::milliseconds>(session.ended_at - session.started_at);
    return session;
}

ActivitySession ActivitySimulator::execute_gmail_check(const std::string& email) {
    ActivitySession session;
    session.id = generate_session_id();
    session.account_email = email;
    session.activity_type = ActivityType::GMAIL_CHECK;
    session.started_at = std::chrono::system_clock::now();
    
    spdlog::info("ActivitySimulator: checking Gmail for {}", email);
    
    try {
        if (!setup_browser_for_account(email)) throw std::runtime_error("Browser setup failed");
        
        navigate_to_url("https://mail.google.com");
        wait_for_page_load();
        simulate_human_behavior();
        
        session.successful = true;
    } catch (const std::exception& e) {
        session.successful = false;
        session.error_message = e.what();
    }
    
    session.ended_at = std::chrono::system_clock::now();
    session.duration = std::chrono::duration_cast<std::chrono::milliseconds>(session.ended_at - session.started_at);
    return session;
}

// Helper implementations
bool ActivitySimulator::setup_browser_for_account(const std::string& email) {
    // In real app, we'd load the specific persona and proxy for this email
    if (!browser_) {
        BrowserConfig b_config;
        b_config.headless = config_.headless;
        browser_ = BrowserFactory::create_stealth_browser(b_config);
    }
    return browser_ != nullptr;
}

bool ActivitySimulator::navigate_to_url(const std::string& url) {
    return browser_->navigate_to(url);
}

bool ActivitySimulator::wait_for_element_and_type(const std::string& selector, const std::string& text, bool clear_first, int timeout_ms) {
    return browser_->type_text(selector, text, clear_first);
}

bool ActivitySimulator::wait_for_page_load(int timeout_ms) {
    std::this_thread::sleep_for(std::chrono::milliseconds(2000)); // Generic wait
    return true;
}

void ActivitySimulator::simulate_human_behavior() {
    // Randomized scrolling and mouse movements
    std::uniform_int_distribution<int> scroll_dis(100, 500);
    browser_->execute_javascript("window.scrollBy(0, " + std::to_string(scroll_dis(rng_)) + ")");
    std::this_thread::sleep_for(std::chrono::milliseconds(1000 + (rng_() % 2000)));
}

bool ActivitySimulator::should_perform_activity(const std::string& email) {
    return real_dist_(rng_) < 0.8; // 80% chance if checked
}

bool ActivitySimulator::is_active_hours(const std::string& email) {
    // Simple check against current system time for now
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    struct tm* tm_ptr = std::localtime(&time);
    int hour = tm_ptr->tm_hour;
    
    return hour >= config_.sleep_end_time.count() && hour < config_.sleep_start_time.count();
}

ActivityType ActivitySimulator::select_next_activity(const std::string& email) {
    if (config_.enabled_activities.empty()) return ActivityType::GOOGLE_SEARCH;
    
    std::uniform_int_distribution<size_t> dis(0, config_.enabled_activities.size() - 1);
    return config_.enabled_activities[dis(rng_)];
}

std::chrono::milliseconds ActivitySimulator::calculate_activity_delay(const std::string& email) {
    std::uniform_int_distribution<long long> dis(
        config_.min_activity_interval.count() * 3600000,
        config_.max_activity_interval.count() * 3600000
    );
    return std::chrono::milliseconds(dis(rng_) / 100); // Scale down for testing/rewrite
}

void ActivitySimulator::record_activity_session(const ActivitySession& session) {
    std::lock_guard<std::mutex> lock(history_mutex_);
    activity_history_.push_back(session);
    total_activities_++;
}

bool ActivitySimulator::load_activity_configs() {
    // Initialize default probabilities etc.
    activity_configs_[ActivityType::GOOGLE_SEARCH] = {ActivityType::GOOGLE_SEARCH, "Google Search", std::chrono::seconds(30)};
    activity_configs_[ActivityType::GMAIL_CHECK] = {ActivityType::GMAIL_CHECK, "Gmail Check", std::chrono::seconds(60)};
    activity_configs_[ActivityType::YOUTUBE_WATCH] = {ActivityType::YOUTUBE_WATCH, "YouTube Watch", std::chrono::seconds(180)};
    return true;
}

WarmingSchedule ActivitySimulator::create_warming_schedule(const std::string& email) {
    WarmingSchedule schedule;
    schedule.account_email = email;
    schedule.current_phase = WarmingPhase::INITIAL_SETUP;
    schedule.phase_started_at = std::chrono::system_clock::now();
    return schedule;
}

void ActivitySimulator::update_warming_schedule(const std::string& email, const ActivitySession& session) {
    std::lock_guard<std::mutex> lock(simulator_mutex_);
    auto& schedule = warming_schedules_[email];
    schedule.activity_counts[session.activity_type]++;
    schedule.total_activity_time += session.duration;
    schedule.last_activity_time = session.duration;
}

std::string ActivitySimulator::generate_session_id() const {
    return "act_" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
}

std::string ActivitySimulator::generate_search_query(ActivityType type) {
    static const std::vector<std::string> queries = {
        "best restaurants near me", "how to learn c++", "weather today", 
        "latest news", "google workspace features", "youtube trending"
    };
    std::uniform_int_distribution<size_t> dis(0, queries.size() - 1);
    return queries[dis(rng_)];
}

// JSON implementations
nlohmann::json ActivityConfig::to_json() const { return json::object(); }
nlohmann::json ActivitySession::to_json() const { return json::object(); }
nlohmann::json WarmingSchedule::to_json() const { return json::object(); }
nlohmann::json WarmingConfig::to_json() const { return json::object(); }

} // namespace gmail_infinity
