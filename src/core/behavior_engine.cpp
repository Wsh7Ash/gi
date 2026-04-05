#include "behavior_engine.h"
#include "app/app_logger.h"

#include <cmath>
#include <algorithm>
#include <unordered_map>
#include <mutex>

namespace gmail_infinity {

// Adjacent keyboard keys for typo simulation
const std::unordered_map<char, std::string> BehaviorEngine::ADJACENT_KEYS = {
    {'a', "sqwz"}, {'b', "vghn"}, {'c', "xdfv"}, {'d', "sefxc"}, {'e', "wsdr"},
    {'f', "drtgv"}, {'g', "ftyhn"}, {'h', "gyujb"}, {'i', "ujko"}, {'j', "huikm"},
    {'k', "jiol"}, {'l', "kop;"}, {'m', "njk,"}, {'n', "bhjm"}, {'o', "iklp"},
    {'p', "ol;["}, {'q', "wa"}, {'r', "edft"}, {'s', "aedwz"}, {'t', "rfgy"},
    {'u', "yhji"}, {'v', "cfgb"}, {'w', "qsea"}, {'x', "zsdc"}, {'y', "tghu"},
    {'z', "asx"}
};

BehaviorEngine::BehaviorEngine() {
    std::random_device rd;
    rng_.seed(rd());
    apply_speed_profile();
}

void BehaviorEngine::seed(uint64_t seed_value) {
    rng_.seed(seed_value);
}

void BehaviorEngine::set_speed_profile(const std::string& profile) {
    speed_profile_ = profile;
    apply_speed_profile();
}

void BehaviorEngine::apply_speed_profile() {
    if (speed_profile_ == "slow") {
        typing_profile_.base_delay_ms    = 180.0;
        typing_profile_.delay_variance_ms = 60.0;
    } else if (speed_profile_ == "fast") {
        typing_profile_.base_delay_ms    = 60.0;
        typing_profile_.delay_variance_ms = 20.0;
    } else { // normal
        typing_profile_.base_delay_ms    = 120.0;
        typing_profile_.delay_variance_ms = 40.0;
    }
}

double BehaviorEngine::gaussian(double mean, double stddev) const {
    std::normal_distribution<double> dist(mean, stddev);
    return dist(rng_);
}

double BehaviorEngine::uniform(double lo, double hi) const {
    std::uniform_real_distribution<double> dist(lo, hi);
    return dist(rng_);
}

std::chrono::milliseconds BehaviorEngine::random_delay(double mean_ms, double stddev_ms) const {
    double ms = gaussian(mean_ms, stddev_ms);
    ms = std::max(10.0, ms); // floor at 10ms
    return std::chrono::milliseconds(static_cast<long long>(ms));
}

std::chrono::milliseconds BehaviorEngine::short_pause() const  { return random_delay(125.0, 50.0); }
std::chrono::milliseconds BehaviorEngine::medium_pause() const { return random_delay(1000.0, 250.0); }
std::chrono::milliseconds BehaviorEngine::long_pause() const   { return random_delay(3500.0, 800.0); }
std::chrono::milliseconds BehaviorEngine::think_pause() const  { return random_delay(2000.0, 500.0); }

bool BehaviorEngine::should_pause_randomly(double probability) const {
    std::uniform_real_distribution<double> dist(0.0, 1.0);
    return dist(rng_) < probability;
}

bool BehaviorEngine::should_make_typo(double probability) const {
    std::uniform_real_distribution<double> dist(0.0, 1.0);
    return dist(rng_) < probability;
}

char BehaviorEngine::get_adjacent_key(char c) const {
    auto it = ADJACENT_KEYS.find(std::tolower(c));
    if (it == ADJACENT_KEYS.end()) return c;
    const auto& neighbors = it->second;
    std::uniform_int_distribution<size_t> dis(0, neighbors.size() - 1);
    return neighbors[dis(rng_)];
}

TypingProfile BehaviorEngine::get_typing_profile() const { return typing_profile_; }

std::vector<std::pair<char, std::chrono::milliseconds>>
BehaviorEngine::generate_keystrokes(const std::string& text, const TypingProfile& profile) const {
    std::vector<std::pair<char, std::chrono::milliseconds>> keystrokes;
    keystrokes.reserve(text.size() * 2);

    for (size_t i = 0; i < text.size(); ++i) {
        char c = text[i];

        // Possibly inject a typo + backspace correction
        if (should_make_typo(profile.typo_probability) && std::isalpha(c)) {
            char wrong = get_adjacent_key(c);
            auto typo_delay = random_delay(profile.base_delay_ms, profile.delay_variance_ms);
            keystrokes.emplace_back(wrong, typo_delay);
            // correction delay before backspace
            keystrokes.emplace_back('\b',
                std::chrono::milliseconds(static_cast<long long>(profile.correction_delay_ms)));
        }

        auto delay = random_delay(profile.base_delay_ms, profile.delay_variance_ms);

        // Extra pause at word boundaries
        if (c == ' ' && i > 0) {
            delay += std::chrono::milliseconds(
                static_cast<long long>(profile.pause_after_word_ms));
        }
        keystrokes.emplace_back(c, delay);
    }
    return keystrokes;
}

void BehaviorEngine::simulate_typing_delay(const std::string& text, const TypingProfile& profile) const {
    auto keystrokes = generate_keystrokes(text, profile);
    for (auto& [c, delay] : keystrokes) {
        std::this_thread::sleep_for(delay);
        (void)c;
    }
}

// ─── Bézier curve ───────────────────────────────────────

MousePosition BezierCurve::evaluate(double t) const {
    double u = 1.0 - t;
    double tt = t * t, uu = u * u;
    double uuu = uu * u, ttt = tt * t;
    return {
        uuu * p0.x + 3 * uu * t * p1.x + 3 * u * tt * p2.x + ttt * p3.x,
        uuu * p0.y + 3 * uu * t * p1.y + 3 * u * tt * p2.y + ttt * p3.y
    };
}

std::vector<MousePosition> BezierCurve::sample(int steps) const {
    std::vector<MousePosition> pts;
    pts.reserve(steps + 1);
    for (int i = 0; i <= steps; ++i) {
        pts.push_back(evaluate(static_cast<double>(i) / steps));
    }
    return pts;
}

BezierCurve BehaviorEngine::generate_mouse_curve(MousePosition from, MousePosition to) const {
    // Generate two random control points for a natural curve
    double dx = to.x - from.x, dy = to.y - from.y;
    BezierCurve bc;
    bc.p0 = from;
    bc.p3 = to;
    bc.p1 = {from.x + uniform(0.1, 0.5) * dx + uniform(-dy * 0.2, dy * 0.2),
             from.y + uniform(0.1, 0.5) * dy + uniform(-dx * 0.2, dx * 0.2)};
    bc.p2 = {to.x - uniform(0.1, 0.5) * dx + uniform(-dy * 0.2, dy * 0.2),
             to.y - uniform(0.1, 0.5) * dy + uniform(-dx * 0.2, dx * 0.2)};
    return bc;
}

std::vector<MousePosition> BehaviorEngine::generate_mouse_path(
    MousePosition from, MousePosition to, int waypoints) const {
    std::vector<MousePosition> path;
    MousePosition current = from;
    for (int w = 0; w <= waypoints; ++w) {
        MousePosition target = (w == waypoints) ? to : MousePosition{
            from.x + (to.x - from.x) * uniform(0.2, 0.8),
            from.y + (to.y - from.y) * uniform(0.2, 0.8)
        };
        auto curve = generate_mouse_curve(current, target);
        auto pts   = curve.sample(15);
        for (auto& p : pts) path.push_back(p);
        current = target;
    }
    return path;
}

// ─── Scroll generation ──────────────────────────────────

ScrollProfile BehaviorEngine::generate_scroll_profile() const {
    return {uniform(100.0, 150.0), uniform(20.0, 40.0), static_cast<int>(uniform(2, 5)),
            std::chrono::milliseconds(static_cast<long long>(uniform(60, 120)))};
}

std::vector<int> BehaviorEngine::generate_scroll_sequence(int total_pixels) const {
    std::vector<int> steps;
    int remaining = total_pixels;
    while (remaining > 0) {
        int chunk = static_cast<int>(std::min<double>(
            gaussian(120.0, 30.0), remaining));
        chunk = std::max(10, chunk);
        steps.push_back(chunk);
        remaining -= chunk;
    }
    return steps;
}

// ─── VelocityTracker ────────────────────────────────────

VelocityTracker::VelocityTracker() = default;

void VelocityTracker::record_action(const std::string& /*action_type*/) {
    std::lock_guard lock(mutex_);
    action_timestamps_.push_back(std::chrono::steady_clock::now());
}

double VelocityTracker::get_actions_per_minute() const {
    std::lock_guard lock(mutex_);
    if (action_timestamps_.empty()) return 0.0;
    auto now    = std::chrono::steady_clock::now();
    auto window = now - std::chrono::minutes(1);
    size_t cnt  = std::count_if(action_timestamps_.begin(), action_timestamps_.end(),
        [&](auto& tp) { return tp >= window; });
    return static_cast<double>(cnt);
}

bool VelocityTracker::is_velocity_too_high(double threshold) const {
    return get_actions_per_minute() > threshold;
}

std::chrono::milliseconds VelocityTracker::get_required_cooldown() const {
    double apm = get_actions_per_minute();
    if (apm < 30.0) return std::chrono::milliseconds(0);
    return std::chrono::milliseconds(static_cast<long long>((apm - 30.0) * 500));
}

void VelocityTracker::reset() {
    std::lock_guard lock(mutex_);
    action_timestamps_.clear();
}

} // namespace gmail_infinity
