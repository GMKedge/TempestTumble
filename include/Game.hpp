#pragma once
#include "Audio.hpp"
#include "Camera.hpp"
#include "Combat.hpp"
#include "Entity.hpp"
#include "Input.hpp"
#include "Level.hpp"
#include "Menu.hpp"
#include "Renderer.hpp"
#include "Skills.hpp"
#include "Texture.hpp"
#include "Tilemap.hpp"
#include <string>
#include <vector>

namespace tempest {

class Window;

enum class Mode { Title, Play, Pause, Dialog, Victory, Defeat };

class Game {
public:
    bool init(const std::string& root, Renderer& renderer, Audio& audio);
    void update(float dt, const Input& input, Audio& audio);
    void render(Renderer& renderer, Window& window);
    bool quitRequested() const { return quit_; }

private:
    bool loadMap(const std::string& id, Audio& audio);
    void spawnFromLevel();
    void updatePlay(float dt, const Input& input, Audio& audio);
    glm::vec2 standOnFloor(int tx, int ty, const glm::vec2& size) const;

    std::string root_;
    Texture atlas_;
    Camera camera_;
    LevelData level_;
    Entity gale_;
    std::vector<Entity> coins_;
    std::vector<Entity> enemies_;
    std::vector<Entity> npcs_;
    CombatState combat_;
    SkillState skills_;
    MenuState menu_;
    Mode mode_ = Mode::Title;
    std::string message_;
    float messageTimer_ = 0.f;
    std::string dialogTitle_, dialogBody_;
    float titleBob_ = 0.f;
    bool quit_ = false;
    glm::vec4 sky_{0.16f, 0.20f, 0.36f, 1};
};

} // namespace tempest
