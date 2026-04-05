#pragma once

#include <string>
#include <vector>
#include <memory>
#include <map>
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <functional>
#include <random>

#include <nlohmann/json.hpp>

#include "../core/browser_controller.h"
#include "../core/proxy_manager.h"
#include "../identity/persona_generator.h"
#include "../creators/gmail_creator.h"

namespace gmail_infinity {

enum class ActivityType {
    GOOGLE_SEARCH,
    YOUTUBE_WATCH,
    GMAIL_CHECK,
    GOOGLE_DRIVE_ACCESS,
    GOOGLE_CALENDAR_USE,
    GOOGLE_MEET_JOIN,
    GOOGLE_CHAT_MESSAGE,
    GOOGLE_MAPS_SEARCH,
    GOOGLE_PLAY_VISIT,
    GOOGLE_NEWS_READ,
    GOOGLE_IMAGES_SEARCH,
    GOOGLE_TRANSLATE_USE,
    GOOGLE_DOCS_EDIT,
    GOOGLE_SHEETS_EDIT,
    GOOGLE_SLIDES_EDIT,
    GOOGLE_FORMS_USE,
    GOOGLE_SITES_VISIT,
    GOOGLE_GROUPS_POST,
    GOOGLE_KEEP_NOTE,
    GOOGLE_PHOTOS_UPLOAD,
    CUSTOM_ACTIVITY
};

enum class ActivityIntensity {
    LOW,
    MEDIUM,
    HIGH,
    REALISTIC,
    STEALTH
};

enum class WarmingPhase {
    INITIAL_SETUP,
    GRADUAL_WARMING,
    ACTIVE_USAGE,
    MAINTENANCE,
    COOLDOWN
};

struct ActivityConfig {
    ActivityType type;
    std::string description;
    std::chrono::duration<double> average_duration;
    std::chrono::duration<double> min_duration;
    std::chrono::duration<double> max_duration;
    double probability; // 0.0 to 1.0
    ActivityIntensity intensity;
    std::vector<std::string> required_permissions;
    std::map<std::string, std::string> parameters;
    
    nlohmann::json to_json() const;
    void from_json(const nlohmann::json& j);
};

struct ActivitySession {
    std::string id;
    std::string account_email;
    ActivityType activity_type;
    std::chrono::system_clock::time_point started_at;
    std::chrono::system_clock::time_point ended_at;
    std::chrono::milliseconds duration;
    bool successful;
    std::string error_message;
    std::map<std::string, std::string> session_data;
    std::vector<std::string> actions_performed;
    
    nlohmann::json to_json() const;
    void from_json(const nlohmann::json& j);
};

struct WarmingSchedule {
    std::string account_email;
    WarmingPhase current_phase;
    std::chrono::system_clock::time_point phase_started_at;
    std::chrono::hours phase_duration;
    std::vector<ActivityConfig> scheduled_activities;
    std::map<ActivityType, size_t> activity_counts;
    std::chrono::milliseconds total_activity_time;
    std::chrono::milliseconds last_activity_time;
    
    nlohmann::json to_json() const;
    void from_json(const nlohmann::json& j);
};

struct WarmingConfig {
    // General settings
    bool enabled = true;
    ActivityIntensity default_intensity = ActivityIntensity::REALISTIC;
    bool simulate_human_behavior = true;
    bool randomize_timing = true;
    std::chrono::hours warming_duration{24 * 7}; // 1 week
    
    // Browser settings
    bool headless = true;
    bool enable_stealth = true;
    std::string user_agent;
    std::chrono::seconds page_load_timeout{30};
    std::chrono::seconds action_timeout{10};
    
    // Activity settings
    std::vector<ActivityType> enabled_activities;
    std::map<ActivityType, double> activity_probabilities;
    std::chrono::hours min_activity_interval{1};
    std::chrono::hours max_activity_interval{8};
    std::chrono::minutes min_session_duration{5};
    std::chrono::minutes max_session_duration{60};
    
