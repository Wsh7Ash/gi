#include "secure_vault.h"
#include "app_logger.h"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <random>
#include <filesystem>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/sha.h>
#include <openssl/aes.h>

namespace gmail_infinity {

// Credential implementation
nlohmann::json Credential::to_json() const {
    nlohmann::json j;
    j["id"] = id;
    j["email"] = email;
    j["password"] = password;
    j["recovery_email"] = recovery_email;
    j["phone"] = phone;
    j["proxy_id"] = proxy_id;
    j["persona_id"] = persona_id;
    j["created_at"] = std::chrono::duration_cast<std::chrono::seconds>(
        created_at.time_since_epoch()).count();
    j["last_used"] = std::chrono::duration_cast<std::chrono::seconds>(
        last_used.time_since_epoch()).count();
    j["is_active"] = is_active;
    j["metadata"] = metadata;
    return j;
}

void Credential::from_json(const nlohmann::json& j) {
    if (j.contains("id")) id = j["id"].get<std::string>();
    if (j.contains("email")) email = j["email"].get<std::string>();
    if (j.contains("password")) password = j["password"].get<std::string>();
    if (j.contains("recovery_email")) recovery_email = j["recovery_email"].get<std::string>();
    if (j.contains("phone")) phone = j["phone"].get<std::string>();
    if (j.contains("proxy_id")) proxy_id = j["proxy_id"].get<std::string>();
    if (j.contains("persona_id")) persona_id = j["persona_id"].get<std::string>();
    if (j.contains("created_at")) {
        created_at = std::chrono::system_clock::from_time_t(j["created_at"].get<int64_t>());
    }
    if (j.contains("last_used")) {
        last_used = std::chrono::system_clock::from_time_t(j["last_used"].get<int64_t>());
    }
    if (j.contains("is_active")) is_active = j["is_active"].get<bool>();
    if (j.contains("metadata")) metadata = j["metadata"].get<std::map<std::string, std::string>>();
}

// SecureVault implementation
SecureVault::SecureVault() {
    // Initialize OpenSSL
    OpenSSL_add_all_algorithms();
    ERR_load_crypto_strings();
}

SecureVault::~SecureVault() {
    // Cleanup OpenSSL
    EVP_cleanup();
    ERR_free_strings();
}

bool SecureVault::load_or_create() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Try to load existing vault
    if (std::filesystem::exists("credentials/vault.key") && 
        std::filesystem::exists("credentials/vault.enc")) {
        return load("credentials/vault.key", "credentials/vault.enc");
    }
    
    // Create new vault
    log_info("Creating new secure vault...");
    
    // Create credentials directory
    std::filesystem::create_directories("credentials");
    
    // Generate new key
    if (!generate_key()) {
        log_error("Failed to generate encryption key");
        return false;
    }
    
    // Save empty vault
    return save("credentials/vault.key", "credentials/vault.enc");
}

bool SecureVault::load(const std::string& key_file, const std::string& data_file) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    try {
        // Load encryption key
        encryption_key_ = load_key_file(key_file);
        if (encryption_key_.empty()) {
            log_error("Failed to load encryption key from " + key_file);
            return false;
        }
        
        // Load and decrypt data
        std::ifstream file(data_file, std::ios::binary);
        if (!file.is_open()) {
            log_error("Failed to open vault data file: " + data_file);
            return false;
        }
        
        std::string encrypted_data((std::istreambuf_iterator<char>(file)),
                                   std::istreambuf_iterator<char>());
        file.close();
        
        std::string decrypted_data = decrypt_data(encrypted_data);
        if (decrypted_data.empty()) {
            log_error("Failed to decrypt vault data");
            return false;
        }
        
        // Parse JSON data
        nlohmann::json json_data = nlohmann::json::parse(decrypted_data);
        
        // Load credentials
        credentials_.clear();
        if (json_data.contains("credentials")) {
            for (const auto& cred_json : json_data["credentials"]) {
                Credential cred;
                cred.from_json(cred_json);
                credentials_[cred.id] = cred;
            }
        }
        
        // Load salt if present
        if (json_data.contains("salt")) {
            salt_ = json_data["salt"].get<std::string>();
        }
        
        log_info("Vault loaded successfully with " + std::to_string(credentials_.size()) + " credentials");
        return true;
        
    } catch (const std::exception& e) {
        log_error("Failed to load vault: " + std::string(e.what()));
        return false;
    }
}

