#pragma once
#include <string>
#include <unordered_map>

struct ma_engine;
struct ma_sound;

namespace tempest {

class Audio {
public:
    bool init();
    void destroy();
    void loadSfx(const std::string& name, const std::string& path);
    void loadMusic(const std::string& name, const std::string& path);
    void play(const std::string& name, float volume = 1.f);
    void playMusic(const std::string& name, float volume = 0.45f);
    void stopMusic();

private:
    ma_engine* engine_ = nullptr;
    std::unordered_map<std::string, std::string> sfxPath_;
    std::unordered_map<std::string, std::string> musicPath_;
    ma_sound* music_ = nullptr;
    std::string musicName_;
};

} // namespace tempest
