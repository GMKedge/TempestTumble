#pragma once
#include "Config.hpp"
#include <glm/glm.hpp>

struct GLFWwindow;

namespace tempest {

enum class Action {
    Left, Right, Up, Down, Jump, Attack, Ability, Dash,
    CycleAbility, Sheet, Interact, Pause, Count
};

class Input {
public:
    void init(GLFWwindow* window, const Config& cfg);
    void beginFrame();
    bool down(Action a) const;
    bool pressed(Action a) const;
    glm::vec2 moveAxis() const;
    bool gamepadConnected() const { return pad_; }

private:
    int keyFor(Action a) const { return keys_[static_cast<int>(a)]; }
    int btnFor(Action a) const { return buttons_[static_cast<int>(a)]; }
    GLFWwindow* window_ = nullptr;
    int keys_[static_cast<int>(Action::Count)]{};
    int buttons_[static_cast<int>(Action::Count)]{};
    bool down_[static_cast<int>(Action::Count)]{};
    bool prev_[static_cast<int>(Action::Count)]{};
    bool pad_ = false;
};

} // namespace tempest