bool SecureVault::save(const std::string& key_file, const std::string& data_file) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    try {
        // Save encryption key
        if (!save_key_file(key_file, encryption_key_)) {
            log_error("Failed to save encryption key to " + key_file);
            return false;
        }
        
        // Create JSON data
        nlohmann::json json_data;
        json_data["credentials"] = nlohmann::json::array();
        
        for (const auto& [id, cred] : credentials_) {
            json_data["credentials"].push_back(cred.to_json());
        }
        
        json_data["salt"] = salt_;
        json_data["version"] = "1.0";
        json_data["timestamp"] = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        
        // Encrypt data
        std::string plain_data = json_data.dump();
        std::string encrypted_data = encrypt_data(plain_data);
        
        // Save encrypted data
        std::ofstream file(data_file, std::ios::binary);
        if (!file.is_open()) {
            log_error("Failed to open vault data file for writing: " + data_file);
            return false;
        }
        
        file.write(encrypted_data.data(), encrypted_data.size());
        file.close();
        
        log_info("Vault saved successfully with " + std::to_string(credentials_.size()) + " credentials");
        return true;
        
    } catch (const std::exception& e) {
        log_error("Failed to save vault: " + std::string(e.what()));
        return false;
    }
}

bool SecureVault::generate_key() {
    try {
        // Generate random key
        std::vector<unsigned char> key(KEY_SIZE);
        if (RAND_bytes(key.data(), KEY_SIZE) != 1) {
            log_error("Failed to generate random key");
            return false;
        }
        
        // Convert to hex string
        std::stringstream ss;
        for (unsigned char byte : key) {
            ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(byte);
        }
        encryption_key_ = ss.str();
        
        // Generate salt
        salt_ = generate_salt();
        
        log_info("Generated new encryption key and salt");
        return true;
        
    } catch (const std::exception& e) {
        log_error("Failed to generate key: " + std::string(e.what()));
        return false;
    }
}

bool SecureVault::set_key(const std::string& key) {
    if (key.size() != KEY_SIZE * 2) { // Hex string should be 2x key size
        log_error("Invalid key size");
        return false;
    }
    
    encryption_key_ = key;
    return true;
}

std::string SecureVault::get_key() const {
    return encryption_key_;
}

std::string SecureVault::save_credential(const Credential& credential) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!validate_credential(credential)) {
        log_error("Invalid credential data");
        return "";
    }
    
    Credential cred = credential;
    if (cred.id.empty()) {
        cred.id = generate_credential_id();
    }
    
    cred.created_at = std::chrono::system_clock::now();
    cred.last_used = cred.created_at;
    
    credentials_[cred.id] = cred;
    
    log_info("Saved credential: " + cred.id);
    return cred.id;
}

bool SecureVault::update_credential(const std::string& id, const Credential& credential) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (credentials_.find(id) == credentials_.end()) {
        log_error("Credential not found: " + id);
        return false;
    }
    
    if (!validate_credential(credential)) {
        log_error("Invalid credential data");
        return false;
    }
    
    Credential cred = credential;
    cred.id = id;
    cred.last_used = std::chrono::system_clock::now();
    
    credentials_[id] = cred;
    
    log_info("Updated credential: " + id);
    return true;
}

bool SecureVault::delete_credential(const std::string& id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (credentials_.erase(id) > 0) {
        log_info("Deleted credential: " + id);
        return true;
    }
    
    log_error("Credential not found for deletion: " + id);
    return false;
}

Credential SecureVault::get_credential(const std::string& id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = credentials_.find(id);
    if (it != credentials_.end()) {
        return it->second;
    }
    
    return Credential{}; // Return empty credential
}

