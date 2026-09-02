#include "Application.hpp"
#include <GLFW/glfw3.h>
#include <iostream>

namespace tempest {

bool Application::init(const std::string& root) {
    root_ = root;
    config_.load(root + "/config/game.cfg");
    const int w = config_.getInt("window_width", 1280);
    const int h = config_.getInt("window_height", 720);
    const bool vsync = config_.getBool("vsync", true);
    if (!window_.create(w, h, "Tempest Tumble", vsync)) return false;
    input_.init(window_.handle(), config_);
    if (!renderer_.init(root + "/shaders")) return false;
    if (!audio_.init(root)) return false;
    if (!game_.init(root, renderer_, audio_)) return false;
    std::cout << "Tempest Tumble is awake. Esc opens the menu.\n";
    return true;
}

void Application::run() {
    double last = glfwGetTime();
    double acc = 0.0;
    while (!window_.shouldClose() && !game_.quitRequested()) {
        window_.poll();
        input_.beginFrame();
        double now = glfwGetTime();
        double frame = now - last;
        last = now;
        if (frame > 0.25) frame = 0.25;
        acc += frame;
        while (acc >= kFixedDt) {
            game_.update(kFixedDt, input_, audio_);
            acc -= kFixedDt;
        }
        game_.render(renderer_, window_);
        window_.swap();
    }
}

void Application::shutdown() {
    audio_.destroy();
    renderer_.destroy();
    window_.destroy();
}

} // namespace tempest
