#pragma once

#include <string>
#include <vector>
#include <memory>
#include <map>
#include <random>
#include <mutex>

#include <nlohmann/json.hpp>

namespace gmail_infinity {

// Basic demographic enums
enum class Gender {
    MALE,
    FEMALE,
    NON_BINARY,
    OTHER
};

enum class AgeGroup {
    TEEN,
    YOUNG_ADULT,
    ADULT,
    MIDDLE_AGED,
    SENIOR
};

enum class EducationLevel {
    HIGH_SCHOOL,
    SOME_COLLEGE,
    BACHELOR,
    MASTER,
    PHD,
    OTHER
};

enum class EmploymentStatus {
    STUDENT,
    EMPLOYED,
    SELF_EMPLOYED,
    UNEMPLOYED,
    RETIRED,
    HOMEMAKER
};

enum class MaritalStatus {
    SINGLE,
    MARRIED,
    DIVORCED,
    WIDOWED,
    SEPARATED
};

// Geographic location
struct GeoLocation {
    std::string country;
    std::string state;
    std::string city;
    std::string postal_code;
    std::string timezone;
    double latitude;
    double longitude;
    std::string locale;
    
    nlohmann::json to_json() const;
    void from_json(const nlohmann::json& j);
};

// Name components
struct NameComponents {
    std::string first_name;
    std::string middle_name;
    std::string last_name;
    std::string full_name;
    std::string display_name;
    std::string initials;
    std::string nickname;
    
    nlohmann::json to_json() const;
    void from_json(const nlohmann::json& j);
};

// Date components
struct DateComponents {
    int year;
    int month;
    int day;
    std::string formatted_date;
    std::string iso_date;
    std::string age_group;
    
    nlohmann::json to_json() const;
    void from_json(const nlohmann::json& j);
    int calculate_age() const;
};

// Contact information
struct ContactInfo {
    std::string email;
    std::string phone;
    std::string secondary_email;
    std::string social_media;
    std::string website;
    std::string instant_messaging;
    
    nlohmann::json to_json() const;
    void from_json(const nlohmann::json& j);
};

// Education details
struct Education {
    EducationLevel level;
    std::string institution;
    std::string degree;
    std::string field_of_study;
    int graduation_year;
    std::string gpa;
    std::vector<std::string> achievements;
    
    nlohmann::json to_json() const;
    void from_json(const nlohmann::json& j);
};

// Employment details
struct Employment {
    EmploymentStatus status;
    std::string company;
    std::string position;
    std::string industry;
    std::string job_description;
    int start_year;
    int end_year; // -1 if current
    std::string salary_range;
    
    nlohmann::json to_json() const;
    void from_json(const nlohmann::json& j);
};

// Family information
struct Family {
    MaritalStatus marital_status;
    int children_count;
    std::vector<std::string> children_ages;
    std::string spouse_name;
    std::vector<std::string> siblings;
    std::string parent_status;
    
    nlohmann::json to_json() const;
    void from_json(const nlohmann::json& j);
};

// Lifestyle information
struct Lifestyle {
    std::vector<std::string> hobbies;
    std::vector<std::string> interests;
    std::vector<std::string> sports;
    std::vector<std::string> music_genres;
    std::vector<std::string> movie_genres;
    std::vector<std::string> book_genres;
    std::vector<std::string> cuisines;
    std::vector<std::string> travel_destinations;
    std::string diet_preference;
    std::string exercise_frequency;
    std::string smoking_status;
    std::string drinking_status;
    
    nlohmann::json to_json() const;
    void from_json(const nlohmann::json& j);
};

// Personality traits
struct Personality {
    // MBTI personality type
    std::string mbti_type;
    std::vector<std::string> mbti_traits;
    
    // Big Five personality traits
    double openness;
    double conscientiousness;
    double extraversion;
    double agreeableness;
    double neuroticism;
    
    // Other traits
    std::vector<std::string> positive_traits;
    std::vector<std::string> negative_traits;
    std::string temperament;
    std::string learning_style;
    
    nlohmann::json to_json() const;
    void from_json(const nlohmann::json& j);
};

// Beliefs and values
struct Beliefs {
    std::string religion;
    std::string political_leaning;
    std::vector<std::string> political_views;
    std::vector<std::string> core_values;
    std::vector<std::string> life_goals;
    std::string environmental_stance;
    std::string social_views;
    