std::vector<Credential> SecureVault::get_all_credentials() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<Credential> result;
    result.reserve(credentials_.size());
    
    for (const auto& [id, cred] : credentials_) {
        result.push_back(cred);
    }
    
    return result;
}

std::vector<Credential> SecureVault::get_active_credentials() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<Credential> result;
    
    for (const auto& [id, cred] : credentials_) {
        if (cred.is_active) {
            result.push_back(cred);
        }
    }
    
    return result;
}

std::vector<Credential> SecureVault::get_credentials_by_proxy(const std::string& proxy_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<Credential> result;
    
    for (const auto& [id, cred] : credentials_) {
        if (cred.proxy_id == proxy_id) {
            result.push_back(cred);
        }
    }
    
    return result;
}

size_t SecureVault::get_total_credentials() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return credentials_.size();
}

size_t SecureVault::get_active_credentials_count() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    size_t count = 0;
    for (const auto& [id, cred] : credentials_) {
        if (cred.is_active) count++;
    }
    
    return count;
}

size_t SecureVault::get_credentials_created_today() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto now = std::chrono::system_clock::now();
    auto today = std::chrono::floor<std::chrono::days>(now);
    
    size_t count = 0;
    for (const auto& [id, cred] : credentials_) {
        auto cred_day = std::chrono::floor<std::chrono::days>(cred.created_at);
        if (cred_day == today) count++;
    }
    
    return count;
}

bool SecureVault::validate_credential(const Credential& credential) const {
    // Basic validation
    if (credential.email.empty() || credential.password.empty()) {
        return false;
    }
    
    // Email format validation (basic)
    if (credential.email.find('@') == std::string::npos) {
        return false;
    }
    
    // Password length validation
    if (credential.password.size() < 8) {
        return false;
    }
    
    return true;
}

std::string SecureVault::generate_credential_id() const {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(100000, 999999);
    
    return "cred_" + std::to_string(dis(gen));
}

std::string SecureVault::encrypt_data(const std::string& data) const {
    // This is a simplified encryption implementation
    // In production, you'd want more robust error handling and validation
    
    try {
        // Convert hex key to binary
        std::vector<unsigned char> key(KEY_SIZE);
        for (size_t i = 0; i < KEY_SIZE; ++i) {
            std::string byte_str = encryption_key_.substr(i * 2, 2);
            key[i] = static_cast<unsigned char>(std::stoul(byte_str, nullptr, 16));
        }
        
        // Generate IV
        std::vector<unsigned char> iv(IV_SIZE);
        RAND_bytes(iv.data(), IV_SIZE);
        
        // Setup encryption context
        EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
        if (!ctx) {
            log_error("Failed to create encryption context");
            return "";
        }
        
        if (EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, key.data(), iv.data()) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            log_error("Failed to initialize encryption");
            return "";
        }
        
        // Encrypt data
        std::vector<unsigned char> ciphertext(data.size());
        int len;
        if (EVP_EncryptUpdate(ctx, ciphertext.data(), &len, 
                             reinterpret_cast<const unsigned char*>(data.data()), 
                             data.size()) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            log_error("Failed to encrypt data");
            return "";
        }
        
        int ciphertext_len = len;
        
        // Finalize encryption
        if (EVP_EncryptFinal_ex(ctx, ciphertext.data() + len, &len) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            log_error("Failed to finalize encryption");
            return "";
        }
        
        ciphertext_len += len;
        
        // Get authentication tag
        std::vector<unsigned char> tag(TAG_SIZE);
        if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, TAG_SIZE, tag.data()) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            log_error("Failed to get authentication tag");
            return "";
        }
        
        EVP_CIPHER_CTX_free(ctx);
        
        // Combine IV + ciphertext + tag
        std::string result;
        result.append(reinterpret_cast<char*>(iv.data()), iv.size());
        result.append(reinterpret_cast<char*>(ciphertext.data()), ciphertext_len);
        result.append(reinterpret_cast<char*>(tag.data()), tag.size());
        
        return result;
        
    } catch (const std::exception& e) {
        log_error("Encryption error: " + std::string(e.what()));
        return "";
    }
}

