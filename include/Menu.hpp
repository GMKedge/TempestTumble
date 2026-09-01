#pragma once
#include "Audio.hpp"
#include "Input.hpp"
#include "Renderer.hpp"
#include "Skills.hpp"
#include "Texture.hpp"

namespace tempest {

enum class MenuTab { Sheet = 0, Tree = 1 };

struct MenuState {
    MenuTab tab = MenuTab::Sheet;
    int titleIndex = 0;
};

void updatePauseMenu(MenuState& m, SkillState& skills, const Input& in, Audio* audio);
void drawCharacterSheet(Renderer& r, const Texture& atlas, const SkillState& skills,
                        int hp, int maxHp, float x, float y);
void drawSkillTree(Renderer& r, const Texture& atlas, const SkillState& skills, float x, float y);
void drawTitle(Renderer& r, const Texture& atlas, const MenuState& m, float bob);

} // namespace tempest
