#pragma once

#include <string>
#include <vector>
#include <memory>
#include <map>
#include <random>
#include <mutex>

#include "persona_generator.h"
#include "name_generator.h"

namespace gmail_infinity {

enum class BioTone {
    PROFESSIONAL,
    CASUAL,
    FRIENDLY,
    FORMAL,
    CREATIVE,
    TECHNICAL,
    ACADEMIC,
    HUMOROUS,
    INSPIRATIONAL,
    MINIMALIST
};

enum class BioLength {
    VERY_SHORT,  // 1-2 sentences
    SHORT,       // 3-4 sentences
    MEDIUM,      // 1-2 paragraphs
    LONG,        // 3-4 paragraphs
    VERY_LONG    // 5+ paragraphs
};

struct BioTemplate {
    std::string id;
    std::string name;
    std::string description;
    BioTone tone;
    BioLength length;
    std::vector<std::string> placeholders;
    std::vector<std::string> template_text;
    double quality_score;
    std::vector<std::string> suitable_personas;
    
    nlohmann::json to_json() const;
    void from_json(const nlohmann::json& j);
};

struct GeneratedBio {
    std::string content;
    BioTone tone;
    BioLength length;
    std::string template_id;
    std::map<std::string, std::string> used_placeholders;
    double quality_score;
    std::chrono::system_clock::time_point generated_at;
    
    nlohmann::json to_json() const;
    void from_json(const nlohmann::json& j);
};

class BioGenerator {
public:
    BioGenerator();
    ~BioGenerator() = default;
    
    // Initialization
    bool load_templates(const std::string& templates_file = "data/bio_templates.json");
    bool save_templates(const std::string& templates_file = "data/bio_templates.json");
    bool load_vocabulary(const std::string& vocab_file = "data/bio_vocabulary.json");
    
    // Basic bio generation
    GeneratedBio generate_bio(const HumanPersona& persona);
    GeneratedBio generate_bio_with_tone(const HumanPersona& persona, BioTone tone);
    GeneratedBio generate_bio_with_length(const HumanPersona& persona, BioLength length);
    
    // Advanced bio generation
    GeneratedBio generate_bio_for_platform(const HumanPersona& persona, const std::string& platform);
    GeneratedBio generate_bio_for_purpose(const HumanPersona& persona, const std::string& purpose);
    GeneratedBio generate_bio_with_constraints(
        const HumanPersona& persona,
        const std::map<std::string, std::string>& constraints);
    
    // Template-based generation
    GeneratedBio generate_from_template(const HumanPersona& persona, const std::string& template_id);
    std::vector<std::string> get_suitable_templates(const HumanPersona& persona);
    
    // Multi-variant generation
    std::vector<GeneratedBio> generate_multiple_bios(
        const HumanPersona& persona,
        size_t count = 5);
    std::vector<GeneratedBio> generate_all_tones(const HumanPersona& persona);
    std::vector<GeneratedBio> generate_all_lengths(const HumanPersona& persona);
    
    // Bio customization
    GeneratedBio customize_bio(
        const GeneratedBio& base_bio,
        const std::map<std::string, std::string>& customizations);
    GeneratedBio blend_bios(const std::vector<GeneratedBio>& bios);
    
    // Platform-specific bios
    GeneratedBio generate_linkedin_bio(const HumanPersona& persona);
    GeneratedBio generate_twitter_bio(const HumanPersona& persona);
    GeneratedBio generate_instagram_bio(const HumanPersona& persona);
    GeneratedBio generate_facebook_bio(const HumanPersona& persona);
    GeneratedBio generate_gmail_signature(const HumanPersona& persona);
    GeneratedBio generate_forum_signature(const HumanPersona& persona);
    
    // Industry and profession specific
    GeneratedBio generate_professional_bio(const HumanPersona& persona);
    GeneratedBio generate_creative_bio(const HumanPersona& persona);
    GeneratedBio generate_technical_bio(const HumanPersona& persona);
    GeneratedBio generate_academic_bio(const HumanPersona& persona);
    GeneratedBio generate_entrepreneur_bio(const HumanPersona& persona);
    
    // Bio analysis and validation
    bool validate_bio(const GeneratedBio& bio);
    std::vector<std::string> get_validation_issues(const GeneratedBio& bio);
    double calculate_quality_score(const GeneratedBio& bio);
    double calculate_authenticity_score(const GeneratedBio& bio, const HumanPersona& persona);
    
    // Content analysis
    std::map<std::string, int> analyze_sentiment(const GeneratedBio& bio);
    std::vector<std::string> extract_keywords(const GeneratedBio& bio);
    std::string detect_primary_theme(const GeneratedBio& bio);
    std::vector<std::string> get_call_to_actions(const GeneratedBio& bio);
    