std::string SecureVault::decrypt_data(const std::string& encrypted_data) const {
    // This is a simplified decryption implementation
    // In production, you'd want more robust error handling and validation
    
    try {
        if (encrypted_data.size() < IV_SIZE + TAG_SIZE) {
            log_error("Invalid encrypted data size");
            return "";
        }
        
        // Convert hex key to binary
        std::vector<unsigned char> key(KEY_SIZE);
        for (size_t i = 0; i < KEY_SIZE; ++i) {
            std::string byte_str = encryption_key_.substr(i * 2, 2);
            key[i] = static_cast<unsigned char>(std::stoul(byte_str, nullptr, 16));
        }
        
        // Extract IV, ciphertext, and tag
        std::vector<unsigned char> iv(encrypted_data.begin(), 
                                     encrypted_data.begin() + IV_SIZE);
        std::vector<unsigned char> ciphertext(encrypted_data.begin() + IV_SIZE,
                                            encrypted_data.end() - TAG_SIZE);
        std::vector<unsigned char> tag(encrypted_data.end() - TAG_SIZE,
                                      encrypted_data.end());
        
        // Setup decryption context
        EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
        if (!ctx) {
            log_error("Failed to create decryption context");
            return "";
        }
        
        if (EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, key.data(), iv.data()) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            log_error("Failed to initialize decryption");
            return "";
        }
        
        // Decrypt data
        std::vector<unsigned char> plaintext(ciphertext.size());
        int len;
        if (EVP_DecryptUpdate(ctx, plaintext.data(), &len,
                             ciphertext.data(), ciphertext.size()) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            log_error("Failed to decrypt data");
            return "";
        }
        
        int plaintext_len = len;
        
        // Set expected tag value
        if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, TAG_SIZE, tag.data()) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            log_error("Failed to set authentication tag");
            return "";
        }
        
        // Finalize decryption (this will verify the tag)
        if (EVP_DecryptFinal_ex(ctx, plaintext.data() + len, &len) <= 0) {
            EVP_CIPHER_CTX_free(ctx);
            log_error("Failed to finalize decryption (authentication failed)");
            return "";
        }
        
        plaintext_len += len;
        EVP_CIPHER_CTX_free(ctx);
        
        return std::string(plaintext.begin(), plaintext.begin() + plaintext_len);
        
    } catch (const std::exception& e) {
        log_error("Decryption error: " + std::string(e.what()));
        return "";
    }
}

std::string SecureVault::generate_salt() const {
    std::vector<unsigned char> salt(SALT_SIZE);
    RAND_bytes(salt.data(), SALT_SIZE);
    
    std::stringstream ss;
    for (unsigned char byte : salt) {
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(byte);
    }
    
    return ss.str();
}

bool SecureVault::save_key_file(const std::string& filename, const std::string& key) const {
    try {
        std::ofstream file(filename, std::ios::binary);
        if (!file.is_open()) {
            return false;
        }
        
        file.write(key.data(), key.size());
        file.close();
        
        // Set file permissions (read-only for owner on Unix systems)
        #ifdef __unix__
        std::filesystem::permissions(filename, 
            std::filesystem::perms::owner_read |
            std::filesystem::perms::owner_write);
        #endif
        
        return true;
        
    } catch (const std::exception& e) {
        log_error("Failed to save key file: " + std::string(e.what()));
        return false;
    }
}

std::string SecureVault::load_key_file(const std::string& filename) const {
    try {
        std::ifstream file(filename, std::ios::binary);
        if (!file.is_open()) {
            return "";
        }
        
        std::string key((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
        file.close();
        
        return key;
        
    } catch (const std::exception& e) {
        log_error("Failed to load key file: " + std::string(e.what()));
        return "";
    }
}

void SecureVault::log_error(const std::string& message) const {
    LOG_ERROR("SecureVault: {}", message);
}

void SecureVault::log_info(const std::string& message) const {
    LOG_INFO("SecureVault: {}", message);
}

} // namespace gmail_infinity
