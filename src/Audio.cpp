#include "Audio.hpp"
#define MINIAUDIO_IMPLEMENTATION
#include <miniaudio.h>
#include <iostream>

namespace tempest {

bool Audio::init() {
    engine_ = new ma_engine();
    ma_engine_config cfg = ma_engine_config_init();
    if (ma_engine_init(&cfg, engine_) != MA_SUCCESS) {
        std::cerr << "miniaudio engine init failed (continuing silent)\n";
        delete engine_;
        engine_ = nullptr;
        return true;
    }
    return true;
}

void Audio::destroy() {
    stopMusic();
    if (engine_) {
        ma_engine_uninit(engine_);
        delete engine_;
        engine_ = nullptr;
    }
}

void Audio::loadSfx(const std::string& name, const std::string& path) { sfxPath_[name] = path; }
void Audio::loadMusic(const std::string& name, const std::string& path) { musicPath_[name] = path; }

void Audio::play(const std::string& name, float volume) {
    if (!engine_) return;
    auto it = sfxPath_.find(name);
    if (it == sfxPath_.end()) return;
    (void)volume;
    ma_engine_play_sound(engine_, it->second.c_str(), nullptr);
}

void Audio::playMusic(const std::string& name, float volume) {
    if (!engine_) return;
    if (musicName_ == name && music_) return;
    stopMusic();
    auto it = musicPath_.find(name);
    if (it == musicPath_.end()) return;
    music_ = new ma_sound();
    if (ma_sound_init_from_file(engine_, it->second.c_str(),
                                MA_SOUND_FLAG_STREAM | MA_SOUND_FLAG_DECODE, nullptr, nullptr, music_) != MA_SUCCESS) {
        delete music_;
        music_ = nullptr;
        return;
    }
    ma_sound_set_looping(music_, MA_TRUE);
    ma_sound_set_volume(music_, volume);
    ma_sound_start(music_);
    musicName_ = name;
}

void Audio::stopMusic() {
    if (music_) {
        ma_sound_uninit(music_);
        delete music_;
        music_ = nullptr;
    }
    musicName_.clear();
}

} // namespace tempest
