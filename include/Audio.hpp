#pragma once
#include <memory>
#include <string>

namespace tempest {

struct AudioImpl;

class Audio {
public:
    Audio();
    ~Audio();
    Audio(const Audio&) = delete;
    Audio& operator=(const Audio&) = delete;

    bool init(const std::string& root);
    void destroy();
    void loadSfx(const std::string& name, const std::string& path);
    void loadMusic(const std::string& name, const std::string& path);
    void play(const std::string& name, float volume = 1.f);
    void playMusic(const std::string& name, float volume = 0.45f);
    void stopMusic();

private:
    std::unique_ptr<AudioImpl> impl_;
};

} // namespace tempest
