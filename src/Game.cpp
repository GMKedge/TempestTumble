#include "Game.hpp"
#include "Atlas.hpp"
#include "Collision.hpp"
#include "Config.hpp"
#include "Window.hpp"
#include <algorithm>
#include <cmath>

namespace tempest {

bool Game::init(const std::string& root, Renderer& renderer, Audio& audio) {
    (void)renderer;
    root_ = root;
    if (!atlas_.loadFromFile(root + "/assets/sprites/atlas.png")) return false;
    audio.loadSfx("jump", root + "/assets/sfx/jump.wav");
    audio.loadSfx("dash", root + "/assets/sfx/dash.wav");
    audio.loadSfx("hit", root + "/assets/sfx/hit.wav");
    audio.loadSfx("hurt", root + "/assets/sfx/hurt.wav");
    audio.loadSfx("levelup", root + "/assets/sfx/levelup.wav");
    audio.loadSfx("select", root + "/assets/sfx/select.wav");
    audio.loadSfx("coin", root + "/assets/sfx/coin.wav");
    audio.loadMusic("storm", root + "/assets/music/storm.mid");
    audio.loadMusic("keep", root + "/assets/music/keep.mid");
    audio.loadMusic("title", root + "/assets/music/title.mid");
    loadSkills(skills_, root + "/config/save.cfg");
    gale_.name = "Gale";
    gale_.kind = EntityKind::Player;
    gale_.puppet.setupGale();
    gale_.applyPuppetSize();
    {
        Config cfg;
        cfg.load(root + "/config/game.cfg");
        debugHitbox_ = cfg.getBool("debug_hitbox", false);
    }
    {
        float mag, coy, dash;
        applyStats(skills_, gale_.maxHp, gale_.damage, mag, coy, dash);
        gale_.hp = gale_.maxHp;
    }
    camera_.setViewportPixels(kViewW, kViewH);
    audio.playMusic("title", 0.4f);
    return true;
}

glm::vec2 Game::standOnFloor(int tx, int ty, const glm::vec2& size) const {
    float x = tx * kTileSize + (kTileSize - size.x) * 0.5f;
    float y = ty * kTileSize - size.y;
    return {x, y};
}

void Game::spawnFromLevel() {
    coins_.clear();
    enemies_.clear();
    npcs_.clear();
    glm::vec2 spawn{32, 32};
    gale_.puppet.setupGale();
    gale_.applyPuppetSize();
    for (auto& m : level_.map.markers()) {
        glm::vec2 p = standOnFloor(m.tx, m.ty, gale_.size);
        if (m.ch == 'P') spawn = p;
        else if (m.ch == 'C') {
            Entity c;
            c.kind = EntityKind::Coin;
            c.pos = {m.tx * kTileSize + 4, m.ty * kTileSize + 4};
            c.size = {8, 8};
            coins_.push_back(c);
        } else if (m.ch == '1' || m.ch == '2') {
            Entity e;
            e.kind = EntityKind::Enemy;
            e.brain = (m.ch == '1') ? 1 : 2;
            if (m.ch == '1') e.puppet.setupWisp();
            else e.puppet.setupGolem();
            e.applyPuppetSize();
            e.pos = standOnFloor(m.tx, m.ty, e.size);
            e.spawn = e.pos;
            e.hp = e.maxHp = (m.ch == '1') ? 2 : 4;
            e.damage = 1;
            enemies_.push_back(e);
        } else if (m.ch == 'N') {
            Entity n;
            n.kind = EntityKind::Npc;
            n.brain = 3;
            n.puppet.setupTrainer();
            n.applyPuppetSize();
            n.pos = standOnFloor(m.tx, m.ty, n.size);
            n.talk = "OPEN THE SKILL TREE WITH C OR TAB";
            npcs_.push_back(n);
        }
    }
    for (auto& n : level_.npcs) {
        for (auto& e : npcs_) {
            if (std::abs(e.pos.x - n.tx * kTileSize) < 24.f) {
                e.talk = n.line.empty() ? e.talk : n.line;
                e.name = n.title;
            }
        }
    }
    gale_.pos = spawn;
    gale_.spawn = spawn;
    gale_.vel = {0, 0};
    gale_.alive = true;
    gale_.hp = gale_.maxHp;
    gale_.facingRight = true;
    gale_.puppet.setupGale();
    camera_.setWorldBounds(level_.map.pixelWidth(), level_.map.pixelHeight());
    camera_.snapTo(gale_.pos);
    sky_ = {level_.sky.x, level_.sky.y, level_.sky.z, 1};
}

bool Game::loadMap(const std::string& id, Audio& audio) {
    LevelData data;
    if (!loadLevelFile(root_ + "/levels/" + id + ".txt", data)) return false;
    level_ = std::move(data);
    spawnFromLevel();
    audio.playMusic(level_.music.empty() ? "storm" : level_.music, 0.4f);
    return true;
}

void Game::updatePlay(float dt, const Input& input, Audio& audio) {
    float mag, coy, dashDur;
    applyStats(skills_, gale_.maxHp, gale_.damage, mag, coy, dashDur);

    // kMaxSpeed=95, kJumpVel=-210, kGravity=520 -> hang 0.808s, jumpDist 76.7px (4.8 tiles), height 42.4px (2.65 tiles).
    glm::vec2 axis = input.moveAxis();
    bool dashing = combat_.dashT > 0.f;
    bool lunging = combat_.lungeT > 0.f || combat_.cycloneT > 0.f;
    if (!dashing && !lunging) {
        if (axis.x < 0) gale_.facingRight = false;
        if (axis.x > 0) gale_.facingRight = true;
        gale_.vel.x = axis.x * kMaxSpeed;
    }
    bool canJump = gale_.onGround || combat_.coyote > 0.f;
    if (input.pressed(Action::Jump) && canJump && combat_.dashT <= 0.f) {
        gale_.vel.y = kJumpVel;
        gale_.onGround = false;
        combat_.coyote = 0.f;
        audio.play("jump");
        gale_.puppet.pose = PuppetPose::Jump;
    }
    if (!dashing) gale_.vel.y += kGravity * dt;
    if (gale_.vel.y > kMaxFall) gale_.vel.y = kMaxFall;

    tickCombat(combat_, gale_, skills_, input, level_.map, enemies_, audio, dt);
    bool pass = combat_.dashT > 0.f;
    if (pass) gale_.vel.y = 0.f;
    MoveHit hit = moveAndCollide(gale_, level_.map, dt, pass);
    if (hit.hitSpike && gale_.iTimer <= 0.f) {
        hurt(gale_, 1, {gale_.facingRight ? -80.f : 80.f, -80.f}, audio);
    }
    if (hit.hitDoor && !level_.next.empty()) {
        if (level_.next == "win") {
            mode_ = Mode::Victory;
            audio.play("levelup");
        } else {
            loadMap(level_.next, audio);
        }
    }
    if (gale_.pos.y > level_.map.pixelHeight() + 40.f) {
        gale_.pos = gale_.spawn;
        gale_.vel = {0, 0};
        hurt(gale_, 1, {0, 0}, audio);
    }

    bool moving = std::abs(gale_.vel.x) > 12.f && gale_.onGround && combat_.attackT <= 0 && combat_.dashT <= 0;
    if (combat_.dashT > 0) gale_.puppet.pose = PuppetPose::Dash;
    else if (combat_.guardT > 0) gale_.puppet.pose = PuppetPose::Guard;
    else if (combat_.attackT > 0 || combat_.cycloneT > 0) gale_.puppet.pose = PuppetPose::Attack;
    else if (gale_.hurtFlash > 0) gale_.puppet.pose = PuppetPose::Hurt;
    else if (!gale_.onGround) gale_.puppet.pose = PuppetPose::Jump;
    gale_.puppet.tick(dt, moving, gale_.onGround, gale_.facingRight ? 1.f : -1.f);
    gale_.syncCollisionFromPuppet();

    for (auto& c : coins_) {
        if (!c.alive) continue;
        glm::vec2 d = gale_.bounds().center() - c.bounds().center();
        float dist = std::sqrt(d.x * d.x + d.y * d.y);
        if (dist < mag + 8.f) {
            c.alive = false;
            skills_.gold += 1;
            bool ding = false;
            grantXp(skills_, 1, message_, messageTimer_, ding);
            audio.play("coin");
            if (ding) audio.play("levelup");
            saveSkills(skills_, root_ + "/config/save.cfg");
        }
        c.animTime += dt;
    }
    for (auto& e : enemies_) {
        if (!e.alive) continue;
        e.hurtFlash = std::max(0.f, e.hurtFlash - dt);
        e.iTimer = std::max(0.f, e.iTimer - dt);
        e.animTime += dt;
        if (e.brain == 1) {
            e.vel.x = std::sin(e.animTime * 1.4f) * 30.f;
            e.vel.y = std::sin(e.animTime * 2.2f) * 10.f;
            e.pos += e.vel * dt;
            e.facingRight = e.vel.x >= 0;
            e.puppet.tick(dt, true, false, e.facingRight ? 1.f : -1.f);
            e.puppet.parts[static_cast<int>(PartSlot::Head)].sprite =
                (static_cast<int>(e.animTime * 6) % 2) ? "wisp1" : "wisp0";
            e.syncCollisionFromPuppet();
        } else {
            if (e.onGround && std::abs(e.pos.x - gale_.pos.x) < 80.f)
                e.vel.x = (gale_.pos.x > e.pos.x) ? 28.f : -28.f;
            else
                e.vel.x = std::sin(e.animTime * 0.8f) * 22.f;
            e.vel.y += kGravity * dt;
            e.facingRight = e.vel.x >= 0;
            moveAndCollide(e, level_.map, dt, false);
            e.puppet.tick(dt, std::abs(e.vel.x) > 4.f, e.onGround, e.facingRight ? 1.f : -1.f);
            e.puppet.parts[static_cast<int>(PartSlot::FarLeg)].sprite =
                (static_cast<int>(e.animTime * 5) % 2) ? "golem_legs1" : "golem_legs0";
            e.syncCollisionFromPuppet();
        }
        if (e.alive && gale_.iTimer <= 0.f && gale_.bounds().overlaps(e.bounds()) && combat_.guardT <= 0.f) {
            hurt(gale_, e.damage, {gale_.facingRight ? -90.f : 90.f, -70.f}, audio);
        }
    }
    for (auto it = enemies_.begin(); it != enemies_.end();) {
        if (!it->alive) {
            bool ding = false;
            grantXp(skills_, it->brain == 2 ? 5 : 3, message_, messageTimer_, ding);
            skills_.gold += 2;
            audio.play("hit");
            if (ding) audio.play("levelup");
            saveSkills(skills_, root_ + "/config/save.cfg");
            it = enemies_.erase(it);
        } else ++it;
    }

    if (input.pressed(Action::Interact)) {
        for (auto& n : npcs_) {
            if (n.bounds().overlaps(Rect{gale_.pos.x - 8, gale_.pos.y - 8, gale_.size.x + 16, gale_.size.y + 16})) {
                dialogTitle_ = n.name.empty() ? "TRAINER" : n.name;
                dialogBody_ = n.talk;
                mode_ = Mode::Dialog;
                audio.play("select");
            }
        }
    }

    if (gale_.hp <= 0) {
        mode_ = Mode::Defeat;
        gale_.alive = false;
    }
    camera_.follow(gale_.bounds().center(), dt);
    if (messageTimer_ > 0) messageTimer_ -= dt;
}

void Game::update(float dt, const Input& input, Audio& audio) {
    titleBob_ += dt;
    if (mode_ == Mode::Title) {
        if (input.pressed(Action::Up)) menu_.titleIndex = (menu_.titleIndex + 2) % 3;
        if (input.pressed(Action::Down)) menu_.titleIndex = (menu_.titleIndex + 1) % 3;
        bool go = input.pressed(Action::Jump) || input.pressed(Action::Attack) || input.pressed(Action::Interact);
        if (go) {
            audio.play("select");
            if (menu_.titleIndex == 0) {
                if (loadMap("ruins", audio)) mode_ = Mode::Play;
            } else if (menu_.titleIndex == 1) {
                mode_ = Mode::Pause;
                menu_.tab = MenuTab::Sheet;
            } else {
                quit_ = true;
            }
        }
        if (input.pressed(Action::Sheet)) {
            mode_ = Mode::Pause;
            menu_.tab = MenuTab::Sheet;
        }
        if (input.pressed(Action::Pause)) quit_ = true;
        return;
    }
    if (mode_ == Mode::Pause) {
        if (input.pressed(Action::Pause) || input.pressed(Action::Sheet)) {
            // if we opened sheet from title without a map, go back to title
            mode_ = gale_.spawn.x == 0 && gale_.spawn.y == 0 && level_.id.empty() ? Mode::Title : Mode::Play;
            saveSkills(skills_, root_ + "/config/save.cfg");
            audio.play("select");
            return;
        }
        updatePauseMenu(menu_, skills_, input, &audio);
        {
            float mag, coy, d;
            applyStats(skills_, gale_.maxHp, gale_.damage, mag, coy, d);
        }
        return;
    }
    if (mode_ == Mode::Dialog) {
        if (input.pressed(Action::Interact) || input.pressed(Action::Jump) || input.pressed(Action::Attack) || input.pressed(Action::Pause))
            mode_ = Mode::Play;
        return;
    }
    if (mode_ == Mode::Victory || mode_ == Mode::Defeat) {
        if (input.pressed(Action::Jump) || input.pressed(Action::Pause)) {
            gale_.hp = gale_.maxHp;
            gale_.alive = true;
            mode_ = Mode::Title;
            level_ = {};
        }
        return;
    }
    if (mode_ == Mode::Play) {
        if (input.pressed(Action::Pause) || input.pressed(Action::Sheet)) {
            mode_ = Mode::Pause;
            menu_.tab = input.pressed(Action::Sheet) ? MenuTab::Tree : MenuTab::Sheet;
            audio.play("select");
            return;
        }
        updatePlay(dt, input, audio);
    }
}

void Game::render(Renderer& renderer, Window& window) {
    window.beginLetterboxed(int(kViewW), int(kViewH));
    camera_.setViewportPixels(kViewW, kViewH);
    if (!(mode_ == Mode::Play || (mode_ == Mode::Pause && !level_.id.empty()) || mode_ == Mode::Dialog)) {
        camera_.setWorldBounds(kViewW, kViewH);
        camera_.snapTo({kViewW * 0.5f, kViewH * 0.5f});
    }
    glm::vec4 clear = sky_;
    renderer.begin(camera_, clear);

    if (mode_ == Mode::Title) {
        renderer.drawSprite(atlas_, Rect{0, 0, 4, 4}, spriteUV("whitepx"));
        for (int i = 0; i < 12; ++i) {
            float y = i * 32.f;
            glm::vec4 c{sky_.r + i * 0.03f, sky_.g + i * 0.02f, sky_.b + 0.05f, 1};
            renderer.drawQuad(Rect{0, y, kViewW + 40.f, 32}, c);
        }
        drawTitle(renderer, atlas_, menu_, std::sin(titleBob_ * 2.f) * 3.f);
        gale_.puppet.setupGale();
        gale_.puppet.tick(0.016f, false, true, 1.f);
        gale_.puppet.draw(renderer, atlas_, {480.f, 280.f}, {1, 1, 1, 1});
        renderer.end();
        return;
    }

    if (!level_.id.empty()) {
        const Tilemap& map = level_.map;
        glm::vec2 cam = camera_.position();
        int tx0 = std::max(0, int(cam.x / kTileSize) - 1);
        int ty0 = std::max(0, int(cam.y / kTileSize) - 1);
        int tx1 = std::min(map.width(), int((cam.x + camera_.viewWidth()) / kTileSize) + 2);
        int ty1 = std::min(map.height(), int((cam.y + camera_.viewHeight()) / kTileSize) + 2);
        renderer.drawSprite(atlas_, Rect{0, 0, 1, 1}, spriteUV("whitepx"));
        for (int i = 0; i < 6; ++i) {
            glm::vec4 c{sky_.r + i * 0.04f, sky_.g + i * 0.03f, sky_.b + i * 0.02f, 1};
            renderer.drawQuad(Rect{cam.x, cam.y + i * 60.f, camera_.viewWidth() + 8, 64}, c);
        }
        for (int ty = ty0; ty < ty1; ++ty) {
            for (int tx = tx0; tx < tx1; ++tx) {
                Tile t = map.at(tx, ty);
                if (t == Tile::Empty) continue;
                renderer.drawSprite(atlas_, Rect{tx * kTileSize, ty * kTileSize, kTileSize, kTileSize},
                                    spriteUV(Tilemap::spriteName(t)));
            }
        }
        for (auto& c : coins_) {
            if (!c.alive) continue;
            const char* s = (static_cast<int>(c.animTime * 6) % 2) ? "coin1" : "coin0";
            renderer.drawSprite(atlas_, Rect{c.pos.x, c.pos.y, 16, 16}, spriteUV(s));
        }
        for (auto& n : npcs_) n.puppet.draw(renderer, atlas_, n.feet(), {1, 1, 1, 1});
        for (auto& e : enemies_) {
            glm::vec4 tint = e.hurtFlash > 0 ? glm::vec4{1, 0.5f, 0.5f, 1} : glm::vec4{1, 1, 1, 1};
            e.puppet.draw(renderer, atlas_, e.feet(), tint);
        }
        glm::vec4 gt = gale_.hurtFlash > 0 ? glm::vec4{1, 0.6f, 0.6f, 1} : glm::vec4{1, 1, 1, 1};
        if (gale_.iTimer > 0.f && std::fmod(gale_.iTimer, 0.08f) < 0.04f) gt.a = 0.5f;
        gale_.puppet.draw(renderer, atlas_, gale_.feet(), gt);
        if (debugHitbox_) {
            auto drawHb = [&](const Rect& b, const glm::vec4& c) {
                renderer.drawQuad(b, c);
            };
            drawHb(gale_.bounds(), {0.2f, 0.9f, 0.3f, 0.28f});
            for (auto& e : enemies_)
                if (e.alive) drawHb(e.bounds(), {0.9f, 0.2f, 0.2f, 0.28f});
            for (auto& n : npcs_) drawHb(n.bounds(), {0.2f, 0.5f, 0.9f, 0.22f});
        }
        if (combat_.fxT > 0.f) {
            renderer.drawSprite(atlas_, Rect{combat_.fxPos.x, combat_.fxPos.y, 24, 24}, spriteUV(combat_.fxSprite));
        }

        glm::vec2 hud = camera_.position();
        for (int i = 0; i < gale_.maxHp; ++i) {
            renderer.drawSprite(atlas_, Rect{hud.x + 10 + i * 22, hud.y + 8, 24, 24},
                                spriteUV(i < gale_.hp ? "heart" : "heart_empty"));
        }
        renderer.drawText(atlas_, "LV" + std::to_string(skills_.level), {hud.x + 10, hud.y + 36}, 2.f);
        if (messageTimer_ > 0.f)
            renderer.drawText(atlas_, message_, {hud.x + 24, hud.y + 68}, 2.f, {1, 0.95f, 0.4f, 1});
    }

    if (mode_ == Mode::Pause) {
        glm::vec2 o = camera_.position();
        if (level_.id.empty()) o = {80, 20};
        if (menu_.tab == MenuTab::Sheet)
            drawCharacterSheet(renderer, atlas_, skills_, gale_.hp, gale_.maxHp, o.x + 80, o.y + 28);
        else
            drawSkillTree(renderer, atlas_, skills_, o.x + 60, o.y + 16);
        renderer.drawText(atlas_, "LEFT/RIGHT TAB   C/ESC BACK", {o.x + 40, o.y + 330}, 2.f);
    }
    if (mode_ == Mode::Dialog) {
        glm::vec2 o = camera_.position();
        renderer.drawQuad(Rect{o.x + 40, o.y + 250, 560, 90}, {0.07f, 0.08f, 0.14f, 0.92f});
        renderer.drawText(atlas_, dialogTitle_, {o.x + 52, o.y + 258}, 2.f, {1, 0.9f, 0.5f, 1});
        renderer.drawText(atlas_, dialogBody_, {o.x + 52, o.y + 290}, 2.f);
    }
    if (mode_ == Mode::Victory)
        renderer.drawText(atlas_, "THE RUINS FALL QUIET", {80, 160}, 2.f, {1, 0.95f, 0.5f, 1});
    if (mode_ == Mode::Defeat)
        renderer.drawText(atlas_, "GALE FALLS  JUMP TO TITLE", {60, 160}, 2.f, {1, 0.5f, 0.5f, 1});

    renderer.end();
}

} // namespace tempest
