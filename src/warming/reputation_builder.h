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

#include "activity_simulator.h"
#include "google_services.h"
#include "../identity/persona_generator.h"
#include "../creators/gmail_creator.h"

namespace gmail_infinity {

enum class ReputationMetric {
    ACCOUNT_AGE,
    ACTIVITY_FREQUENCY,
    SERVICE_DIVERSITY,
    CONTENT_QUALITY,
    SOCIAL_SIGNALS,
    TRUST_SCORE,
    AUTHORITY_SCORE,
    ENGAGEMENT_RATE,
    CONSISTENCY_SCORE,
    OVERALL_REPUTATION
};

enum class ReputationLevel {
    VERY_LOW,
    LOW,
    MEDIUM,
    HIGH,
    VERY_HIGH,
    EXCELLENT
};

enum class ReputationStrategy {
    GRADUAL_BUILDING,
    ACCELERATED_BUILDING,
    MAINTENANCE,
    RECOVERY,
    STEALTH_BUILDING,
    AGGRESSIVE_BUILDING
};

struct ReputationConfig {
    // General settings
    ReputationStrategy strategy = ReputationStrategy::GRADUAL_BUILDING;
    ReputationLevel target_level = ReputationLevel::HIGH;
    std::chrono::hours building_duration{24 * 30}; // 30 days
    bool enable_monitoring = true;
    bool enable_adaptive_adjustments = true;
    
    // Activity settings
    std::chrono::hours min_daily_activity{2};
    std::chrono::hours max_daily_activity{8};
    std::chrono::days min_activity_days{5};
    std::chrono::days max_activity_days{7};
    double activity_diversity_threshold{0.7}; // 70% diversity required
    
    // Service preferences
    std::map<GoogleService, double> service_weights;
    std::vector<GoogleService> core_services;
    std::vector<GoogleService> optional_services;
    
    // Content settings
    bool create_original_content = true;
    bool engage_with_others = true;
    double content_quality_threshold{0.8};
    std::chrono::days content_creation_interval{3};
    
    // Social settings
    bool build_social_connections = true;
    size_t min_connections = 5;
    size_t max_connections = 50;
    double interaction_quality_threshold{0.7};
    
    // Trust building
    bool enable_trust_signals = true;
    bool maintain_consistency = true;
    double consistency_threshold{0.8};
    std::chrono::hours activity_time_variance{2}; // 2 hours variance allowed
    
    // Monitoring
    std::chrono::hours monitoring_interval{24};
    std::chrono::hours reputation_check_interval{72};
    bool enable_alerts = true;
    double reputation_drop_threshold{0.1}; // Alert if reputation drops 10%
    
    // Adaptive parameters
    double adaptation_rate = 0.1; // How quickly to adapt to changes
    bool risk_adjustment_enabled = true;
    double risk_tolerance = 0.3; // 30% risk tolerance
    
    nlohmann::json to_json() const;
    void from_json(const nlohmann::json& j);
};

struct ReputationScore {
    ReputationMetric metric;
    double score; // 0.0 to 1.0
    double weight; // Importance weight
    std::chrono::system_clock::time_point last_updated;
    std::map<std::string, std::string> contributing_factors;
    std::vector<std::string> positive_indicators;
    std::vector<std::string> negative_indicators;
    
    nlohmann::json to_json() const;
    void from_json(const nlohmann::json& j);
};

struct ReputationProfile {
    std::string account_email;
    std::chrono::system_clock::time_point profile_created;
    std::chrono::system_clock::time_point last_updated;
    ReputationLevel current_level;
    ReputationLevel target_level;
    std::map<ReputationMetric, ReputationScore> scores;
    double overall_score;
    std::chrono::hours total_activity_time;
    std::chrono::days active_days;
    std::vector<GoogleService> used_services;
    std::vector<std::string> achievements;
    std::vector<std::string> warnings;
    std::map<std::string, std::string> metadata;
    