    // Proxy settings
    bool use_proxy = true;
    bool rotate_proxy_per_session = true;
    std::string preferred_proxy_country = "US";
    
    // Scheduling settings
    std::chrono::hours daily_active_hours{16}; // 16 active hours per day
    std::chrono::hours sleep_start_time{22}; // 10 PM
    std::chrono::hours sleep_end_time{6}; // 6 AM
    bool respect_timezones = true;
    
    // Progression settings
    bool gradual_intensity_increase = true;
    double initial_activity_rate = 0.1; // 10% of max activity
    double final_activity_rate = 0.8; // 80% of max activity
    std::chrono::hours intensity_ramp_up_duration{24 * 3}; // 3 days to ramp up
    
    nlohmann::json to_json() const;
    void from_json(const nlohmann::json& j);
};

class ActivitySimulator {
public:
    ActivitySimulator();
    ~ActivitySimulator();
    
    // Initialization
    bool initialize(const WarmingConfig& config);
    void shutdown();
    
    // Account warming
    std::future<bool> warm_account_async(const CreationResult& account_result);
    bool warm_account(const CreationResult& account_result);
    std::future<bool> warm_account_with_schedule_async(const std::string& email, const WarmingSchedule& schedule);
    bool warm_account_with_schedule(const std::string& email, const WarmingSchedule& schedule);
    
    // Batch warming
    std::vector<std::future<bool>> warm_multiple_accounts_async(const std::vector<CreationResult>& accounts);
    std::vector<bool> warm_multiple_accounts(const std::vector<CreationResult>& accounts);
    
    // Individual activities
    std::future<ActivitySession> perform_google_search_async(const std::string& email, const std::string& query);
    ActivitySession perform_google_search(const std::string& email, const std::string& query);
    std::future<ActivitySession> watch_youtube_video_async(const std::string& email, const std::string& video_url);
    ActivitySession watch_youtube_video(const std::string& email, const std::string& video_url);
    std::future<ActivitySession> check_gmail_async(const std::string& email);
    ActivitySession check_gmail(const std::string& email);
    std::future<ActivitySession> access_google_drive_async(const std::string& email);
    ActivitySession access_google_drive(const std::string& email);
    std::future<ActivitySession> use_google_calendar_async(const std::string& email);
    ActivitySession use_google_calendar(const std::string& email);
    std::future<ActivitySession> join_google_meet_async(const std::string& email, const std::string& meet_url);
    ActivitySession join_google_meet(const std::string& email, const std::string& meet_url);
    std::future<ActivitySession> send_google_chat_async(const std::string& email, const std::string& message);
    ActivitySession send_google_chat(const std::string& email, const std::string& message);
    std::future<ActivitySession> search_google_maps_async(const std::string& email, const std::string& location);
    ActivitySession search_google_maps(const std::string& email, const std::string& location);
    std::future<ActivitySession> read_google_news_async(const std::string& email);
    ActivitySession read_google_news(const std::string& email);
    std::future<ActivitySession> search_google_images_async(const std::string& email, const std::string& query);
    ActivitySession search_google_images(const std::string& email, const std::string& query);
    std::future<ActivitySession> use_google_translate_async(const std::string& email, const std::string& text, const std::string& target_lang);
    ActivitySession use_google_translate(const std::string& email, const std::string& text, const std::string& target_lang);
    std::future<ActivitySession> edit_google_docs_async(const std::string& email, const std::string& doc_id);
    ActivitySession edit_google_docs(const std::string& email, const std::string& doc_id);
    std::future<ActivitySession> edit_google_sheets_async(const std::string& email, const std::string& sheet_id);
    ActivitySession edit_google_sheets(const std::string& email, const std::string& sheet_id);
    std::future<ActivitySession> edit_google_slides_async(const std::string& email, const std::string& slide_id);
    ActivitySession edit_google_slides(const std::string& email, const std::string& slide_id);
    std::future<ActivitySession> use_google_forms_async(const std::string& email, const std::string& form_url);
    ActivitySession use_google_forms(const std::string& email, const std::string& form_url);
    std::future<ActivitySession> visit_google_sites_async(const std::string& email, const std::string& site_url);
    ActivitySession visit_google_sites(const std::string& email, const std::string& site_url);
    std::future<ActivitySession> post_google_groups_async(const std::string& email, const std::string& group_id, const std::string& message);
    ActivitySession post_google_groups(const std::string& email, const std::string& group_id, const std::string& message);
    std::future<ActivitySession> create_google_keep_note_async(const std::string& email, const std::string& note_content);
    ActivitySession create_google_keep_note(const std::string& email, const std::string& note_content);
    std::future<ActivitySession> upload_google_photos_async(const std::string& email, const std::string& image_path);
    ActivitySession upload_google_photos(const std::string& email, const std::string& image_path);
    
