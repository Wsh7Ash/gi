#pragma once

#include <string>
#include <vector>
#include <memory>
#include <map>
#include <random>
#include <mutex>
#include <chrono>

#include <curl/curl.h>
#include <nlohmann/json.hpp>

#include "persona_generator.h"

namespace gmail_infinity {

enum class PhotoStyle {
    PROFESSIONAL_HEADSHOT,
    CASUAL_PORTRAIT,
    LIFESTYLE_PHOTO,
    ACTION_SHOT,
    ARTISTIC_PORTRAIT,
    DOCUMENTARY_STYLE,
    SELFIE_STYLE,
    FORMAL_PORTRAIT,
    CANDID_PHOTO,
    GROUP_PHOTO
};

enum class PhotoBackground {
    PLAIN_COLOR,
    OFFICE_BACKGROUND,
    NATURE_OUTDOOR,
    URBAN_SETTING,
    HOME_INTERIOR,
    STUDIO_BACKGROUND,
    ABSTRACT_BACKGROUND,
    PROFESSIONAL_SETTING,
    CASUAL_SETTING,
    TRAVEL_LOCATION
};

enum class PhotoAge {
    TEENAGER,
    YOUNG_ADULT,
    ADULT,
    MIDDLE_AGED,
    SENIOR
};

enum class PhotoEthnicity {
    CAUCASIAN,
    AFRICAN_AMERICAN,
    ASIAN,
    HISPANIC,
    MIDDLE_EASTERN,
    INDIAN,
    NATIVE_AMERICAN,
    MIXED,
    OTHER
};

struct PhotoParameters {
    // Basic parameters
    std::string prompt;
    PhotoStyle style;
    PhotoBackground background;
    PhotoAge age_group;
    PhotoEthnicity ethnicity;
    
    // Physical attributes
    std::string gender;
    std::string hair_color;
    std::string hair_style;
    std::string eye_color;
    std::string skin_tone;
    std::string facial_features;
    std::string body_type;
    std::string height_description;
    
    // Clothing and appearance
    std::string clothing_style;
    std::string clothing_color;
    std::string accessories;
    std::string grooming_style;
    std::string expression;
    std::string pose;
    
    // Technical parameters
    std::string lighting;
    std::string camera_angle;
    std::string composition;
    std::string mood;
    std::string artistic_style;
    
    // Quality settings
    int width = 512;
    int height = 512;
    double quality = 0.9;
    int seed = -1; // -1 for random
    
    nlohmann::json to_json() const;
    void from_json(const nlohmann::json& j);
    std::string generate_prompt() const;
};

struct GeneratedPhoto {
    std::string id;
    std::string filename;
    std::string file_path;
    PhotoParameters parameters;
    std::string provider;
    std::string api_response;
    std::chrono::system_clock::time_point generated_at;
    double quality_score;
    std::map<std::string, std::string> metadata;
    
    nlohmann::json to_json() const;
    void from_json(const nlohmann::json& j);
};

class PhotoGenerator {
public:
    PhotoGenerator();
    ~PhotoGenerator();
    
    // Initialization
    bool initialize(const std::string& config_file = "config/photo_generator.yaml");
    void shutdown();
    
    // Basic photo generation
    std::shared_ptr<GeneratedPhoto> generate_profile_photo(const HumanPersona& persona);
    std::shared_ptr<GeneratedPhoto> generate_photo_with_parameters(const PhotoParameters& params);
    
    // Style-specific generation
    std::shared_ptr<GeneratedPhoto> generate_professional_headshot(const HumanPersona& persona);
    std::shared_ptr<GeneratedPhoto> generate_casual_portrait(const HumanPersona& persona);
    std::shared_ptr<GeneratedPhoto> generate_lifestyle_photo(const HumanPersona& persona);
    std::shared_ptr<GeneratedPhoto> generate_artistic_portrait(const HumanPersona& persona);
    
    // Platform-specific generation
    std::shared_ptr<GeneratedPhoto> generate_linkedin_photo(const HumanPersona& persona);
    std::shared_ptr<GeneratedPhoto> generate_facebook_photo(const HumanPersona& persona);
    std::shared_ptr<GeneratedPhoto> generate_instagram_photo(const HumanPersona& persona);
    std::shared_ptr<GeneratedPhoto> generate_twitter_photo(const HumanPersona& persona);
    std::shared_ptr<GeneratedPhoto> generate_gmail_avatar(const HumanPersona& persona);
    
    // Batch generation
    std::vector<std::shared_ptr<GeneratedPhoto>> generate_multiple_photos(
        const HumanPersona& persona,
        size_t count = 5);
    std::vector<std::shared_ptr<GeneratedPhoto>> generate_all_styles(const HumanPersona& persona);
    