    nlohmann::json to_json() const;
    void from_json(const nlohmann::json& j);
    
    double calculate_overall_score() const;
    ReputationLevel determine_level(double score) const;
    bool meets_target_level() const;
};

struct ReputationBuildingTask {
    std::string id;
    std::string account_email;
    ReputationMetric target_metric;
    std::string description;
    std::chrono::system_clock::time_point scheduled_at;
    std::chrono::system_clock::time_point started_at;
    std::chrono::system_clock::time_point completed_at;
    std::chrono::milliseconds duration;
    bool completed;
    bool successful;
    double impact_score;
    std::string error_message;
    std::map<std::string, std::string> task_data;
    
    nlohmann::json to_json() const;
    void from_json(const nlohmann::json& j);
};

class ReputationBuilder {
public:
    ReputationBuilder();
    ~ReputationBuilder();
    
    // Initialization
    bool initialize(const ReputationConfig& config);
    void shutdown();
    
    // Reputation building
    std::future<bool> build_reputation_async(const CreationResult& account);
    bool build_reputation(const CreationResult& account);
    std::future<bool> build_reputation_with_strategy_async(const CreationResult& account, ReputationStrategy strategy);
    bool build_reputation_with_strategy(const CreationResult& account, ReputationStrategy strategy);
    
    // Batch reputation building
    std::vector<std::future<bool>> build_multiple_reputations_async(const std::vector<CreationResult>& accounts);
    std::vector<bool> build_multiple_reputations(const std::vector<CreationResult>& accounts);
    
    // Reputation monitoring
    std::future<ReputationProfile> get_reputation_profile_async(const std::string& email);
    ReputationProfile get_reputation_profile(const std::string& email);
    std::future<std::vector<ReputationProfile>> get_all_reputation_profiles_async();
    std::vector<ReputationProfile> get_all_reputation_profiles();
    
    // Reputation analysis
    std::future<std::map<ReputationMetric, double>> analyze_reputation_metrics_async(const std::string& email);
    std::map<ReputationMetric, double> analyze_reputation_metrics(const std::string& email);
    std::future<double> calculate_overall_reputation_score_async(const std::string& email);
    double calculate_overall_reputation_score(const std::string& email);
    std::future<ReputationLevel> get_reputation_level_async(const std::string& email);
    ReputationLevel get_reputation_level(const std::string& email);
    
    // Specific metric building
    std::future<bool> build_account_age_metric_async(const std::string& email);
    bool build_account_age_metric(const std::string& email);
    std::future<bool> build_activity_frequency_metric_async(const std::string& email);
    bool build_activity_frequency_metric(const std::string& email);
    std::future<bool> build_service_diversity_metric_async(const std::string& email);
    bool build_service_diversity_metric(const std::string& email);
    std::future<bool> build_content_quality_metric_async(const std::string& email);
    bool build_content_quality_metric(const std::string& email);
    std::future<bool> build_social_signals_metric_async(const std::string& email);
    bool build_social_signals_metric(const std::string& email);
    std::future<bool> build_trust_score_metric_async(const std::string& email);
    bool build_trust_score_metric(const std::string& email);
    std::future<bool> build_authority_score_metric_async(const std::string& email);
    bool build_authority_score_metric(const std::string& email);
    std::future<bool> build_engagement_rate_metric_async(const std::string& email);
    bool build_engagement_rate_metric(const std::string& email);
    std::future<bool> build_consistency_score_metric_async(const std::string& email);
    bool build_consistency_score_metric(const std::string& email);
    
    // Task management
    std::future<std::vector<ReputationBuildingTask>> get_building_tasks_async(const std::string& email);
    std::vector<ReputationBuildingTask> get_building_tasks(const std::string& email);
    std::future<bool> schedule_custom_task_async(const std::string& email, const ReputationBuildingTask& task);
    bool schedule_custom_task(const std::string& email, const ReputationBuildingTask& task);
    
