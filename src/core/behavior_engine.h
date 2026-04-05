#pragma once

#include <string>
#include <vector>
#include <memory>
#include <random>
#include <chrono>
#include <functional>
#include <thread>

#include <nlohmann/json.hpp>

namespace gmail_infinity {

struct MousePosition {
    double x{0.0};
    double y{0.0};
};

struct BezierCurve {
    MousePosition p0; // start
    MousePosition p1; // control 1
    MousePosition p2; // control 2
    MousePosition p3; // end

    MousePosition evaluate(double t) const;
    std::vector<MousePosition> sample(int steps = 20) const;
};

struct TypingProfile {
    double base_delay_ms{120.0};      // avg ms between keystrokes
    double delay_variance_ms{40.0};   // gaussian std dev
    double typo_probability{0.02};    // prob of making a typo
    double correction_delay_ms{250.0};
    bool use_shift_for_capitals{true};
    double pause_after_word_ms{40.0};
};

struct ScrollProfile {
    double pixels_per_scroll{120.0};
    double scroll_variance{30.0};
    int scroll_steps{3};
    std::chrono::milliseconds step_delay{80};
};

class BehaviorEngine {
public:
    BehaviorEngine();
    ~BehaviorEngine() = default;

    // Human-like typing
    std::vector<std::pair<char, std::chrono::milliseconds>> generate_keystrokes(
        const std::string& text, const TypingProfile& profile = {}) const;

    void simulate_typing_delay(const std::string& text,
                               const TypingProfile& profile = {}) const;

    // Bézier mouse movement
    BezierCurve generate_mouse_curve(MousePosition from, MousePosition to) const;
    std::vector<MousePosition> generate_mouse_path(MousePosition from,
                                                    MousePosition to,
                                                    int waypoints = 2) const;

    // Timing helpers
    std::chrono::milliseconds random_delay(double mean_ms, double stddev_ms) const;
    std::chrono::milliseconds short_pause() const;   // 50–200ms
    std::chrono::milliseconds medium_pause() const;  // 500–1500ms
    std::chrono::milliseconds long_pause() const;    // 2000–5000ms
    std::chrono::milliseconds think_pause() const;   // 1000–3000ms

    // Scroll simulation
    ScrollProfile generate_scroll_profile() const;
    std::vector<int> generate_scroll_sequence(int total_pixels) const;

    // Action randomisation
    bool should_pause_randomly(double probability = 0.15) const;
    bool should_make_typo(double probability = 0.02) const;
    char get_adjacent_key(char c) const;

    // Profiles
    void set_speed_profile(const std::string& profile); // "slow", "normal", "fast"
    TypingProfile get_typing_profile() const;

    // Seed for reproducibility (testing)
    void seed(uint64_t seed_value);

private:
    mutable std::mt19937_64 rng_;
    TypingProfile typing_profile_;
    std::string speed_profile_{"normal"};

    void apply_speed_profile();
    double gaussian(double mean, double stddev) const;
    double uniform(double lo, double hi) const;

    static const std::unordered_map<char, std::string> ADJACENT_KEYS;
};

// Velocity tracker for anti-detection
class VelocityTracker {
public:
    VelocityTracker();
    ~VelocityTracker() = default;

    void record_action(const std::string& action_type);
    double get_actions_per_minute() const;
    bool is_velocity_too_high(double threshold = 30.0) const;
    std::chrono::milliseconds get_required_cooldown() const;
    void reset();

private:
    std::vector<std::chrono::steady_clock::time_point> action_timestamps_;
    mutable std::mutex mutex_;
    std::chrono::minutes window_{1};
};

} // namespace gmail_infinity