    // Scheduling and automation
    std::future<bool> start_automated_warming_async(const std::string& email);
    bool start_automated_warming(const std::string& email);
    std::future<bool> stop_automated_warming_async(const std::string& email);
    bool stop_automated_warming(const std::string& email);
    
    // Progress monitoring
    WarmingSchedule get_warming_schedule(const std::string& email) const;
    std::vector<ActivitySession> get_activity_history(const std::string& email, size_t limit = 100);
    std::map<ActivityType, size_t> get_activity_statistics(const std::string& email) const;
    double get_warming_progress(const std::string& email) const;
    
    // Configuration
    void update_config(const WarmingConfig& config);
    WarmingConfig get_config() const;
    
    // Statistics
    std::map<std::string, WarmingSchedule> get_all_warming_schedules() const;
    std::map<ActivityType, double> get_global_activity_statistics() const;
    std::map<std::string, double> get_account_warming_scores() const;
    std::chrono::milliseconds get_average_activity_duration(ActivityType type) const;
    
    // History and logging
    std::vector<ActivitySession> get_all_activity_history(size_t limit = 1000);
    bool export_activity_history(const std::string& filename) const;
    bool export_warming_schedules(const std::string& filename) const;

private:
    // Core components
    std::shared_ptr<BrowserController> browser_;
    std::shared_ptr<ProxyManager> proxy_manager_;
    std::shared_ptr<FingerprintCache> fingerprint_cache_;
    
    // Configuration
    WarmingConfig config_;
    
    // Activity definitions
    std::map<ActivityType, ActivityConfig> activity_configs_;
    
    // Warming schedules
    std::map<std::string, WarmingSchedule> warming_schedules_;
    std::map<std::string, std::thread> warming_threads_;
    std::map<std::string, std::atomic<bool>> warming_thread_flags_;
    
    // Activity history
    std::vector<ActivitySession> activity_history_;
    mutable std::mutex history_mutex_;
    
    // Statistics
    std::map<ActivityType, std::vector<std::chrono::milliseconds>> activity_durations_;
    std::map<ActivityType, std::atomic<size_t>> activity_counts_;
    std::atomic<size_t> total_activities_{0};
    
    // Thread safety
    mutable std::mutex simulator_mutex_;
    std::atomic<bool> shutdown_requested_{false};
    
    // Random generation
    mutable std::mt19937 rng_;
    std::uniform_real_distribution<double> real_dist_{0.0, 1.0};
    
    // Private methods
    bool load_activity_configs();
    WarmingSchedule create_warming_schedule(const std::string& email);
    void update_warming_schedule(const std::string& email, const ActivitySession& session);
    
