#pragma once

#include <string>
#include <vector>
#include <memory>
#include <map>
#include <random>
#include <mutex>

namespace gmail_infinity {

struct NameData {
    std::string name;
    std::string gender; // "male", "female", "unisex"
    std::string origin; // country/culture of origin
    std::string meaning; // meaning of the name
    double popularity; // popularity score (0.0 to 1.0)
    std::vector<std::string> variations; // spelling variations
};

struct SurnameData {
    std::string surname;
    std::string origin;
    std::string meaning;
    double popularity;
    std::vector<std::string> variations;
};

struct NameCombination {
    std::string first_name;
    std::string middle_name;
    std::string last_name;
    std::string full_name;
    std::string display_name;
    std::string initials;
    double compatibility_score;
    std::string cultural_context;
};

enum class NameGenerationStrategy {
    RANDOM,
    WEIGHTED_BY_POPULARITY,
    CULTURAL_AUTHENTIC,
    MODERN_TRENDING,
    CLASSIC_TRADITIONAL,
    PHONETIC_BALANCE,
    MEANING_BASED
};

class NameGenerator {
public:
    NameGenerator();
    ~NameGenerator() = default;
    
    // Initialization
    bool load_name_data(const std::string& data_directory = "data/names/");
    bool save_name_data(const std::string& data_directory = "data/names/");
    
    // Basic name generation
    std::string generate_first_name(const std::string& gender = "any", const std::string& culture = "any");
    std::string generate_last_name(const std::string& culture = "any");
    std::string generate_middle_name(const std::string& gender = "any", const std::string& culture = "any");
    
    // Full name generation
    NameCombination generate_full_name(const std::string& gender = "any", const std::string& culture = "any");
    std::vector<NameCombination> generate_multiple_names(size_t count, const std::string& gender = "any", const std::string& culture = "any");
    
    // Advanced generation with constraints
    NameCombination generate_name_with_constraints(
        const std::map<std::string, std::string>& constraints);
    NameCombination generate_name_for_age_group(int age, const std::string& culture = "any");
    NameCombination generate_name_with_meaning(const std::string& meaning_theme);
    
    // Cultural and regional generation
    NameCombination generate_american_name(const std::string& gender = "any");
    NameCombination generate_british_name(const std::string& gender = "any");
    NameCombination generate_european_name(const std::string& country = "any", const std::string& gender = "any");
    NameCombination generate_asian_name(const std::string& country = "any", const std::string& gender = "any");
    NameCombination generate_african_name(const std::string& country = "any", const std::string& gender = "any");
    NameCombination generate_hispanic_name(const std::string& gender = "any");
    
    // Name validation and analysis
    bool validate_name_combination(const NameCombination& name);
    double calculate_name_compatibility(const std::string& first_name, const std::string& last_name);
    std::vector<std::string> get_name_suggestions(const std::string& partial_name, const std::string& gender = "any");
    
    // Nickname generation
    std::vector<std::string> generate_nicknames(const std::string& first_name);
    std::string generate_primary_nickname(const std::string& first_name);
    
    // Name variations and alternatives
    std::vector<std::string> get_name_variations(const std::string& name);
    std::vector<std::string> get_spelling_alternatives(const std::string& name);
    NameCombination generate_alternative_combination(const NameCombination& original);
    
    // Phonetic analysis
    std::string get_phonetic_pronunciation(const std::string& name);
    double calculate_phonetic_balance(const NameCombination& name);
    bool has_good_rhythm(const NameCombination& name);
    
    // Trend analysis
    std::vector<std::string> get_trending_names(const std::string& gender, int year = 2023);
    std::vector<std::string> get_classic_names(const std::string& gender);
    std::vector<std::string> get_unique_names(const std::string& gender);
    
    // Configuration
    void set_generation_strategy(NameGenerationStrategy strategy);
    void set_cultural_preference(const std::string& culture);
    void set_popularity_weight(double weight); // 0.0 = unique, 1.0 = popular
    void set_meaning_importance(double importance); // 0.0 = ignore, 1.0 = important
    
    // Statistics and analytics
    std::map<std::string, size_t> get_name_distribution() const;
    std::vector<std::string> get_most_popular_names(const std::string& gender, size_t limit = 10);
    std::vector<std::string> get_least_popular_names(const std::string& gender, size_t limit = 10);
    
    // Data management
    bool add_name_data(const NameData& name_data);
    bool add_surname_data(const SurnameData& surname_data);
    bool remove_name(const std::string& name);
    bool update_name_popularity(const std::string& name, double popularity);
    
    // Export/Import
    bool export_name_database(const std::string& filename) const;
    bool import_name_database(const std::string& filename);

private:
    // Data storage
    std::map<std::string, std::vector<NameData>> first_names_by_gender_;
    std::map<std::string, std::vector<NameData>> first_names_by_culture_;
    std::map<std::string, std::vector<SurnameData>> surnames_by_culture_;
    std::map<std::string, NameData> name_lookup_;
    std::map<std::string, SurnameData> surname_lookup_;
    
