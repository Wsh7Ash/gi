#include "bio_generator.h"
#include "app/app_logger.h"
#include <spdlog/spdlog.h>
#include <random>
#include <sstream>
#include <regex>
#include <stdexcept>

namespace gmail_infinity {

BioGenerator::BioGenerator() {
    std::random_device rd;
    rng_.seed(rd());
    initialize_templates();
}

void BioGenerator::seed(uint64_t s) { rng_.seed(s); }

void BioGenerator::initialize_templates() {
    // Professional bio templates with placeholders
    professional_templates_ = {
        "{first_name} is a {job_title} at {company} with {years} years of experience in {industry}. "
        "Based in {city}, {country}. Passionate about {interest}.",

        "Experienced {job_title} specializing in {skill}. "
        "Currently working at {company} in {city}. Loves {hobby} in spare time.",

        "{first_name} {last_name} | {job_title} | {city}, {country} | {years}+ years in {industry}",

        "Hello! I'm {first_name}, a {job_title} with a background in {industry}. "
        "I enjoy {hobby} and am always learning about {interest}.",

        "{job_title} and {hobby} enthusiast from {city}. "
        "Working at {company}. Let's connect!"
    };

    casual_templates_ = {
        "Just a {age}-year-old {noun} who loves {hobby} and {interest}. "
        "Living my best life in {city}!",

        "{first_name} from {city}. Coffee addict ☕️. {hobby} fan. {interest} nerd.",

        "Living in {city} | {hobby} | {interest} | {noun}",

        "Simple person with big dreams. {hobby} is life. {city} based.",

        "📍 {city} | {hobby} lover | obsessed with {interest} | {age} years young"
    };

    recovery_bio_fragments_ = {
        "Member since {year}",
        "Verified account",
        "Personal account",
        "Private account - {city}",
    };

    // Vocabulary pools
    hobbies_ = {
        "hiking","cooking","photography","gaming","reading","traveling","yoga","cycling",
        "painting","music","gardening","running","swimming","chess","dancing","writing",
        "camping","fishing","baking","knitting","surfing","rock climbing","podcasting"
    };
    interests_ = {
        "technology","sustainability","mental health","finance","AI","space exploration",
        "history","philosophy","nutrition","environmental science","crypto","design",
        "entrepreneurship","psychology","data science","renewable energy","literature"
    };
    jobs_ = {
        "Software Engineer","Product Manager","Data Scientist","UX Designer","Marketing Manager",
        "Financial Analyst","Project Manager","Business Analyst","Sales Representative",
        "Graphic Designer","Content Creator","Operations Manager","HR Manager",
        "DevOps Engineer","Machine Learning Engineer","Cloud Architect","Consultant"
    };
    companies_ = {
        "Tech Startup","Digital Agency","Consulting Group","Innovation Labs","Creative Studio",
        "Solutions Inc.","Global Services","Analytics Corp","Development Hub","Media Group"
    };
    industries_ = {
        "technology","finance","healthcare","education","e-commerce","media","consulting",
        "manufacturing","hospitality","retail","logistics","real estate","energy"
    };
    skills_ = {
        "Python","JavaScript","data analysis","project management","digital marketing",
        "cloud computing","machine learning","UX research","content strategy","SEO",
        "financial modeling","supply chain","agile methodology","graphic design"
    };
    nouns_ = {
        "student","professional","freelancer","entrepreneur","dreamer","traveler",
        "creator","developer","artist","writer","thinker","builder","explorer"
    };
}

std::string BioGenerator::generate_bio(const BioContext& ctx) const {
    const auto& templates = (ctx.style == "casual") ? casual_templates_ : professional_templates_;
    std::uniform_int_distribution<size_t> tmpl_dis(0, templates.size() - 1);
    std::string bio = templates[tmpl_dis(rng_)];
    return fill_template(bio, ctx);
}

std::string BioGenerator::generate_professional_bio(const BioContext& ctx) const {
    BioContext c = ctx; c.style = "professional";
    return generate_bio(c);
}

std::string BioGenerator::generate_casual_bio(const BioContext& ctx) const {
    BioContext c = ctx; c.style = "casual";
    return generate_bio(c);
}

std::string BioGenerator::generate_short_bio(int max_chars) const {
    BioContext ctx;
    ctx.first_name = "User";
    ctx.last_name  = "Name";
    ctx.city       = pick({"New York","London","Berlin","Paris","Sydney"});
    ctx.country    = "US";
    ctx.hobby      = pick(hobbies_);
    ctx.interest   = pick(interests_);
    ctx.style      = "casual";
    auto bio = generate_bio(ctx);
    if (static_cast<int>(bio.size()) > max_chars) bio = bio.substr(0, max_chars - 3) + "...";
    return bio;
}

std::string BioGenerator::fill_template(const std::string& tmpl, const BioContext& ctx) const {
    auto replace = [](std::string s, const std::string& from, const std::string& to) {
        size_t pos = 0;
        while ((pos = s.find(from, pos)) != std::string::npos) {
            s.replace(pos, from.size(), to);
            pos += to.size();
        }
        return s;
    };

    std::string result = tmpl;

    // Contextual values
    result = replace(result, "{first_name}", ctx.first_name);
    result = replace(result, "{last_name}",  ctx.last_name);
    result = replace(result, "{city}",       ctx.city.empty()    ? pick({"New York","London","Berlin"}) : ctx.city);
    result = replace(result, "{country}",    ctx.country.empty() ? "US" : ctx.country);
    result = replace(result, "{age}",        ctx.age > 0 ? std::to_string(ctx.age) : std::to_string(rand_int(22, 45)));
    result = replace(result, "{year}",       std::to_string(rand_int(2018, 2024)));
    result = replace(result, "{years}",      std::to_string(rand_int(2, 15)));

    // Random from pools
    result = replace(result, "{hobby}",      ctx.hobby.empty()    ? pick(hobbies_)    : ctx.hobby);
    result = replace(result, "{interest}",   ctx.interest.empty() ? pick(interests_)  : ctx.interest);
    result = replace(result, "{job_title}",  ctx.job.empty()      ? pick(jobs_)       : ctx.job);
    result = replace(result, "{company}",    pick(companies_));
    result = replace(result, "{industry}",   pick(industries_));
    result = replace(result, "{skill}",      pick(skills_));
    result = replace(result, "{noun}",       pick(nouns_));

    return result;
}

std::string BioGenerator::pick(const std::vector<std::string>& pool) const {
    if (pool.empty()) return "";
    std::uniform_int_distribution<size_t> dis(0, pool.size() - 1);
    return pool[dis(rng_)];
}

int BioGenerator::rand_int(int lo, int hi) const {
    std::uniform_int_distribution<int> dis(lo, hi);
    return dis(rng_);
}

} // namespace gmail_infinity