    // Progress monitoring
    std::future<double> get_building_progress_async(const std::string& email);
    double get_building_progress(const std::string& email);
    std::future<std::chrono::hours> get_estimated_completion_time_async(const std::string& email);
    std::chrono::hours get_estimated_completion_time(const std::string& email);
    std::future<bool> is_building_complete_async(const std::string& email);
    bool is_building_complete(const std::string& email);
    
    // Configuration
    void update_config(const ReputationConfig& config);
    ReputationConfig get_config() const;
    
    // Statistics
    std::map<std::string, ReputationProfile> get_all_profiles() const;
    std::map<ReputationLevel, size_t> get_reputation_distribution() const;
    std::map<ReputationStrategy, double> get_strategy_effectiveness() const;
    std::chrono::milliseconds get_average_building_time() const;
    double get_overall_success_rate() const;
    
    // History and logging
    std::vector<ReputationBuildingTask> get_task_history(size_t limit = 1000);
    bool export_reputation_profiles(const std::string& filename) const;
    bool export_task_history(const std::string& filename) const;

private:
    // Core components
    std::shared_ptr<ActivitySimulator> activity_simulator_;
    std::shared_ptr<GoogleServiceManager> service_manager_;
    std::shared_ptr<ProxyManager> proxy_manager_;
    
    // Configuration
    ReputationConfig config_;
    
    // Reputation profiles
    std::map<std::string, ReputationProfile> reputation_profiles_;
    mutable std::mutex profiles_mutex_;
    
    // Building tasks
    std::map<std::string, std::vector<ReputationBuildingTask>> building_tasks_;
    std::map<std::string, std::thread> building_threads_;
    std::map<std::string, std::atomic<bool>> building_thread_flags_;
    
    // Task history
    std::vector<ReputationBuildingTask> task_history_;
    mutable std::mutex history_mutex_;
    
    // Statistics
    std::map<ReputationStrategy, std::vector<bool>> strategy_results_;
    std::map<ReputationMetric, std::vector<double>> metric_scores_;
    std::atomic<size_t> total_tasks_{0};
    std::atomic<size_t> successful_tasks_{0};
    std::atomic<std::chrono::milliseconds::rep> total_building_time_ms_{0};
    
    // Monitoring
    std::unique_ptr<std::thread> monitoring_thread_;
    std::atomic<bool> monitoring_running_{false};
    std::chrono::seconds monitoring_interval_{3600}; // 1 hour
    
    // Thread safety
    mutable std::mutex builder_mutex_;
    std::atomic<bool> shutdown_requested_{false};
    
    // Random generation
    mutable std::mt19937 rng_;
    std::uniform_real_distribution<double> real_dist_{0.0, 1.0};
    
    // Private methods
    ReputationProfile create_reputation_profile(const std::string& email);
    void update_reputation_profile(const std::string& email, const ReputationBuildingTask& task);
    
    // Main building workflow
    bool execute_reputation_building(const CreationResult& account);
    bool execute_strategy_based_building(const CreationResult& account, ReputationStrategy strategy);
    
    // Metric-specific implementations
    bool execute_account_age_building(const std::string& email);
    bool execute_activity_frequency_building(const std::string& email);
    bool execute_service_diversity_building(const std::string& email);
    bool execute_content_quality_building(const std::string& email);
    bool execute_social_signals_building(const std::string& email);
    bool execute_trust_score_building(const std::string& email);
    bool execute_authority_score_building(const std::string& email);
    bool execute_engagement_rate_building(const std::string& email);
    bool execute_consistency_score_building(const std::string& email);
    
    // Strategy implementations
    bool execute_gradual_building(const std::string& email);
    bool execute_accelerated_building(const std::string& email);
    bool execute_maintenance_building(const std::string& email);
    bool execute_recovery_building(const std::string& email);
    bool execute_stealth_building(const std::string& email);
    bool execute_aggressive_building(const std::string& email);
    