    // Custom generation
    std::shared_ptr<GeneratedPhoto> generate_custom_photo(
        const HumanPersona& persona,
        const std::map<std::string, std::string>& custom_params);
    std::shared_ptr<GeneratedPhoto> generate_from_text_prompt(const std::string& prompt);
    
    // Photo management
    bool save_photo(const GeneratedPhoto& photo, const std::string& directory = "photos/");
    std::shared_ptr<GeneratedPhoto> load_photo(const std::string& filename);
    bool delete_photo(const std::string& photo_id);
    
    // Photo analysis
    double analyze_photo_quality(const std::string& image_path);
    std::map<std::string, double> analyze_photo_features(const std::string& image_path);
    bool validate_photo_authenticity(const GeneratedPhoto& photo, const HumanPersona& persona);
    
    // Provider management
    bool set_primary_provider(const std::string& provider_name);
    std::vector<std::string> get_available_providers();
    bool test_provider_connection(const std::string& provider_name);
    
    // Configuration
    void set_output_directory(const std::string& directory);
    void set_default_quality(double quality);
    void set_default_size(int width, int height);
    void set_api_key(const std::string& provider, const std::string& api_key);
    
    // Statistics
    std::map<std::string, size_t> get_generation_statistics();
    std::vector<std::shared_ptr<GeneratedPhoto>> get_recent_photos(size_t limit = 10);
    double get_average_quality_score();
    std::map<PhotoStyle, size_t> get_style_distribution();
    
    // Export/Import
    bool export_photo_library(const std::string& filename) const;
    bool import_photo_library(const std::string& filename);

private:
    // Provider interfaces
    class PhotoProvider {
    public:
        virtual ~PhotoProvider() = default;
        virtual bool initialize(const std::map<std::string, std::string>& config) = 0;
        virtual std::string generate_photo(const PhotoParameters& params) = 0;
        virtual bool test_connection() = 0;
        virtual std::string get_provider_name() const = 0;
        virtual double get_cost_per_generation() const = 0;
    };
    
    // Specific providers
    class StabilityAIProvider : public PhotoProvider {
    public:
        bool initialize(const std::map<std::string, std::string>& config) override;
        std::string generate_photo(const PhotoParameters& params) override;
        bool test_connection() override;
        std::string get_provider_name() const override { return "Stability AI"; }
        double get_cost_per_generation() const override { return 0.02; }
        
    private:
        std::string api_key_;
        std::string base_url_;
        std::string engine_id_;
        
        std::string build_stability_prompt(const PhotoParameters& params);
        std::string send_generation_request(const std::string& prompt, const PhotoParameters& params);
        std::string download_image(const std::string& generation_id);
    };
    
    class DALLEProvider : public PhotoProvider {
    public:
        bool initialize(const std::map<std::string, std::string>& config) override;
        std::string generate_photo(const PhotoParameters& params) override;
        bool test_connection() override;
        std::string get_provider_name() const override { return "DALL-E"; }
        double get_cost_per_generation() const override { return 0.04; }
        
    private:
        std::string api_key_;
        std::string base_url_;
        
        std::string build_dalle_prompt(const PhotoParameters& params);
        std::string send_generation_request(const std::string& prompt, const PhotoParameters& params);
    };
    
    class MidjourneyProvider : public PhotoProvider {
    public:
        bool initialize(const std::map<std::string, std::string>& config) override;
        std::string generate_photo(const PhotoParameters& params) override;
        bool test_connection() override;
        std::string get_provider_name() const override { return "Midjourney"; }
        double get_cost_per_generation() const override { return 0.03; }
        
    private:
        std::string api_key_;
        std::string base_url_;
        std::string discord_token_;
        
        std::string build_midjourney_prompt(const PhotoParameters& params);
        std::string send_discord_request(const std::string& prompt);
    };
    
    // Member variables
    std::map<std::string, std::unique_ptr<PhotoProvider>> providers_;
    std::string primary_provider_{"Stability AI"};
    std::string output_directory_{"photos/"};
    double default_quality_{0.9};
    int default_width_{512};
    int default_height_{512};
    
    // Photo storage
    std::map<std::string, std::shared_ptr<GeneratedPhoto>> photo_library_;
    std::map<std::string, size_t> generation_statistics_;
    
    // Random generation
    mutable std::mt19937 rng_;
    std::uniform_real_distribution<double> real_dist_{0.0, 1.0};
    
    // Thread safety
    mutable std::mutex generator_mutex_;
    
