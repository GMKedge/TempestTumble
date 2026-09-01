#include "Config.hpp"
#include <cctype>
#include <fstream>
#include <sstream>

namespace tempest {
namespace {
std::string trim(const std::string& s) {
    size_t a = 0, b = s.size();
    while (a < b && std::isspace(static_cast<unsigned char>(s[a]))) ++a;
    while (b > a && std::isspace(static_cast<unsigned char>(s[b - 1]))) --b;
    return s.substr(a, b - a);
}
}

bool Config::load(const std::string& path) {
    std::ifstream in(path);
    if (!in) return false;
    std::string line;
    while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        auto t = trim(line);
        if (t.empty() || t[0] == '#') continue;
        auto eq = t.find('=');
        if (eq == std::string::npos) continue;
        values_[trim(t.substr(0, eq))] = trim(t.substr(eq + 1));
    }
    return true;
}

bool Config::save(const std::string& path) const {
    std::ofstream out(path);
    if (!out) return false;
    out << "# Tempest Tumble save — skills, xp, gold\n";
    for (auto& kv : values_) out << kv.first << "=" << kv.second << "\n";
    return true;
}

int Config::getInt(const std::string& key, int fallback) const {
    auto it = values_.find(key);
    if (it == values_.end()) return fallback;
    try { return std::stoi(it->second); } catch (...) { return fallback; }
}

bool Config::getBool(const std::string& key, bool fallback) const {
    auto it = values_.find(key);
    if (it == values_.end()) return fallback;
    const auto& v = it->second;
    return v == "1" || v == "true" || v == "yes" || v == "on";
}

std::string Config::getString(const std::string& key, const std::string& fallback) const {
    auto it = values_.find(key);
    return it == values_.end() ? fallback : it->second;
}

void Config::set(const std::string& key, const std::string& value) { values_[key] = value; }
void Config::setInt(const std::string& key, int value) { values_[key] = std::to_string(value); }

} // namespace tempest