    // Template management
    bool add_template(const BioTemplate& template_obj);
    bool remove_template(const std::string& template_id);
    bool update_template(const BioTemplate& template_obj);
    std::vector<BioTemplate> get_all_templates();
    std::vector<BioTemplate> get_templates_by_tone(BioTone tone);
    std::vector<BioTemplate> get_templates_by_length(BioLength length);
    
    // Vocabulary and phrases
    std::vector<std::string> get_vocabulary_for_tone(BioTone tone);
    std::vector<std::string> get_phrases_for_industry(const std::string& industry);
    std::vector<std::string> get_opening_sentences(BioTone tone);
    std::vector<std::string> get_closing_sentences(BioTone tone);
    
    // Configuration
    void set_default_tone(BioTone tone);
    void set_default_length(BioLength length);
    void set_quality_threshold(double threshold);
    void set_creativity_level(double level); // 0.0 = conservative, 1.0 = creative
    
    // Statistics and analytics
    std::map<BioTone, size_t> get_tone_distribution() const;
    std::map<BioLength, size_t> get_length_distribution() const;
    std::vector<std::string> get_most_used_templates(size_t limit = 10);
    double get_average_quality_score() const;
    
    // Export/Import
    bool export_bio_library(const std::string& filename) const;
    bool import_bio_library(const std::string& filename);

private:
    // Template storage
    std::map<std::string, BioTemplate> templates_;
    std::map<BioTone, std::vector<std::string>> template_ids_by_tone_;
    std::map<BioLength, std::vector<std::string>> template_ids_by_length_;
    
    // Vocabulary and phrases
    std::map<BioTone, std::vector<std::string>> vocabulary_by_tone_;
    std::map<std::string, std::vector<std::string>> industry_phrases_;
    std::map<BioTone, std::vector<std::string>> opening_sentences_;
    std::map<BioTone, std::vector<std::string>> closing_sentences_;
    
    // Configuration
    BioTone default_tone_{BioTone::PROFESSIONAL};
    BioLength default_length_{BioLength::MEDIUM};
    double quality_threshold_{0.7};
    double creativity_level_{0.5};
    
    // Statistics
    std::map<std::string, size_t> template_usage_count_;
    std::atomic<double> total_quality_score_{0.0};
    std::atomic<size_t> total_bios_generated_{0};
    
    // Random generation
    mutable std::mt19937 rng_;
    std::uniform_real_distribution<double> real_dist_{0.0, 1.0};
    
    // Thread safety
    mutable std::mutex generator_mutex_;
    
    // Private generation methods
    GeneratedBio generate_from_template_internal(
        const HumanPersona& persona,
        const BioTemplate& template_obj);
    std::string fill_placeholders(
        const BioTemplate& template_obj,
        const HumanPersona& persona);
    std::string select_appropriate_template(const HumanPersona& persona, BioTone tone, BioLength length);
    
    // Content generation helpers
    std::string generate_professional_content(const HumanPersona& persona);
    std::string generate_personal_content(const HumanPersona& persona);
    std::string generate_hobby_content(const HumanPersona& persona);
    std::string generate_achievement_content(const HumanPersona& persona);
    std::string generate_aspiration_content(const HumanPersona& persona);
    
    // Placeholder filling
    std::string fill_name_placeholders(const std::string& template_text, const HumanPersona& persona);
    std::string fill_profession_placeholders(const std::string& template_text, const HumanPersona& persona);
    std::string fill_location_placeholders(const std::string& template_text, const HumanPersona& persona);
    std::string fill_hobby_placeholders(const std::string& template_text, const HumanPersona& persona);
    std::string fill_personality_placeholders(const std::string& template_text, const HumanPersona& persona);
    std::string fill_achievement_placeholders(const std::string& template_text, const HumanPersona& persona);
    
    // Quality assessment
    double calculate_grammar_score(const std::string& content);
    double calculate_readability_score(const std::string& content);
    double calculate_coherence_score(const std::string& content);
    double calculate_relevance_score(const std::string& content, const HumanPersona& persona);
    double calculate_uniqueness_score(const std::string& content);
    
    // Content optimization
    std::string optimize_for_length(const std::string& content, BioLength target_length);
    std::string optimize_for_tone(const std::string& content, BioTone target_tone);
    std::string optimize_for_platform(const std::string& content, const std::string& platform);
    
    // Template selection
    std::vector<BioTemplate> filter_templates_by_persona(const HumanPersona& persona);
    std::vector<BioTemplate> filter_templates_by_constraints(
        const std::map<std::string, std::string>& constraints);
    BioTemplate select_best_template(const std::vector<BioTemplate>& candidates);
    
