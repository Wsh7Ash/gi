#include "persona_generator.h"
#include "app/app_logger.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <chrono>
#include <iomanip>
#include <filesystem>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>

namespace gmail_infinity {

// GeoLocation implementation
nlohmann::json GeoLocation::to_json() const {
    nlohmann::json j;
    j["country"] = country;
    j["state"] = state;
    j["city"] = city;
    j["postal_code"] = postal_code;
    j["timezone"] = timezone;
    j["latitude"] = latitude;
    j["longitude"] = longitude;
    j["locale"] = locale;
    return j;
}

void GeoLocation::from_json(const nlohmann::json& j) {
    if (j.contains("country")) country = j["country"].get<std::string>();
    if (j.contains("state")) state = j["state"].get<std::string>();
    if (j.contains("city")) city = j["city"].get<std::string>();
    if (j.contains("postal_code")) postal_code = j["postal_code"].get<std::string>();
    if (j.contains("timezone")) timezone = j["timezone"].get<std::string>();
    if (j.contains("latitude")) latitude = j["latitude"].get<double>();
    if (j.contains("longitude")) longitude = j["longitude"].get<double>();
    if (j.contains("locale")) locale = j["locale"].get<std::string>();
}

// NameComponents implementation
nlohmann::json NameComponents::to_json() const {
    nlohmann::json j;
    j["first_name"] = first_name;
    j["middle_name"] = middle_name;
    j["last_name"] = last_name;
    j["full_name"] = full_name;
    j["display_name"] = display_name;
    j["initials"] = initials;
    j["nickname"] = nickname;
    return j;
}

void NameComponents::from_json(const nlohmann::json& j) {
    if (j.contains("first_name")) first_name = j["first_name"].get<std::string>();
    if (j.contains("middle_name")) middle_name = j["middle_name"].get<std::string>();
    if (j.contains("last_name")) last_name = j["last_name"].get<std::string>();
    if (j.contains("full_name")) full_name = j["full_name"].get<std::string>();
    if (j.contains("display_name")) display_name = j["display_name"].get<std::string>();
    if (j.contains("initials")) initials = j["initials"].get<std::string>();
    if (j.contains("nickname")) nickname = j["nickname"].get<std::string>();
}

// DateComponents implementation
nlohmann::json DateComponents::to_json() const {
    nlohmann::json j;
    j["year"] = year;
    j["month"] = month;
    j["day"] = day;
    j["formatted_date"] = formatted_date;
    j["iso_date"] = iso_date;
    j["age_group"] = age_group;
    return j;
}

void DateComponents::from_json(const nlohmann::json& j) {
    if (j.contains("year")) year = j["year"].get<int>();
    if (j.contains("month")) month = j["month"].get<int>();
    if (j.contains("day")) day = j["day"].get<int>();
    if (j.contains("formatted_date")) formatted_date = j["formatted_date"].get<std::string>();
    if (j.contains("iso_date")) iso_date = j["iso_date"].get<std::string>();
    if (j.contains("age_group")) age_group = j["age_group"].get<std::string>();
}

int DateComponents::calculate_age() const {
    auto now = std::chrono::system_clock::now();
    auto now_tm = std::chrono::system_clock::to_time_t(now);
    std::tm now_tm_struct = *std::localtime(&now_tm);
    
    int age = now_tm_struct.tm_year + 1900 - year;
    
    // Adjust if birthday hasn't occurred yet this year
    if (now_tm_struct.tm_mon + 1 < month || 
        (now_tm_struct.tm_mon + 1 == month && now_tm_struct.tm_mday < day)) {
        age--;
    }
    
    return age;
}

// HumanPersona implementation
nlohmann::json HumanPersona::to_json() const {
    nlohmann::json j;
    j["id"] = id;
    j["name"] = name.to_json();
    j["gender"] = static_cast<int>(gender);
    j["birth_date"] = birth_date.to_json();
    j["location"] = location.to_json();
    j["contact"] = contact.to_json();
    j["education"] = education.to_json();
    j["employment"] = employment.to_json();
    j["family"] = family.to_json();
    j["lifestyle"] = lifestyle.to_json();
    j["personality"] = personality.to_json();
    j["beliefs"] = beliefs.to_json();
    j["digital_footprint"] = digital_footprint.to_json();
    j["created_at"] = std::chrono::duration_cast<std::chrono::seconds>(
        created_at.time_since_epoch()).count();
    j["last_updated"] = std::chrono::duration_cast<std::chrono::seconds>(
        last_updated.time_since_epoch()).count();
    j["metadata"] = metadata;
    return j;
}

void HumanPersona::from_json(const nlohmann::json& j) {
    if (j.contains("id")) id = j["id"].get<std::string>();
    
    if (j.contains("name")) name.from_json(j["name"]);
    if (j.contains("gender")) gender = static_cast<Gender>(j["gender"].get<int>());
    if (j.contains("birth_date")) birth_date.from_json(j["birth_date"]);
    if (j.contains("location")) location.from_json(j["location"]);
    if (j.contains("contact")) contact.from_json(j["contact"]);
    if (j.contains("education")) education.from_json(j["education"]);
    if (j.contains("employment")) employment.from_json(j["employment"]);
    if (j.contains("family")) family.from_json(j["family"]);
    if (j.contains("lifestyle")) lifestyle.from_json(j["lifestyle"]);
    if (j.contains("personality")) personality.from_json(j["personality"]);
    if (j.contains("beliefs")) beliefs.from_json(j["beliefs"]);
    if (j.contains("digital_footprint")) digital_footprint.from_json(j["digital_footprint"]);
    
    if (j.contains("created_at")) {
        created_at = std::chrono::system_clock::from_time_t(j["created_at"].get<int64_t>());
    }
    if (j.contains("last_updated")) {
        last_updated = std::chrono::system_clock::from_time_t(j["last_updated"].get<int64_t>());
    }
    if (j.contains("metadata")) metadata = j["metadata"].get<std::map<std::string, std::string>>();
}

bool HumanPersona::is_consistent() const {
    int age = get_age();
    
    // Check age consistency
    if (age < 13) return false; // Too young for Gmail
    if (age > 120) return false; // Too old
    
    // Check education consistency
    if (age < 18 && education.level != EducationLevel::HIGH_SCHOOL) return false;
    if (age < 22 && education.level == EducationLevel::MASTER) return false;
    if (age < 25 && education.level == EducationLevel::PHD) return false;
    
    // Check employment consistency
    if (age < 16 && employment.status != EmploymentStatus::STUDENT) return false;
    if (age > 65 && employment.status == EmploymentStatus::STUDENT) return false;
    
    // Check family consistency
    if (age < 18 && family.marital_status != MaritalStatus::SINGLE) return false;
    if (family.children_count > 0 && age < 20) return false;
    
    return true;
}

void HumanPersona::ensure_consistency() {
    int age = get_age();
    
    // Adjust education based on age
    if (age < 18) {
        education.level = EducationLevel::HIGH_SCHOOL;
        education.graduation_year = birth_date.year + 18;
    } else if (age < 22 && education.level == EducationLevel::HIGH_SCHOOL) {
        education.level = EducationLevel::BACHELOR;
        education.graduation_year = birth_date.year + 22;
    }
    
    // Adjust employment based on age and education
    if (age < 16) {
        employment.status = EmploymentStatus::STUDENT;
    } else if (age > 65 && employment.status == EmploymentStatus::STUDENT) {
        employment.status = EmploymentStatus::RETIRED;
    }
    
    // Adjust family based on age
    if (age < 18) {
        family.marital_status = MaritalStatus::SINGLE;
        family.children_count = 0;
    }
    
    // Update timestamps
    last_updated = std::chrono::system_clock::now();
}

int HumanPersona::get_age() const {
    return birth_date.calculate_age();
}

std::string HumanPersona::get_age_group() const {
    int age = get_age();
    if (age < 18) return "teen";
    if (age < 25) return "young_adult";
    if (age < 40) return "adult";
    if (age < 65) return "middle_aged";
    return "senior";
}

std::string HumanPersona::get_full_name() const {
    return name.full_name;
}

std::string HumanPersona::get_email_username() const {
    std::string username = name.first_name;
    username += ".";
    username += name.last_name;
    
    // Add birth year for uniqueness
    username += std::to_string(birth_date.year);
    
    // Convert to lowercase
    std::transform(username.begin(), username.end(), username.begin(), ::tolower);
    
    return username;
}

std::string HumanPersona::get_cultural_background() const {
    return location.country + " (" + location.locale + ")";
}

// PersonaGenerator implementation
PersonaGenerator::PersonaGenerator() : rng_(std::random_device{}()) {
    load_name_data();
    load_location_data();
    load_lifestyle_data();
    load_personality_data();
}

std::shared_ptr<HumanPersona> PersonaGenerator::generate_persona() {
    std::lock_guard<std::mutex> lock(generator_mutex_);
    
    auto persona = std::make_shared<HumanPersona>();
    
    // Generate basic demographic information
    persona->id = generate_persona_id();
    persona->gender = select_gender();
    persona->birth_date = generate_birth_date(select_age_group());
    persona->location = generate_location(select_country());
    
    // Generate name
    persona->name = generate_name(persona->gender, persona->location.country);
    
    // Generate other components
    int age = persona->get_age();
    persona->contact = generate_contact(persona->name, persona->location);
    persona->education = generate_education(age);
    persona->employment = generate_employment(age, persona->education.level);
    persona->family = generate_family(age, persona->family.marital_status);
    persona->personality = generate_personality();
    persona->lifestyle = generate_lifestyle(persona->personality, age);
    persona->beliefs = generate_beliefs(persona->location, age);
    persona->digital_footprint = generate_digital_footprint(age, persona->location);
    
    // Set timestamps
    persona->created_at = std::chrono::system_clock::now();
    persona->last_updated = persona->created_at;
    
    // Ensure consistency
    persona->ensure_consistency();
    
    log_persona_info("Generated new persona: " + persona->get_full_name() + 
                    " (" + std::to_string(age) + " years old, " + persona->location.country + ")");
    
    return persona;
}

std::shared_ptr<HumanPersona> PersonaGenerator::generate_persona_with_constraints(
    const std::map<std::string, std::string>& constraints) {
    
    std::lock_guard<std::mutex> lock(generator_mutex_);
    return generate_with_constraints_internal(constraints);
}

std::shared_ptr<HumanPersona> PersonaGenerator::generate_persona_for_country(const std::string& country) {
    std::map<std::string, std::string> constraints = {{"country", country}};
    return generate_persona_with_constraints(constraints);
}

std::shared_ptr<HumanPersona> PersonaGenerator::generate_persona_for_age_group(AgeGroup age_group) {
    std::string age_str;
    switch (age_group) {
        case AgeGroup::TEEN: age_str = "teen"; break;
        case AgeGroup::YOUNG_ADULT: age_str = "young_adult"; break;
        case AgeGroup::ADULT: age_str = "adult"; break;
        case AgeGroup::MIDDLE_AGED: age_str = "middle_aged"; break;
        case AgeGroup::SENIOR: age_str = "senior"; break;
    }
    
    std::map<std::string, std::string> constraints = {{"age_group", age_str}};
    return generate_persona_with_constraints(constraints);
}

std::shared_ptr<HumanPersona> PersonaGenerator::generate_persona_for_gender(Gender gender) {
    std::string gender_str;
    switch (gender) {
        case Gender::MALE: gender_str = "male"; break;
        case Gender::FEMALE: gender_str = "female"; break;
        case Gender::NON_BINARY: gender_str = "non_binary"; break;
        case Gender::OTHER: gender_str = "other"; break;
    }
    
    std::map<std::string, std::string> constraints = {{"gender", gender_str}};
    return generate_persona_with_constraints(constraints);
}

NameComponents PersonaGenerator::generate_name(Gender gender, const std::string& country) {
    NameComponents name;
    
    std::string gender_key = (gender == Gender::MALE) ? "male" : "female";
    std::string country_key = country.empty() ? "us" : country;
    
    // Get first names
    auto first_names_it = name_cache_.find(gender_key + "_first_" + country_key);
    std::vector<std::string> first_names;
    if (first_names_it != name_cache_.end()) {
        first_names = first_names_it->second;
    } else {
        // Fallback to generic names
        first_names_it = name_cache_.find(gender_key + "_first");
        if (first_names_it != name_cache_.end()) {
            first_names = first_names_it->second;
        }
    }
    
    if (!first_names.empty()) {
        std::uniform_int_distribution<size_t> dist(0, first_names.size() - 1);
        name.first_name = first_names[dist(rng_)];
    } else {
        // Fallback names
        if (gender == Gender::MALE) {
            std::vector<std::string> fallback_names = {"John", "James", "Robert", "Michael", "William"};
            std::uniform_int_distribution<size_t> dist(0, fallback_names.size() - 1);
            name.first_name = fallback_names[dist(rng_)];
        } else {
            std::vector<std::string> fallback_names = {"Mary", "Patricia", "Jennifer", "Linda", "Elizabeth"};
            std::uniform_int_distribution<size_t> dist(0, fallback_names.size() - 1);
            name.first_name = fallback_names[dist(rng_)];
        }
    }
    
    // Get last names
    auto last_names_it = name_cache_.find("last_" + country_key);
    std::vector<std::string> last_names;
    if (last_names_it != name_cache_.end()) {
        last_names = last_names_it->second;
    } else {
        last_names_it = name_cache_.find("last");
        if (last_names_it != name_cache_.end()) {
            last_names = last_names_it->second;
        }
    }
    
    if (!last_names.empty()) {
        std::uniform_int_distribution<size_t> dist(0, last_names.size() - 1);
        name.last_name = last_names[dist(rng_)];
    } else {
        // Fallback last names
        std::vector<std::string> fallback_last_names = {"Smith", "Johnson", "Williams", "Brown", "Jones"};
        std::uniform_int_distribution<size_t> dist(0, fallback_last_names.size() - 1);
        name.last_name = fallback_last_names[dist(rng_)];
    }
    
    // Generate middle name (optional)
    if (real_dist_(rng_) < 0.7) { // 70% chance of having middle name
        if (!first_names.empty()) {
            std::uniform_int_distribution<size_t> dist(0, first_names.size() - 1);
            name.middle_name = first_names[dist(rng_)];
        }
    }
    
    // Generate nickname (optional)
    if (real_dist_(rng_) < 0.3) { // 30% chance of having nickname
        name.nickname = name.first_name.substr(0, 3) + "y";
    }
    
    // Build full name and display name
    name.full_name = name.first_name;
    if (!name.middle_name.empty()) {
        name.full_name += " " + name.middle_name;
    }
    name.full_name += " " + name.last_name;
    
    name.display_name = name.first_name + " " + name.last_name;
    
    // Generate initials
    name.initials = name.first_name.substr(0, 1);
    if (!name.middle_name.empty()) {
        name.initials += name.middle_name.substr(0, 1);
    }
    name.initials += name.last_name.substr(0, 1);
    
    return name;
}

DateComponents PersonaGenerator::generate_birth_date(AgeGroup age_group) {
    DateComponents date;
    
    auto now = std::chrono::system_clock::now();
    auto now_time_t = std::chrono::system_clock::to_time_t(now);
    std::tm now_tm = *std::localtime(&now_time_t);
    
    int min_age, max_age;
    switch (age_group) {
        case AgeGroup::TEEN: min_age = 13; max_age = 17; break;
        case AgeGroup::YOUNG_ADULT: min_age = 18; max_age = 24; break;
        case AgeGroup::ADULT: min_age = 25; max_age = 39; break;
        case AgeGroup::MIDDLE_AGED: min_age = 40; max_age = 64; break;
        case AgeGroup::SENIOR: min_age = 65; max_age = 85; break;
        default: min_age = 18; max_age = 65; break;
    }
    
    std::uniform_int_distribution<int> age_dist(min_age, max_age);
    std::uniform_int_distribution<int> month_dist(1, 12);
    
    int age = age_dist(rng_);
    date.year = now_tm.tm_year + 1900 - age;
    date.month = month_dist(rng_);
    
    // Generate valid day for the month
    int max_day;
    switch (date.month) {
        case 2: // February
            max_day = ((date.year % 4 == 0 && date.year % 100 != 0) || (date.year % 400 == 0)) ? 29 : 28;
            break;
        case 4: case 6: case 9: case 11: // 30-day months
            max_day = 30;
            break;
        default: // 31-day months
            max_day = 31;
            break;
    }
    
    std::uniform_int_distribution<int> day_dist(1, max_day);
    date.day = day_dist(rng_);
    
    // Set age group
    switch (age_group) {
        case AgeGroup::TEEN: date.age_group = "teen"; break;
        case AgeGroup::YOUNG_ADULT: date.age_group = "young_adult"; break;
        case AgeGroup::ADULT: date.age_group = "adult"; break;
        case AgeGroup::MIDDLE_AGED: date.age_group = "middle_aged"; break;
        case AgeGroup::SENIOR: date.age_group = "senior"; break;
    }
    
    // Format dates
    std::stringstream formatted, iso;
    formatted << std::setfill('0') << std::setw(2) << date.month << "/" 
              << std::setw(2) << date.day << "/" << date.year;
    date.formatted_date = formatted.str();
    
    iso << date.year << "-" << std::setfill('0') << std::setw(2) << date.month << "-" 
        << std::setw(2) << date.day;
    date.iso_date = iso.str();
    
    return date;
}

GeoLocation PersonaGenerator::generate_location(const std::string& country) {
    GeoLocation location;
    
    if (country.empty()) {
        // Select random country (weighted towards US)
        std::vector<std::string> countries = {"US", "CA", "GB", "AU", "DE", "FR", "IT", "ES", "JP", "KR"};
        std::vector<double> weights = {0.4, 0.1, 0.1, 0.05, 0.05, 0.05, 0.05, 0.05, 0.05, 0.05};
        std::discrete_distribution<size_t> dist(weights.begin(), weights.end());
        
        location.country = countries[dist(rng_)];
    } else {
        location.country = country;
    }
    
    // Generate location data based on country
    if (location.country == "US") {
        std::vector<std::string> states = {"California", "Texas", "New York", "Florida", "Illinois"};
        std::vector<std::string> cities = {"Los Angeles", "Houston", "New York", "Miami", "Chicago"};
        std::vector<std::string> zipcodes = {"90210", "77001", "10001", "33101", "60601"};
        std::vector<std::string> timezones = {"America/Los_Angeles", "America/Chicago", "America/New_York", "America/New_York", "America/Chicago"};
        
        std::uniform_int_distribution<size_t> dist(0, states.size() - 1);
        size_t index = dist(rng_);
        
        location.state = states[index];
        location.city = cities[index];
        location.postal_code = zipcodes[index];
        location.timezone = timezones[index];
        location.locale = "en_US";
        
        // Approximate coordinates
        std::uniform_real_distribution<double> lat_dist(25.0, 48.0);
        std::uniform_real_distribution<double> lng_dist(-125.0, -66.0);
        location.latitude = lat_dist(rng_);
        location.longitude = lng_dist(rng_);
        
    } else if (location.country == "GB") {
        location.state = "England";
        location.city = "London";
        location.postal_code = "SW1A 0AA";
        location.timezone = "Europe/London";
        location.locale = "en_GB";
        location.latitude = 51.5074;
        location.longitude = -0.1278;
        
    } else {
        // Default location for other countries
        location.state = "Unknown";
        location.city = "Unknown";
        location.postal_code = "00000";
        location.timezone = "UTC";
        location.locale = "en";
        location.latitude = 0.0;
        location.longitude = 0.0;
    }
    
    return location;
}

ContactInfo PersonaGenerator::generate_contact(const NameComponents& name, const GeoLocation& location) {
    ContactInfo contact;
    
    // Generate email address
    contact.email = generate_email_address(name, location);
    
    // Generate phone number
    contact.phone = generate_phone_number(location);
    
    // Generate secondary email (optional)
    if (real_dist_(rng_) < 0.3) { // 30% chance
        contact.secondary_email = contact.email + ".backup@gmail.com";
    }
    
    // Generate social media (optional)
    if (real_dist_(rng_) < 0.6) { // 60% chance
        contact.social_media = "@" + name.first_name + name.last_name;
    }
    
    return contact;
}

std::string PersonaGenerator::generate_email_address(const NameComponents& name, const GeoLocation& location) {
    std::string username = name.first_name;
    username += ".";
    username += name.last_name;
    
    // Add random number for uniqueness
    std::uniform_int_distribution<int> num_dist(100, 999);
    username += std::to_string(num_dist(rng_));
    
    // Convert to lowercase
    std::transform(username.begin(), username.end(), username.begin(), ::tolower);
    
    // Remove spaces and special characters
    username.erase(std::remove_if(username.begin(), username.end(), 
        [](char c) { return !std::isalnum(c); }), username.end());
    
    return username + "@gmail.com";
}

std::string PersonaGenerator::generate_phone_number(const GeoLocation& location) {
    std::string phone;
    
    if (location.country == "US") {
        std::uniform_int_distribution<int> area_dist(200, 999);
        std::uniform_int_distribution<int> exchange_dist(200, 999);
        std::uniform_int_distribution<int> number_dist(1000, 9999);
        
        phone = "+1";
        phone += std::to_string(area_dist(rng_));
        phone += std::to_string(exchange_dist(rng_));
        phone += std::to_string(number_dist(rng_));
        
    } else if (location.country == "GB") {
        std::uniform_int_distribution<int> number_dist(10000000, 99999999);
        phone = "+44" + std::to_string(number_dist(rng_));
        
    } else {
        // Generic international format
        std::uniform_int_distribution<int> number_dist(1000000000, 9999999999);
        phone = "+" + std::to_string(number_dist(rng_));
    }
    
    return phone;
}

bool PersonaGenerator::load_name_data() {
    // Initialize with basic name data
    name_cache_["male_first"] = {"John", "James", "Robert", "Michael", "William", "David", "Richard", "Thomas", "Charles", "Christopher"};
    name_cache_["female_first"] = {"Mary", "Patricia", "Jennifer", "Linda", "Elizabeth", "Barbara", "Susan", "Jessica", "Sarah", "Karen"};
    name_cache_["last"] = {"Smith", "Johnson", "Williams", "Brown", "Jones", "Garcia", "Miller", "Davis", "Rodriguez", "Martinez"};
    
    return true;
}

bool PersonaGenerator::load_location_data() {
    // Location data would typically be loaded from files
    // For now, using hardcoded data in generate_location()
    return true;
}

bool PersonaGenerator::load_lifestyle_data() {
    // Initialize with basic lifestyle data
    hobby_cache_["general"] = {"Reading", "Music", "Sports", "Travel", "Cooking", "Photography", "Gardening", "Gaming", "Art", "Writing"};
    interest_cache_["general"] = {"Technology", "Science", "History", "Politics", "Fashion", "Movies", "Health", "Finance", "Education", "Environment"};
    
    return true;
}

bool PersonaGenerator::load_personality_data() {
    // Personality data would typically be loaded from files
    return true;
}

Gender PersonaGenerator::select_gender(const std::map<std::string, std::string>& constraints) {
    auto it = constraints.find("gender");
    if (it != constraints.end()) {
        std::string gender_str = it->second;
        if (gender_str == "male") return Gender::MALE;
        if (gender_str == "female") return Gender::FEMALE;
        if (gender_str == "non_binary") return Gender::NON_BINARY;
        if (gender_str == "other") return Gender::OTHER;
    }
    
    // Default: 50/50 male/female distribution
    std::uniform_int_distribution<int> dist(0, 1);
    return dist(rng_) == 0 ? Gender::MALE : Gender::FEMALE;
}

AgeGroup PersonaGenerator::select_age_group(const std::map<std::string, std::string>& constraints) {
    auto it = constraints.find("age_group");
    if (it != constraints.end()) {
        std::string age_str = it->second;
        if (age_str == "teen") return AgeGroup::TEEN;
        if (age_str == "young_adult") return AgeGroup::YOUNG_ADULT;
        if (age_str == "adult") return AgeGroup::ADULT;
        if (age_str == "middle_aged") return AgeGroup::MIDDLE_AGED;
        if (age_str == "senior") return AgeGroup::SENIOR;
    }
    
    // Default: weighted distribution towards adults
    std::vector<double> weights = {0.05, 0.15, 0.40, 0.30, 0.10}; // teen, young_adult, adult, middle_aged, senior
    std::discrete_distribution<size_t> dist(weights.begin(), weights.end());
    
    size_t index = dist(rng_);
    switch (index) {
        case 0: return AgeGroup::TEEN;
        case 1: return AgeGroup::YOUNG_ADULT;
        case 2: return AgeGroup::ADULT;
        case 3: return AgeGroup::MIDDLE_AGED;
        case 4: return AgeGroup::SENIOR;
        default: return AgeGroup::ADULT;
    }
}

std::string PersonaGenerator::generate_persona_id() const {
    boost::uuids::uuid uuid = boost::uuids::random_generator()();
    std::stringstream ss;
    ss << uuid;
    return "persona_" + ss.str();
}

void PersonaGenerator::log_persona_info(const std::string& message) const {
    LOG_INFO("PersonaGenerator: {}", message);
}

void PersonaGenerator::log_persona_error(const std::string& message) const {
    LOG_ERROR("PersonaGenerator: {}", message);
}

void PersonaGenerator::log_persona_debug(const std::string& message) const {
    LOG_DEBUG("PersonaGenerator: {}", message);
}

} // namespace gmail_infinity