    // Configuration
    NameGenerationStrategy strategy_{NameGenerationStrategy::WEIGHTED_BY_POPULARITY};
    std::string cultural_preference_{"western"};
    double popularity_weight_{0.7};
    double meaning_importance_{0.3};
    
    // Random generation
    mutable std::mt19937 rng_;
    std::uniform_real_distribution<double> real_dist_{0.0, 1.0};
    
    // Thread safety
    mutable std::mutex generator_mutex_;
    
    // Private generation methods
    NameData select_first_name(const std::string& gender, const std::string& culture);
    SurnameData select_surname(const std::string& culture);
    std::string select_middle_name(const std::string& gender, const std::string& culture, const std::string& first_name);
    
    // Strategy-specific methods
    NameData select_random_name(const std::vector<NameData>& names);
    NameData select_weighted_name(const std::vector<NameData>& names);
    NameData select_culturally_authentic_name(const std::vector<NameData>& names);
    NameData select_trending_name(const std::vector<NameData>& names);
    NameData select_classic_name(const std::vector<NameData>& names);
    NameData select_phonetically_balanced_name(const std::vector<NameData>& names, const std::string& last_name);
    NameData select_meaning_based_name(const std::vector<NameData>& names, const std::string& meaning_theme);
    
    // Cultural adaptation methods
    void adapt_name_to_culture(NameCombination& name, const std::string& culture);
    std::string get_cultural_naming_pattern(const std::string& culture);
    std::vector<std::string> get_cultural_middle_names(const std::string& culture, const std::string& gender);
    
    // Validation and analysis methods
    bool is_name_appropriate_for_age(const std::string& name, int age);
    bool is_name_culturally_consistent(const NameCombination& name);
    double calculate_phonetic_flow(const std::string& first_name, const std::string& middle_name, const std::string& last_name);
    
    // Utility methods
    std::string format_full_name(const std::string& first, const std::string& middle, const std::string& last);
    std::string generate_initials(const std::string& first, const std::string& middle, const std::string& last);
    std::string normalize_name(const std::string& name);
    std::string capitalize_name(const std::string& name);
    
    // Data loading helpers
    bool load_first_names_from_file(const std::string& filename);
    bool load_surnames_from_file(const std::string& filename);
    bool parse_name_line(const std::string& line, NameData& name_data);
    bool parse_surname_line(const std::string& line, SurnameData& surname_data);
    
    // Phonetic analysis helpers
    std::vector<std::string> get_syllables(const std::string& name);
    int count_syllables(const std::string& name);
    char get_last_vowel(const std::string& name);
    char get_first_consonant(const std::string& name);
    
    // Logging
    void log_name_info(const std::string& message) const;
    void log_name_error(const std::string& message) const;
    void log_name_debug(const std::string& message) const;
};

// Name validator for advanced validation rules
class NameValidator {
public:
    NameValidator();
    ~NameValidator() = default;
    
    bool validate_name(const NameCombination& name);
    std::vector<std::string> get_validation_issues(const NameCombination& name);
    
    // Specific validation rules
    bool validate_length(const NameCombination& name);
    bool validate_characters(const std::string& name);
    bool validate_pronunciation(const NameCombination& name);
    bool validate_cultural_appropriateness(const NameCombination& name);
    bool validate_age_appropriateness(const NameCombination& name, int age);
    
    // Scoring
    double calculate_quality_score(const NameCombination& name);
    double calculate_authenticity_score(const NameCombination& name);
    double calculate_memorability_score(const NameCombination& name);

private:
    std::map<std::string, std::vector<std::string>> validation_rules_;
    
    bool has_proper_capitalization(const std::string& name);
    bool has_appropriate_length(const std::string& name);
    bool contains_invalid_characters(const std::string& name);
    bool has_good_phonetic_flow(const NameCombination& name);
};

// Name trend analyzer
class NameTrendAnalyzer {
public:
    NameTrendAnalyzer();
    ~NameTrendAnalyzer() = default;
    
    bool load_historical_data(const std::string& filename);
    std::vector<std::string> get_trending_names(int year, const std::string& gender);
    std::vector<std::string> get_rising_names(const std::string& gender);
    std::vector<std::string> get_declining_names(const std::string& gender);
    
    // Trend analysis
    double calculate_trend_score(const std::string& name, int current_year);
    std::vector<std::string> predict_future_trends(const std::string& gender, int years_ahead = 5);
    std::map<std::string, double> get_name_popularity_history(const std::string& name);
    
    // Regional trends
    std::vector<std::string> get_regional_trends(const std::string& region, const std::string& gender);
    std::map<std::string, std::vector<std::string>> get_all_regional_trends(const std::string& gender);

private:
    std::map<int, std::map<std::string, double>> historical_popularity_;
    std::map<std::string, std::map<std::string, std::vector<std::string>>> regional_trends_;
    
    void analyze_trends();
    std::vector<std::string> calculate_rising_names(const std::string& gender);
    std::vector<std::string> calculate_declining_names(const std::string& gender);
};

} // namespace gmail_infinity