    // Private methods
    PhotoParameters extract_parameters_from_persona(const HumanPersona& persona, PhotoStyle style);
    PhotoParameters build_professional_parameters(const HumanPersona& persona);
    PhotoParameters build_casual_parameters(const HumanPersona& persona);
    PhotoParameters build_lifestyle_parameters(const HumanPersona& persona);
    PhotoParameters build_artistic_parameters(const HumanPersona& persona);
    
    // Parameter extraction helpers
    std::string determine_gender(const HumanPersona& persona);
    std::string determine_age_group(const HumanPersona& persona);
    std::string determine_ethnicity(const HumanPersona& persona);
    std::string determine_hair_color(const HumanPersona& persona);
    std::string determine_clothing_style(const HumanPersona& persona);
    std::string determine_background(const HumanPersona& persona, PhotoStyle style);
    
    // Prompt engineering
    std::string enhance_prompt_for_realism(const std::string& base_prompt);
    std::string add_quality_keywords(const std::string& prompt);
    std::string add_style_keywords(const std::string& prompt, PhotoStyle style);
    std::string add_negative_prompts(const std::string& prompt);
    
    // Image processing
    bool save_image_data(const std::string& image_data, const std::string& filename);
    std::string generate_unique_filename(const PhotoParameters& params);
    bool validate_image_format(const std::string& image_data);
    
    // Quality assessment
    double calculate_image_quality(const std::string& image_path);
    std::map<std::string, double> extract_image_features(const std::string& image_path);
    bool check_for_ai_artifacts(const std::string& image_path);
    
    // Provider management
    bool initialize_providers();
    void register_provider(std::unique_ptr<PhotoProvider> provider);
    PhotoProvider* get_current_provider();
    
    // Configuration
    bool load_configuration(const std::string& config_file);
    void set_default_configuration();
    
    // Utility methods
    std::string sanitize_filename(const std::string& filename);
    bool ensure_output_directory();
    void update_generation_statistics(const std::string& provider, bool success);
    
    // Logging
    void log_photo_info(const std::string& message) const;
    void log_photo_error(const std::string& message) const;
    void log_photo_debug(const std::string& message) const;
};

// Photo quality analyzer
class PhotoQualityAnalyzer {
public:
    PhotoQualityAnalyzer();
    ~PhotoQualityAnalyzer() = default;
    
    double analyze_quality(const std::string& image_path);
    std::map<std::string, double> get_quality_metrics(const std::string& image_path);
    std::vector<std::string> get_quality_issues(const std::string& image_path);
    
    // Specific quality aspects
    double analyze_resolution(const std::string& image_path);
    double analyze_composition(const std::string& image_path);
    double analyze_lighting(const std::string& image_path);
    double analyze_focus(const std::string& image_path);
    double analyze_color_quality(const std::string& image_path);
    double analyze_noise_level(const std::string& image_path);
    
    // Authenticity analysis
    double analyze_authenticity(const std::string& image_path);
    bool detect_ai_generation_artifacts(const std::string& image_path);
    bool detect_photo_manipulation(const std::string& image_path);

private:
    std::map<std::string, double> quality_weights_;
    
    // Image analysis helpers
    std::vector<int> get_image_resolution(const std::string& image_path);
    double calculate_sharpness(const std::string& image_path);
    double calculate_contrast(const std::string& image_path);
    double calculate_brightness(const std::string& image_path);
    double calculate_color_balance(const std::string& image_path);
    
    // AI detection
    bool check_unnatural_blur(const std::string& image_path);
    bool check_inconsistent_lighting(const std::string& image_path);
    bool check_perfect_symmetry(const std::string& image_path);
    bool check_unrealistic_texture(const std::string& image_path);
};

// Photo style recommender
class PhotoStyleRecommender {
public:
    PhotoStyleRecommender();
    ~PhotoStyleRecommender() = default;
    
    std::vector<PhotoStyle> recommend_styles_for_persona(const HumanPersona& persona);
    std::vector<PhotoStyle> recommend_styles_for_platform(const std::string& platform);
    std::vector<PhotoStyle> recommend_styles_for_purpose(const std::string& purpose);
    
    PhotoStyle get_best_style_match(const HumanPersona& persona, const std::string& platform);
    std::map<PhotoStyle, double> get_style_scores(const HumanPersona& persona);

private:
    std::map<std::string, std::vector<PhotoStyle>> platform_style_mapping_;
    std::map<PhotoStyle, std::vector<std::string>> style_characteristics_;
    
    void initialize_style_mappings();
    double calculate_style_compatibility(PhotoStyle style, const HumanPersona& persona);
    std::vector<std::string> get_persona_traits(const HumanPersona& persona);
};

} // namespace gmail_infinity
