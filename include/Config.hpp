#pragma once
#include <string>
#include <unordered_map>

namespace tempest {

class Config {
public:
    bool load(const std::string& path);
    bool save(const std::string& path) const;
    int getInt(const std::string& key, int fallback) const;
    bool getBool(const std::string& key, bool fallback) const;
    std::string getString(const std::string& key, const std::string& fallback) const;
    void set(const std::string& key, const std::string& value);
    void setInt(const std::string& key, int value);

private:
    std::unordered_map<std::string, std::string> values_;
};

} // namespace tempest
