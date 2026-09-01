#include "Input.hpp"
#include <GLFW/glfw3.h>
#include <cctype>

namespace tempest {
namespace {
int parseKey(const std::string& name) {
    std::string n = name;
    for (auto& c : n) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    if (n == "SPACE") return GLFW_KEY_SPACE;
    if (n == "ESCAPE" || n == "ESC") return GLFW_KEY_ESCAPE;
    if (n == "ENTER" || n == "RETURN") return GLFW_KEY_ENTER;
    if (n == "TAB") return GLFW_KEY_TAB;
    if (n == "LEFT") return GLFW_KEY_LEFT;
    if (n == "RIGHT") return GLFW_KEY_RIGHT;
    if (n == "UP") return GLFW_KEY_UP;
    if (n == "DOWN") return GLFW_KEY_DOWN;
    if (n == "LEFT_SHIFT" || n == "SHIFT") return GLFW_KEY_LEFT_SHIFT;
    if (n.size() == 1 && n[0] >= 'A' && n[0] <= 'Z') return GLFW_KEY_A + (n[0] - 'A');
    if (n.size() == 1 && n[0] >= '0' && n[0] <= '9') return GLFW_KEY_0 + (n[0] - '0');
    return GLFW_KEY_UNKNOWN;
}
}

void Input::init(GLFWwindow* window, const Config& cfg) {
    window_ = window;
    auto bind = [&](Action a, const char* keyName, const char* defKey, const char* padName, int defPad) {
        keys_[static_cast<int>(a)] = parseKey(cfg.getString(keyName, defKey));
        buttons_[static_cast<int>(a)] = cfg.getInt(padName, defPad);
    };
    bind(Action::Left, "key_left", "A", "pad_left", GLFW_GAMEPAD_BUTTON_DPAD_LEFT);
    bind(Action::Right, "key_right", "D", "pad_right", GLFW_GAMEPAD_BUTTON_DPAD_RIGHT);
    bind(Action::Up, "key_up", "W", "pad_up", GLFW_GAMEPAD_BUTTON_DPAD_UP);
    bind(Action::Down, "key_down", "S", "pad_down", GLFW_GAMEPAD_BUTTON_DPAD_DOWN);
    bind(Action::Jump, "key_jump", "SPACE", "pad_jump", GLFW_GAMEPAD_BUTTON_A);
    bind(Action::Attack, "key_attack", "J", "pad_attack", GLFW_GAMEPAD_BUTTON_X);
    bind(Action::Ability, "key_ability", "K", "pad_ability", GLFW_GAMEPAD_BUTTON_Y);
    bind(Action::Dash, "key_dash", "L", "pad_dash", GLFW_GAMEPAD_BUTTON_LEFT_BUMPER);
    bind(Action::CycleAbility, "key_cycle_ability", "Q", "pad_cycle_ability", GLFW_GAMEPAD_BUTTON_LEFT_BUMPER);
    bind(Action::Sheet, "key_sheet", "C", "pad_sheet", GLFW_GAMEPAD_BUTTON_RIGHT_BUMPER);
    bind(Action::Interact, "key_interact", "E", "pad_interact", GLFW_GAMEPAD_BUTTON_B);
    bind(Action::Pause, "key_pause", "ESCAPE", "pad_pause", GLFW_GAMEPAD_BUTTON_START);
}

void Input::beginFrame() {
    for (int i = 0; i < static_cast<int>(Action::Count); ++i) prev_[i] = down_[i];
    for (int i = 0; i < static_cast<int>(Action::Count); ++i) down_[i] = false;

    auto keyDown = [&](int key) {
        return key != GLFW_KEY_UNKNOWN && glfwGetKey(window_, key) == GLFW_PRESS;
    };
    bool left = keyDown(keyFor(Action::Left)) || keyDown(GLFW_KEY_LEFT);
    bool right = keyDown(keyFor(Action::Right)) || keyDown(GLFW_KEY_RIGHT);
    bool up = keyDown(keyFor(Action::Up)) || keyDown(GLFW_KEY_UP);
    bool down = keyDown(keyFor(Action::Down)) || keyDown(GLFW_KEY_DOWN);
    down_[static_cast<int>(Action::Left)] = left;
    down_[static_cast<int>(Action::Right)] = right;
    down_[static_cast<int>(Action::Up)] = up;
    down_[static_cast<int>(Action::Down)] = down;
    for (Action a : {Action::Jump, Action::Attack, Action::Ability, Action::Dash,
                     Action::CycleAbility, Action::Sheet, Action::Interact, Action::Pause}) {
        down_[static_cast<int>(a)] = keyDown(keyFor(a));
    }
    // Aliases: Shift dash, Tab sheet, E confirm already bound
    down_[static_cast<int>(Action::Dash)] =
        down_[static_cast<int>(Action::Dash)] || keyDown(GLFW_KEY_LEFT_SHIFT) || keyDown(GLFW_KEY_RIGHT_SHIFT);
    down_[static_cast<int>(Action::Sheet)] =
        down_[static_cast<int>(Action::Sheet)] || keyDown(GLFW_KEY_TAB) || keyDown(GLFW_KEY_C);

    pad_ = false;
    GLFWgamepadstate gs;
    if (glfwJoystickIsGamepad(GLFW_JOYSTICK_1) && glfwGetGamepadState(GLFW_JOYSTICK_1, &gs)) {
        pad_ = true;
        auto btn = [&](int b) { return b >= 0 && b <= 14 && gs.buttons[b] == GLFW_PRESS; };
        down_[static_cast<int>(Action::Left)] = down_[static_cast<int>(Action::Left)] || btn(btnFor(Action::Left)) || gs.axes[GLFW_GAMEPAD_AXIS_LEFT_X] < -0.4f;
        down_[static_cast<int>(Action::Right)] = down_[static_cast<int>(Action::Right)] || btn(btnFor(Action::Right)) || gs.axes[GLFW_GAMEPAD_AXIS_LEFT_X] > 0.4f;
        down_[static_cast<int>(Action::Up)] = down_[static_cast<int>(Action::Up)] || btn(btnFor(Action::Up)) || gs.axes[GLFW_GAMEPAD_AXIS_LEFT_Y] < -0.4f;
        down_[static_cast<int>(Action::Down)] = down_[static_cast<int>(Action::Down)] || btn(btnFor(Action::Down)) || gs.axes[GLFW_GAMEPAD_AXIS_LEFT_Y] > 0.4f;
        for (Action a : {Action::Jump, Action::Attack, Action::Ability, Action::Dash,
                         Action::CycleAbility, Action::Sheet, Action::Interact, Action::Pause}) {
            down_[static_cast<int>(a)] = down_[static_cast<int>(a)] || btn(btnFor(a));
        }
    }
}

bool Input::down(Action a) const { return down_[static_cast<int>(a)]; }
bool Input::pressed(Action a) const {
    return down_[static_cast<int>(a)] && !prev_[static_cast<int>(a)];
}
glm::vec2 Input::moveAxis() const {
    float x = (down(Action::Right) ? 1.f : 0.f) - (down(Action::Left) ? 1.f : 0.f);
    float y = (down(Action::Down) ? 1.f : 0.f) - (down(Action::Up) ? 1.f : 0.f);
    return {x, y};
}

} // namespace tempest