    // Activity implementations
    ActivitySession execute_google_search(const std::string& email, const std::string& query);
    ActivitySession execute_youtube_watch(const std::string& email, const std::string& video_url);
    ActivitySession execute_gmail_check(const std::string& email);
    ActivitySession execute_google_drive_access(const std::string& email);
    ActivitySession execute_google_calendar_use(const std::string& email);
    ActivitySession execute_google_meet_join(const std::string& email, const std::string& meet_url);
    ActivitySession execute_google_chat_message(const std::string& email, const std::string& message);
    ActivitySession execute_google_maps_search(const std::string& email, const std::string& location);
    ActivitySession execute_google_news_read(const std::string& email);
    ActivitySession execute_google_images_search(const std::string& email, const std::string& query);
    ActivitySession execute_google_translate_use(const std::string& email, const std::string& text, const std::string& target_lang);
    ActivitySession execute_google_docs_edit(const std::string& email, const std::string& doc_id);
    ActivitySession execute_google_sheets_edit(const std::string& email, const std::string& sheet_id);
    ActivitySession execute_google_slides_edit(const std::string& email, const std::string& slide_id);
    ActivitySession execute_google_forms_use(const std::string& email, const std::string& form_url);
    ActivitySession execute_google_sites_visit(const std::string& email, const std::string& site_url);
    ActivitySession execute_google_groups_post(const std::string& email, const std::string& group_id, const std::string& message);
    ActivitySession execute_google_keep_note(const std::string& email, const std::string& note_content);
    ActivitySession execute_google_photos_upload(const std::string& email, const std::string& image_path);
    
    // Automated warming
    void automated_warming_loop(const std::string& email);
    ActivityType select_next_activity(const std::string& email);
    std::chrono::milliseconds calculate_activity_delay(const std::string& email);
    bool should_perform_activity(const std::string& email);
    bool is_active_hours(const std::string& email);
    
    // Helper methods
    bool setup_browser_for_account(const std::string& email);
    bool login_to_google_account(const std::string& email, const std::string& password);
    void simulate_human_behavior();
    void simulate_typing_delay(const std::string& text);
    void simulate_mouse_movement();
    void simulate_reading_time(int word_count);
    void simulate_scroll_behavior();
    
    // Content generation
    std::string generate_search_query(ActivityType type);
    std::string generate_youtube_query();
    std::string generate_chat_message();
    std::string generate_keep_note_content();
    std::string generate_calendar_event_title();
    std::string generate_docs_content();
    std::string generate_groups_post_content();
    
    // Progress tracking
    void record_activity_session(const ActivitySession& session);
    void update_activity_statistics(ActivityType type, std::chrono::milliseconds duration);
    void calculate_warming_progress(const std::string& email);
    
    // Utility methods
    std::string activity_type_to_string(ActivityType type) const;
    ActivityType string_to_activity_type(const std::string& type_str) const;
    std::string generate_session_id() const;
    std::chrono::milliseconds get_random_duration(const ActivityConfig& config) const;
    
    // Browser interaction helpers
    bool wait_for_element_and_click(const std::string& selector, int timeout_ms = 10000);
    bool wait_for_element_and_type(const std::string& selector, const std::string& text, bool clear_first = true, int timeout_ms = 10000);
    bool wait_for_element_and_select(const std::string& selector, const std::string& value, int timeout_ms = 10000);
    std::string get_element_text(const std::string& selector, int timeout_ms = 10000);
    bool wait_for_page_load(int timeout_ms = 30000);
    bool navigate_to_url(const std::string& url);
    
    // Google service navigation
    bool navigate_to_google_search();
    bool navigate_to_youtube();
    bool navigate_to_gmail();
    bool navigate_to_google_drive();
    bool navigate_to_google_calendar();
    bool navigate_to_google_meet();
    bool navigate_to_google_chat();
    bool navigate_to_google_maps();
    bool navigate_to_google_news();
    bool navigate_to_google_images();
    bool navigate_to_google_translate();
    bool navigate_to_google_docs();
    bool navigate_to_google_sheets();
    bool navigate_to_google_slides();
    bool navigate_to_google_forms();
    bool navigate_to_google_sites();
    bool navigate_to_google_groups();
    bool navigate_to_google_keep();
    bool navigate_to_google_photos();
    