    // Vocabulary management
    void load_default_vocabulary();
    void load_industry_phrases();
    void load_sentence_templates();
    
    // Utility methods
    std::string capitalize_sentences(const std::string& text);
    std::string remove_extra_whitespace(const std::string& text);
    std::string ensure_proper_punctuation(const std::string& text);
    std::string truncate_to_length(const std::string& text, size_t max_length);
    std::string expand_to_length(const std::string& text, size_t target_length);
    
    // Platform-specific optimization
    std::string optimize_for_linkedin(const std::string& content);
    std::string optimize_for_twitter(const std::string& content);
    std::string optimize_for_instagram(const std::string& content);
    std::string optimize_for_facebook(const std::string& content);
    
    // Sentiment analysis
    std::map<std::string, int> analyze_sentiment_basic(const std::string& text);
    std::string detect_sentiment_label(const std::map<std::string, int>& sentiment_scores);
    
    // Keyword extraction
    std::vector<std::string> extract_keywords_basic(const std::string& text);
    std::vector<std::string> extract_professional_keywords(const HumanPersona& persona);
    std::vector<std::string> extract_personal_keywords(const HumanPersona& persona);
    
    // Template creation
    BioTemplate create_default_template(BioTone tone, BioLength length);
    std::vector<BioTemplate> create_professional_templates();
    std::vector<BioTemplate> create_casual_templates();
    std::vector<BioTemplate> create_creative_templates();
    std::vector<BioTemplate> create_technical_templates();
    
    // Logging
    void log_bio_info(const std::string& message) const;
    void log_bio_error(const std::string& message) const;
    void log_bio_debug(const std::string& message) const;
};

// Bio quality analyzer
class BioQualityAnalyzer {
public:
    BioQualityAnalyzer();
    ~BioQualityAnalyzer() = default;
    
    double analyze_quality(const GeneratedBio& bio);
    std::vector<std::string> get_quality_issues(const GeneratedBio& bio);
    std::map<std::string, double> get_quality_metrics(const GeneratedBio& bio);
    
    // Specific quality aspects
    double analyze_grammar(const std::string& content);
    double analyze_readability(const std::string& content);
    double analyze_coherence(const std::string& content);
    double analyze_authenticity(const GeneratedBio& bio, const HumanPersona& persona);
    double analyze_engagement(const std::string& content);
    double analyze_completeness(const GeneratedBio& bio, const HumanPersona& persona);
    
    // Platform-specific quality
    double analyze_linkedin_quality(const GeneratedBio& bio);
    double analyze_twitter_quality(const GeneratedBio& bio);
    double analyze_instagram_quality(const GeneratedBio& bio);

private:
    std::map<std::string, double> quality_weights_;
    
    // Grammar checking
    bool has_grammar_errors(const std::string& content);
    std::vector<std::string> find_grammar_issues(const std::string& content);
    
    // Readability analysis
    double calculate_flesch_reading_ease(const std::string& content);
    double calculate_flesch_kincaid_grade(const std::string& content);
    int count_sentences(const std::string& content);
    int count_words(const std::string& content);
    int count_syllables(const std::string& word);
    
    // Coherence analysis
    bool has_logical_flow(const std::string& content);
    bool has_consistent_tone(const std::string& content);
    double calculate_coherence_score(const std::string& content);
};

// Bio template manager
class BioTemplateManager {
public:
    BioTemplateManager();
    ~BioTemplateManager() = default;
    
    bool load_templates(const std::string& filename);
    bool save_templates(const std::string& filename);
    
    // Template CRUD operations
    bool create_template(const BioTemplate& template_obj);
    std::shared_ptr<BioTemplate> read_template(const std::string& template_id);
    bool update_template(const BioTemplate& template_obj);
    bool delete_template(const std::string& template_id);
    
    // Template search and filtering
    std::vector<BioTemplate> search_templates(const std::string& query);
    std::vector<BioTemplate> filter_by_tone(BioTone tone);
    std::vector<BioTemplate> filter_by_length(BioLength length);
    std::vector<BioTemplate> filter_by_quality(double min_quality);
    
    // Template analysis
    std::vector<BioTemplate> get_most_used_templates(size_t limit = 10);
    std::vector<BioTemplate> get_highest_quality_templates(size_t limit = 10);
    std::map<BioTone, size_t> get_template_distribution();

private:
    std::map<std::string, BioTemplate> templates_;
    std::map<std::string, std::chrono::system_clock::time_point> template_usage_times_;
    mutable std::mutex template_mutex_;
    
    void load_default_templates();
    void validate_template(const BioTemplate& template_obj);
};

} // namespace gmail_infinity