    nlohmann::json to_json() const;
    void from_json(const nlohmann::json& j);
};

// Digital footprint
struct DigitalFootprint {
    std::vector<std::string> social_platforms;
    std::vector<std::string> online_communities;
    std::vector<std::string> forums;
    std::vector<std::string> apps;
    std::string device_type;
    std::string operating_system;
    std::vector<std::string> browsers_used;
    std::vector<std::string> search_engines;
    std::string online_behavior_pattern;
    double daily_screen_time_hours;
    
    nlohmann::json to_json() const;
    void from_json(const nlohmann::json& j);
};

// Main persona structure
struct HumanPersona {
    std::string id;
    NameComponents name;
    Gender gender;
    DateComponents birth_date;
    GeoLocation location;
    ContactInfo contact;
    Education education;
    Employment employment;
    Family family;
    Lifestyle lifestyle;
    Personality personality;
    Beliefs beliefs;
    DigitalFootprint digital_footprint;
    
    // Metadata
    std::chrono::system_clock::time_point created_at;
    std::chrono::system_clock::time_point last_updated;
    std::map<std::string, std::string> metadata;
    
    // Validation and consistency
    bool is_consistent() const;
    void ensure_consistency();
    
    // Serialization
    nlohmann::json to_json() const;
    void from_json(const nlohmann::json& j);
    
    // Utility methods
    int get_age() const;
    std::string get_age_group() const;
    std::string get_full_name() const;
    std::string get_email_username() const;
    std::string get_cultural_background() const;
};

class PersonaGenerator {
public:
    PersonaGenerator();
    ~PersonaGenerator() = default;
    
    // Persona generation
    std::shared_ptr<HumanPersona> generate_persona();
    std::shared_ptr<HumanPersona> generate_persona_with_constraints(
        const std::map<std::string, std::string>& constraints);
    std::shared_ptr<HumanPersona> generate_persona_for_country(const std::string& country);
    std::shared_ptr<HumanPersona> generate_persona_for_age_group(AgeGroup age_group);
    std::shared_ptr<HumanPersona> generate_persona_for_gender(Gender gender);
    
    // Persona management
    bool save_persona(const HumanPersona& persona, const std::string& filename = "");
    std::shared_ptr<HumanPersona> load_persona(const std::string& filename);
    std::vector<std::shared_ptr<HumanPersona>> load_all_personas(const std::string& directory = "personas/");
    
    // Persona validation
    bool validate_persona(const HumanPersona& persona);
    std::vector<std::string> get_validation_errors(const HumanPersona& persona);
    
    // Statistics and analysis
    std::map<std::string, size_t> get_persona_distribution() const;
    std::vector<std::shared_ptr<HumanPersona>> get_similar_personas(const HumanPersona& reference, size_t limit = 10);
    
    // Configuration
    void set_data_directory(const std::string& directory);
    void set_cultural_context(const std::string& context);
    void set_realism_level(double level); // 0.0 to 1.0
    
    // Batch operations
    std::vector<std::shared_ptr<HumanPersona>> generate_batch(size_t count);
    bool export_batch_to_csv(const std::vector<HumanPersona>& personas, const std::string& filename);
    bool import_batch_from_csv(const std::string& filename);

private:
    // Data sources
    std::string data_directory_{"data/personas/"};
    std::string cultural_context_{"western"};
    double realism_level_{0.8};
    
    // Random generation
    mutable std::mt19937 rng_;
    std::uniform_real_distribution<double> real_dist_{0.0, 1.0};
    
    // Data caches
    std::map<std::string, std::vector<std::string>> name_cache_;
    std::map<std::string, std::vector<std::string>> location_cache_;
    std::map<std::string, std::vector<std::string>> hobby_cache_;
    std::map<std::string, std::vector<std::string>> interest_cache_;
    
    // Thread safety
    mutable std::mutex generator_mutex_;
    
    // Private generation methods
    NameComponents generate_name(Gender gender, const std::string& country = "");
    DateComponents generate_birth_date(AgeGroup age_group);
    GeoLocation generate_location(const std::string& country = "");
    ContactInfo generate_contact(const NameComponents& name, const GeoLocation& location);
    Education generate_education(int age);
    Employment generate_employment(int age, EducationLevel education);
    Family generate_family(int age, MaritalStatus marital_status);
    Lifestyle generate_lifestyle(const Personality& personality, int age);
    Personality generate_personality();
    Beliefs generate_beliefs(const GeoLocation& location, int age);
    DigitalFootprint generate_digital_footprint(int age, const GeoLocation& location);
    
