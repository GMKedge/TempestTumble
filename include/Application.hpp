#pragma once
#include "Audio.hpp"
#include "Config.hpp"
#include "Game.hpp"
#include "Input.hpp"
#include "Renderer.hpp"
#include "Window.hpp"
#include <string>

namespace tempest {

class Application {
public:
    bool init(const std::string& root);
    void run();
    void shutdown();

private:
    std::string root_;
    Config config_;
    Window window_;
    Renderer renderer_;
    Audio audio_;
    Input input_;
    Game game_;
};

} // namespace tempest