    // Task management
    ReputationBuildingTask create_building_task(const std::string& email, ReputationMetric metric);
    void execute_building_task(const std::string& email, ReputationBuildingTask& task);
    void schedule_next_tasks(const std::string& email);
    std::vector<ReputationMetric> get_next_metrics(const std::string& email);
    
    // Metric calculation
    double calculate_account_age_score(const std::string& email);
    double calculate_activity_frequency_score(const std::string& email);
    double calculate_service_diversity_score(const std::string& email);
    double calculate_content_quality_score(const std::string& email);
    double calculate_social_signals_score(const std::string& email);
    double calculate_trust_score(const std::string& email);
    double calculate_authority_score(const std::string& email);
    double calculate_engagement_rate_score(const std::string& email);
    double calculate_consistency_score(const std::string& email);
    
    // Activity generation
    std::vector<ActivityConfig> generate_activity_schedule(const std::string& email, ReputationStrategy strategy);
    std::vector<GoogleService> select_services_for_diversity(const std::string& email);
    std::chrono::hours calculate_activity_frequency(const std::string& email, ReputationStrategy strategy);
    
    // Content creation
    bool create_high_quality_content(const std::string& email);
    bool engage_with_existing_content(const std::string& email);
    bool maintain_content_consistency(const std::string& email);
    
    // Social building
    bool build_social_connections(const std::string& email);
    bool maintain_social_engagement(const std::string& email);
    bool establish_authority_signals(const std::string& email);
    
    // Trust building
    bool establish_trust_signals(const std::string& email);
    bool maintain_behavioral_consistency(const std::string& email);
    bool demonstrate_reliability(const std::string& email);
    
    // Monitoring and adaptation
    void monitoring_loop();
    void monitor_reputation_changes();
    void adapt_building_strategy(const std::string& email);
    bool check_reputation_drop(const std::string& email);
    void handle_reputation_issues(const std::string& email);
    
    // Progress tracking
    void update_building_progress(const std::string& email);
    double calculate_progress_percentage(const std::string& email);
    std::chrono::hours estimate_remaining_time(const std::string& email);
    
    // Helper methods
    std::string generate_task_id() const;
    std::chrono::milliseconds calculate_task_impact(ReputationMetric metric) const;
    bool is_metric_improving(const std::string& email, ReputationMetric metric);
    std::vector<std::string> get_metric_achievements(ReputationMetric metric, double score);
    std::vector<std::string> get_metric_warnings(ReputationMetric metric, double score);
    
    // Utility methods
    std::string metric_to_string(ReputationMetric metric) const;
    std::string strategy_to_string(ReputationStrategy strategy) const;
    std::string level_to_string(ReputationLevel level) const;
    ReputationMetric string_to_metric(const std::string& metric_str) const;
    ReputationStrategy string_to_strategy(const std::string& strategy_str) const;
    ReputationLevel string_to_level(const std::string& level_str) const;
    
    // Logging
    void log_builder_info(const std::string& message) const;
    void log_builder_error(const std::string& message) const;
    void log_builder_debug(const std::string& message) const;
};

// Reputation analyzer
class ReputationAnalyzer {
public:
    ReputationAnalyzer();
    ~ReputationAnalyzer() = default;
    
    // Profile analysis
    std::map<ReputationMetric, double> analyze_profile_strengths(const ReputationProfile& profile);
    std::map<ReputationMetric, double> analyze_profile_weaknesses(const ReputationProfile& profile);
    std::vector<std::string> identify_improvement_opportunities(const ReputationProfile& profile);
    
    // Comparative analysis
    std::vector<ReputationProfile> identify_top_performers(const std::vector<ReputationProfile>& profiles, size_t limit = 10);
    std::vector<ReputationProfile> identify_underperformers(const std::vector<ReputationProfile>& profiles, size_t limit = 10);
    std::map<ReputationStrategy, double> analyze_strategy_effectiveness(const std::map<ReputationStrategy, std::vector<ReputationProfile>>& strategy_profiles);
    