    // Constraint-based generation
    std::shared_ptr<HumanPersona> generate_with_constraints_internal(
        const std::map<std::string, std::string>& constraints);
    
    // Data loading methods
    bool load_name_data();
    bool load_location_data();
    bool load_lifestyle_data();
    bool load_personality_data();
    
    // Helper methods
    Gender select_gender(const std::map<std::string, std::string>& constraints = {});
    AgeGroup select_age_group(const std::map<std::string, std::string>& constraints = {});
    std::string select_country(const std::map<std::string, std::string>& constraints = {});
    std::string select_mbti_type(const Personality& base_personality = {});
    std::vector<std::string> select_hobbies(const Personality& personality, int count = 5);
    std::vector<std::string> select_interests(const Personality& personality, int count = 8);
    
    // Validation helpers
    bool validate_name_consistency(const NameComponents& name);
    bool validate_age_consistency(const DateComponents& birth_date, const Education& education);
    bool validate_location_consistency(const GeoLocation& location, const std::string& language);
    bool validate_personality_consistency(const Personality& personality);
    bool validate_digital_footprint_consistency(const DigitalFootprint& footprint, int age);
    
    // Utility methods
    std::string generate_persona_id() const;
    std::string format_name(const NameComponents& name);
    std::string format_date(int year, int month, int day);
    std::string generate_email_address(const NameComponents& name, const GeoLocation& location);
    std::string generate_phone_number(const GeoLocation& location);
    int calculate_age_from_birth_date(const DateComponents& birth_date);
    
    // Cultural adaptation
    void adapt_persona_to_culture(HumanPersona& persona, const std::string& culture);
    void adapt_names_to_culture(NameComponents& name, const std::string& culture);
    void adapt_lifestyle_to_culture(Lifestyle& lifestyle, const std::string& culture);
    void adapt_beliefs_to_culture(Beliefs& beliefs, const std::string& culture);
    
    // Logging
    void log_persona_info(const std::string& message) const;
    void log_persona_error(const std::string& message) const;
    void log_persona_debug(const std::string& message) const;
};

// Persona validator for advanced validation
class PersonaValidator {
public:
    PersonaValidator();
    ~PersonaValidator() = default;
    
    bool validate_persona(const HumanPersona& persona);
    std::vector<std::string> get_validation_errors(const HumanPersona& persona);
    double calculate_realism_score(const HumanPersona& persona);
    
    // Specific validation rules
    bool validate_demographic_consistency(const HumanPersona& persona);
    bool validate_geographic_consistency(const HumanPersona& persona);
    bool validate_temporal_consistency(const HumanPersona& persona);
    bool validate_cultural_consistency(const HumanPersona& persona);
    
    // Anomaly detection
    std::vector<std::string> detect_anomalies(const HumanPersona& persona);
    bool is_suspicious_persona(const HumanPersona& persona);

private:
    std::map<std::string, std::vector<std::string>> validation_rules_;
    double realism_threshold_{0.7};
    
    // Validation helpers
    bool is_valid_age_for_education(int age, EducationLevel education);
    bool is_valid_marriage_age(int age, MaritalStatus marital_status);
    bool is_valid_career_progression(const Employment& employment, int age);
    bool is_valid_digital_footprint(const DigitalFootprint& footprint, int age);
};

// Persona template system
class PersonaTemplate {
public:
    struct Template {
        std::string name;
        std::string description;
        std::map<std::string, std::string> constraints;
        std::map<std::string, double> weights;
        std::vector<std::string> required_fields;
        std::vector<std::string> optional_fields;
    };
    
    PersonaTemplate();
    ~PersonaTemplate() = default;
    
    bool load_templates(const std::string& filename = "data/persona_templates.json");
    bool save_templates(const std::string& filename = "data/persona_templates.json");
    
    std::shared_ptr<Template> get_template(const std::string& template_name);
    std::vector<std::string> get_available_templates();
    
    bool add_template(const Template& template_obj);
    bool remove_template(const std::string& template_name);
    
    std::shared_ptr<HumanPersona> generate_from_template(const std::string& template_name);

private:
    std::map<std::string, std::shared_ptr<Template>> templates_;
    mutable std::mutex template_mutex_;
    
    void load_default_templates();
    Template create_student_template();
    Template create_professional_template();
    Template create_retired_template();
    Template create_entrepreneur_template();
};

} // namespace gmail_infinity