    // Logging
    void log_simulator_info(const std::string& message) const;
    void log_simulator_error(const std::string& message) const;
    void log_simulator_debug(const std::string& message) const;
};

// Human behavior simulator
class HumanBehaviorSimulator {
public:
    HumanBehaviorSimulator();
    ~HumanBehaviorSimulator() = default;
    
    // Behavior simulation
    void simulate_typing_pattern(const std::string& text, ActivityIntensity intensity = ActivityIntensity::REALISTIC);
    void simulate_mouse_movement_pattern(ActivityIntensity intensity = ActivityIntensity::REALISTIC);
    void simulate_scroll_pattern(ActivityIntensity intensity = ActivityIntensity::REALISTIC);
    void simulate_reading_pattern(int word_count, ActivityIntensity intensity = ActivityIntensity::REALISTIC);
    void simulate_click_pattern(ActivityIntensity intensity = ActivityIntensity::REALISTIC);
    
    // Timing simulation
    std::chrono::milliseconds get_typing_delay(const std::string& text, ActivityIntensity intensity);
    std::chrono::milliseconds get_mouse_delay(ActivityIntensity intensity);
    std::chrono::milliseconds get_scroll_delay(ActivityIntensity intensity);
    std::chrono::milliseconds get_reading_delay(int word_count, ActivityIntensity intensity);
    std::chrono::milliseconds get_click_delay(ActivityIntensity intensity);
    
    // Randomization
    void set_randomization_level(double level); // 0.0 = no randomization, 1.0 = maximum randomization
    void set_typing_variance(double variance);
    void set_mouse_variance(double variance);
    void set_timing_variance(double variance);

private:
    double randomization_level_{0.3};
    double typing_variance_{0.2};
    double mouse_variance_{0.3};
    double timing_variance_{0.25};
    
    mutable std::mt19937 rng_;
    std::normal_distribution<double> normal_dist_{0.0, 1.0};
    std::uniform_real_distribution<double> uniform_dist_{0.0, 1.0};
    
    std::chrono::milliseconds apply_variance(std::chrono::milliseconds base_time, double variance) const;
    double calculate_typing_speed(const std::string& text, ActivityIntensity intensity) const;
    std::vector<std::pair<int, int>> generate_mouse_path(const std::string& selector) const;
    std::vector<int> generate_scroll_pattern(ActivityIntensity intensity) const;
};

// Activity analyzer
class ActivityAnalyzer {
public:
    ActivityAnalyzer();
    ~ActivityAnalyzer() = default;
    
    // Analysis methods
    std::map<ActivityType, double> analyze_activity_patterns(const std::vector<ActivitySession>& sessions);
    std::vector<std::string> detect_anomalous_activities(const std::vector<ActivitySession>& sessions);
    double calculate_human_behavior_score(const std::vector<ActivitySession>& sessions);
    std::map<std::string, double> calculate_account_warming_scores(const std::map<std::string, std::vector<ActivitySession>>& account_sessions);
    
    // Pattern detection
    std::vector<std::string> detect_bot_like_patterns(const std::vector<ActivitySession>& sessions);
    std::vector<std::string> detect_suspicious_timing_patterns(const std::vector<ActivitySession>& sessions);
    std::vector<std::string> detect_unrealistic_behavior(const std::vector<ActivitySession>& sessions);
    
    // Recommendations
    std::vector<std::string> generate_behavior_improvements(const std::vector<ActivitySession>& sessions);
    std::map<std::string, std::string> suggest_activity_adjustments(const std::map<std::string, std::vector<ActivitySession>>& account_sessions);

private:
    std::map<ActivityType, std::vector<std::string>> activity_patterns_;
    std::map<ActivityType, std::chrono::milliseconds> realistic_durations_;
    
    void initialize_patterns();
    bool is_realistic_timing(ActivityType type, std::chrono::milliseconds duration);
    bool has_natural_activity_sequence(const std::vector<ActivitySession>& sessions);
};

} // namespace gmail_infinity
