#pragma once

#include <string>
#include <vector>
#include <memory>
#include <mutex>
#include <map>

#include <nlohmann/json.hpp>

namespace gmail_infinity {

struct Credential {
    std::string id;
    std::string email;
    std::string password;
    std::string recovery_email;
    std::string phone;
    std::string proxy_id;
    std::string persona_id;
    std::chrono::system_clock::time_point created_at;
    std::chrono::system_clock::time_point last_used;
    bool is_active;
    std::map<std::string, std::string> metadata;
    
    nlohmann::json to_json() const;
    void from_json(const nlohmann::json& j);
};

class SecureVault {
public:
    SecureVault();
    ~SecureVault();
    
    // Vault operations
    bool load_or_create();
    bool load(const std::string& key_file = "credentials/vault.key", 
              const std::string& data_file = "credentials/vault.enc");
    bool save(const std::string& key_file = "credentials/vault.key",
              const std::string& data_file = "credentials/vault.enc");
    
    // Key management
    bool generate_key();
    bool set_key(const std::string& key);
    std::string get_key() const;
    
    // Credential operations
    std::string save_credential(const Credential& credential);
    bool update_credential(const std::string& id, const Credential& credential);
    bool delete_credential(const std::string& id);
    
    Credential get_credential(const std::string& id) const;
    std::vector<Credential> get_all_credentials() const;
    std::vector<Credential> get_active_credentials() const;
    std::vector<Credential> get_credentials_by_proxy(const std::string& proxy_id) const;
    
    // Search and filtering
    std::vector<Credential> search_credentials(const std::string& query) const;
    std::vector<Credential> get_credentials_created_after(std::chrono::system_clock::time_point time) const;
    std::vector<Credential> get_credentials_created_before(std::chrono::system_clock::time_point time) const;
    
    // Statistics
    size_t get_total_credentials() const;
    size_t get_active_credentials_count() const;
    size_t get_credentials_created_today() const;
    
    // Export/Import
    bool export_to_csv(const std::string& filename) const;
    bool import_from_csv(const std::string& filename);
    bool export_to_json(const std::string& filename) const;
    
    // Cleanup
    bool cleanup_old_credentials(std::chrono::hours age = std::chrono::hours(24 * 30)); // 30 days
    bool cleanup_inactive_credentials();
    
    // Security
    bool change_password(const std::string& new_password);
    bool verify_integrity() const;

private:
    // Encryption
    std::string encrypt_data(const std::string& data) const;
    std::string decrypt_data(const std::string& encrypted_data) const;
    
    // Key derivation
    std::string derive_key(const std::string& password, const std::string& salt) const;
    std::string generate_salt() const;
    
    // File operations
    bool save_key_file(const std::string& filename, const std::string& key) const;
    std::string load_key_file(const std::string& filename) const;
    
    // Data validation
    bool validate_credential(const Credential& credential) const;
    std::string generate_credential_id() const;
    
    // Internal storage
    mutable std::mutex mutex_;
    std::map<std::string, Credential> credentials_;
    std::string encryption_key_;
    std::string salt_;
    
    // Constants
    static constexpr size_t KEY_SIZE = 32; // AES-256
    static constexpr size_t SALT_SIZE = 16;
    static constexpr size_t IV_SIZE = 12;  // GCM IV
    static constexpr size_t TAG_SIZE = 16; // GCM tag
    
    // Error handling
    void log_error(const std::string& message) const;
    void log_info(const std::string& message) const;
    
    // Helper methods
    std::string get_timestamp() const;
    std::string sanitize_filename(const std::string& filename) const;
};

} // namespace gmail_infinity