    // Trend analysis
    std::map<std::string, std::vector<double>> analyze_reputation_trends(const std::vector<ReputationProfile>& profiles_over_time);
    std::vector<std::string> detect_reputation_anomalies(const std::vector<ReputationProfile>& profiles);
    std::map<ReputationMetric, double> calculate_metric_correlations(const std::vector<ReputationProfile>& profiles);
    
    // Predictive analysis
    std::vector<ReputationLevel> predict_future_reputation(const ReputationProfile& current_profile, std::chrono::days future_period);
    std::vector<std::string> predict_potential_issues(const ReputationProfile& profile);
    double predict_success_probability(const ReputationProfile& profile, ReputationLevel target_level);
    
    // Recommendations
    std::vector<std::string> generate_profile_improvement_recommendations(const ReputationProfile& profile);
    std::map<std::string, std::vector<std::string>> generate_strategy_recommendations(const std::map<ReputationStrategy, std::vector<ReputationProfile>>& strategy_profiles);
    std::vector<std::string> generate_metric_prioritization_recommendations(const ReputationProfile& profile);

private:
    std::map<ReputationMetric, std::vector<std::string>> metric_improvement_strategies_;
    std::map<ReputationLevel, std::vector<std::string>> level_characteristics_;
    
    void initialize_analyzers();
    double calculate_profile_strength_score(const ReputationProfile& profile);
    std::vector<ReputationMetric> get_priority_metrics(const ReputationProfile& profile);
    bool is_profile_consistent(const ReputationProfile& profile);
};

// Reputation optimizer
class ReputationOptimizer {
public:
    ReputationOptimizer();
    ~ReputationOptimizer() = default;
    
    // Optimization strategies
    ReputationConfig optimize_config_for_speed(const ReputationConfig& base_config);
    ReputationConfig optimize_config_for_quality(const ReputationConfig& base_config);
    ReputationConfig optimize_config_for_stealth(const ReputationConfig& base_config);
    ReputationConfig optimize_config_for_maintenance(const ReputationConfig& base_config);
    
    // Task optimization
    std::vector<ReputationBuildingTask> optimize_task_sequence(const std::vector<ReputationBuildingTask>& tasks);
    std::vector<ReputationBuildingTask> prioritize_tasks_by_impact(const std::vector<ReputationBuildingTask>& tasks);
    std::vector<ReputationBuildingTask> remove_redundant_tasks(const std::vector<ReputationBuildingTask>& tasks);
    
    // Resource optimization
    std::map<std::string, std::chrono::hours> optimize_resource_allocation(const std::vector<std::string>& emails, const std::map<std::string, ReputationProfile>& profiles);
    std::vector<std::string> prioritize_accounts_by_potential(const std::map<std::string, ReputationProfile>& profiles);
    std::map<std::string, ReputationStrategy> recommend_optimal_strategies(const std::map<std::string, ReputationProfile>& profiles);
    
    // Performance optimization
    std::vector<std::string> identify_performance_bottlenecks(const std::vector<ReputationBuildingTask>& tasks);
    std::map<std::string, std::string> suggest_efficiency_improvements(const std::map<std::string, std::vector<ReputationBuildingTask>>& account_tasks);

private:
    std::map<ReputationStrategy, std::vector<std::string>> strategy_optimizations_;
    std::map<ReputationMetric, std::vector<std::string>> metric_optimizations_;
    
    void initialize_optimizations();
    double calculate_task_efficiency(const ReputationBuildingTask& task);
    std::vector<ReputationBuildingTask> reorder_tasks_by_dependencies(const std::vector<ReputationBuildingTask>& tasks);
};

} // namespace gmail_infinity
