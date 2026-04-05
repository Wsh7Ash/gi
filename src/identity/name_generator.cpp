#include "name_generator.h"
#include "app/app_logger.h"
#include <spdlog/spdlog.h>
#include <random>
#include <algorithm>
#include <stdexcept>

namespace gmail_infinity {

NameGenerator::NameGenerator() {
    std::random_device rd;
    rng_.seed(rd());
    initialize_name_pools();
}

void NameGenerator::seed(uint64_t s) { rng_.seed(s); }

void NameGenerator::initialize_name_pools() {
    // ── US/English names ──────────────────────────────────────────
    first_names_["en_US"] = {
        "James","John","Robert","Michael","William","David","Richard","Joseph","Thomas","Charles",
        "Christopher","Daniel","Matthew","Anthony","Mark","Donald","Steven","Paul","Andrew","Joshua",
        "Mary","Patricia","Jennifer","Linda","Barbara","Elizabeth","Susan","Jessica","Sarah","Karen",
        "Lisa","Nancy","Betty","Dorothy","Sandra","Ashley","Dorothy","Kimberly","Emily","Donna",
        "Amanda","Melissa","Deborah","Stephanie","Rebecca","Sharon","Laura","Cynthia","Kathleen","Amy"
    };
    last_names_["en_US"] = {
        "Smith","Johnson","Williams","Brown","Jones","Garcia","Miller","Davis","Rodriguez","Martinez",
        "Hernandez","Lopez","Gonzalez","Wilson","Anderson","Thomas","Taylor","Moore","Jackson","Martin",
        "Lee","Perez","Thompson","White","Harris","Sanchez","Clark","Ramirez","Lewis","Robinson",
        "Walker","Young","Allen","King","Wright","Scott","Torres","Nguyen","Hill","Flores",
        "Green","Adams","Nelson","Baker","Hall","Rivera","Campbell","Mitchell","Carter","Roberts"
    };

    // ── UK names ─────────────────────────────────────────────────
    first_names_["en_GB"] = {
        "Oliver","George","Harry","Jack","Noah","Charlie","Jacob","Alfie","Freddie","Oscar",
        "Amelia","Olivia","Isla","Emily","Ava","Ella","Isabella","Mia","Poppy","Sophia"
    };
    last_names_["en_GB"] = {
        "Smith","Jones","Williams","Taylor","Brown","Davies","Evans","Wilson","Thomas","Roberts",
        "Johnson","Lewis","Walker","Robinson","Wood","Thompson","White","Turner","Parker","Green"
    };

    // ── German names ──────────────────────────────────────────────
    first_names_["de_DE"] = {
        "Lukas","Maximilian","Paul","Jonas","Leon","Felix","Luca","Tim","Niklas","Finn",
        "Emma","Mia","Hannah","Marie","Lena","Sophia","Lea","Anna","Emilia","Clara"
    };
    last_names_["de_DE"] = {
        "Müller","Schmidt","Schneider","Fischer","Weber","Meyer","Wagner","Becker","Schulz","Hoffmann",
        "Schäfer","Koch","Bauer","Richter","Klein","Wolf","Schröder","Neumann","Schwarz","Zimmermann"
    };

    // ── French names ─────────────────────────────────────────────
    first_names_["fr_FR"] = {
        "Gabriel","Raphaël","Louis","Léo","Lucas","Hugo","Thomas","Théo","Tom","Baptiste",
        "Camille","Manon","Inès","Chloé","Emma","Léa","Zoé","Alice","Lucie","Clara"
    };
    last_names_["fr_FR"] = {
        "Martin","Bernard","Thomas","Petit","Robert","Richard","Durand","Dubois","Moreau","Laurent",
        "Simon","Michel","Lefebvre","Leroy","Roux","David","Bertrand","Morin","Fournier","Girard"
    };

    // ── Spanish names ────────────────────────────────────────────
    first_names_["es_ES"] = {
        "Alejandro","Daniel","Pablo","Diego","Adrián","Álvaro","Javier","Sergio","Carlos","David",
        "Lucía","María","Paula","Laura","Ana","Marta","Sara","Elena","Carmen","Isabel"
    };
    last_names_["es_ES"] = {
        "García","Martínez","López","Sánchez","González","Pérez","Torres","Rodríguez","Fernández","Moreno",
        "Jiménez","Ruiz","Hernández","Díaz","Álvarez","Muñoz","Romero","Alonso","Gutiérrez","Navarro"
    };

    // ── Brazilian Portuguese ──────────────────────────────────────
    first_names_["pt_BR"] = {
        "João","Pedro","Lucas","Mateus","Gabriel","Rafael","Felipe","Bruno","Eduardo","André",
        "Ana","Maria","Juliana","Amanda","Fernanda","Camila","Larissa","Beatriz","Carla","Daniela"
    };
    last_names_["pt_BR"] = {
        "Silva","Santos","Oliveira","Souza","Rodrigues","Ferreira","Alves","Pereira","Lima","Gomes",
        "Costa","Ribeiro","Martins","Carvalho","Almeida","Lopes","Sousa","Fernandes","Vieira","Barbosa"
    };

    // ── Russian names ────────────────────────────────────────────
    first_names_["ru_RU"] = {
        "Aleksandr","Dmitry","Sergei","Ivan","Andrei","Mikhail","Nikolai","Pavel","Artem","Kirill",
        "Anastasia","Natalia","Ekaterina","Olga","Irina","Marina","Elena","Tatyana","Anna","Yulia"
    };
    last_names_["ru_RU"] = {
        "Ivanov","Smirnov","Kuznetsov","Popov","Vasiliev","Petrov","Sokolov","Mikhailov","Novikov","Fedorov",
        "Morozov","Volkov","Alekseev","Lebedev","Semyonov","Egorov","Pavlov","Kozlov","Stepanov","Nikolaev"
    };

    // ── Indian names ─────────────────────────────────────────────
    first_names_["hi_IN"] = {
        "Aarav","Vivaan","Aditya","Vihaan","Arjun","Sai","Reyansh","Ayaan","Krishna","Ishaan",
        "Aadhya","Ananya","Diya","Saanvi","Aarohi","Pari","Anika","Navya","Angel","Priya"
    };
    last_names_["hi_IN"] = {
        "Sharma","Verma","Singh","Kumar","Patel","Gupta","Malhotra","Rao","Mehta","Nair",
        "Joshi","Pillai","Mishra","Chaudhary","Reddy","Shah","Bose","Pandey","Kapoor","Bhatt"
    };

    // Default locales
    supported_locales_ = {"en_US","en_GB","de_DE","fr_FR","es_ES","pt_BR","ru_RU","hi_IN"};
}

std::string NameGenerator::get_first_name(const std::string& locale) const {
    const auto& pool = get_pool(first_names_, locale.empty() ? "en_US" : locale);
    std::uniform_int_distribution<size_t> dis(0, pool.size() - 1);
    return pool[dis(rng_)];
}

std::string NameGenerator::get_last_name(const std::string& locale) const {
    const auto& pool = get_pool(last_names_, locale.empty() ? "en_US" : locale);
    std::uniform_int_distribution<size_t> dis(0, pool.size() - 1);
    return pool[dis(rng_)];
}

std::string NameGenerator::get_middle_name(const std::string& locale) const {
    // Middle names are drawn from first name pool
    return get_first_name(locale);
}

std::string NameGenerator::get_username(const std::string& first, const std::string& last) const {
    std::uniform_int_distribution<int> pattern_dis(0, 5);
    std::uniform_int_distribution<int> num_dis(10, 9999);
    int pattern = pattern_dis(rng_);
    int num     = num_dis(rng_);

    std::string f = first.empty() ? "user" : to_lower(first);
    std::string l = last.empty()  ? "name" : to_lower(last);

    switch (pattern) {
        case 0: return f + l + std::to_string(num);
        case 1: return f + "." + l;
        case 2: return f.substr(0,1) + l + std::to_string(num);
        case 3: return l + f.substr(0,1) + std::to_string(num);
        case 4: return f + "_" + l;
        default: return f + std::to_string(num);
    }
}

std::vector<std::string> NameGenerator::get_username_alternatives(
    const std::string& first, const std::string& last, int count) const {
    std::vector<std::string> alts;
    alts.reserve(count);
    for (int i = 0; i < count; ++i) alts.push_back(get_username(first, last));
    return alts;
}

std::string NameGenerator::get_random_locale() const {
    std::uniform_int_distribution<size_t> dis(0, supported_locales_.size() - 1);
    return supported_locales_[dis(rng_)];
}

const std::vector<std::string>& NameGenerator::get_pool(
    const std::unordered_map<std::string, std::vector<std::string>>& map,
    const std::string& locale) const {
    auto it = map.find(locale);
    if (it != map.end()) return it->second;
    // fallback
    auto def = map.find("en_US");
    if (def != map.end()) return def->second;
    static const std::vector<std::string> empty;
    return empty;
}

std::string NameGenerator::to_lower(std::string s) const {
    std::transform(s.begin(), s.end(), s.begin(), ::tolower);
    return s;
}

} // namespace gmail_infinity
