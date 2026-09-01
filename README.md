# Tempest Tumble

A tiny C++17 storm-knight platformer. You play **Gale**, a cloak-and-greaves tempest mage who lunges, dashes, and vaults through rain-washed ruins.

Kids: Gale is **not** one blob picture. The game glues body parts together (cape, arms, chest, head, legs, spear) and wiggles each piece. That puppet lives in `src/Puppet.cpp`.

## Build

Linux:

```
sudo apt install g++ make pkg-config libglfw3-dev libgl1-mesa-dev python3-pil
python3 tools/generate_assets.py   # atlas + wavs (already shipped)
make
./tempest-tumble
```

Windows MinGW (MSYS2):

```
pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-glfw
mingw32-make
tempest-tumble.exe
```

Headless map check (no OpenGL): `make check`

## Controls

| Action | Keyboard | Gamepad |
|---|---|---|
| Run | A / D | Stick / D-pad |
| Jump | Space | A |
| Tempest Strike (lunge) | J | X |
| Equipped skill | K | Y |
| Dash (Bolt Step, if bought) | Shift or L | LB |
| Cycle K skill | Q | LB |
| Character sheet / skill tree | C or Tab | RB |
| Pause | Esc | Start |
| Confirm / talk | E or J | B / A |

First Jump/Start on the title screen enters **Stormwash Ruins**.

## Combat that moves you

Abilities shove Gale; they are not in-place flashes.

- **Tempest Strike** (J, free): short lunge, hitbox in front, tiny i-frames.
- **Cyclone Cleave**: spin that carries Gale sideways while hitting.
- **Bolt Step** (Shift/L): blink-dash with i-frames; crosses 2–3 tile pits.
- **Updraft**: extra air burst / double-jump.
- **Gale Vault**: leap a 2-tile wall or a wide pit.
- **Stormguard**: brief shield and knockback backward.

## Skill tree (~11 nodes)

Level-ups grant **skill points**. Spend them on the tree (arrows + E/J). Locked nodes are dim.

- Core: Tempest Strike (free)
- Combat branch: Wider Strike → Cyclone Cleave → Stormguard
- Mobility branch: Bolt Step → Updraft → Gale Vault → Air Current
- Stats: Storm Heart (HP), Runed Spear (damage), Essence Magnet

XP comes from coins (essence) and enemies. On level-up: `POINT EARNED - OPEN SKILL TREE`.

Choices persist in `config/save.cfg`.

## How the puppet works

Feet/hips are the origin. Each `BodyPart` has a local offset, z-order, sprite frame, and extra animation offset. FlipX mirrors offsets. Draw order is cape → back arm → far leg → torso → head → near leg → front arm → spear. Walk cycles the legs out of phase, arms counter-swing, the cape lags, and the head bobs. Jump tucks the legs. Attack translates the whole body plus the weapon arm.

## Engine notes

OpenGL 3.3 core, GLFW, GLAD, GLM, stb_image, miniaudio. Camera is y-down. Sprite V is **not** inverted: dest top samples atlas v0 (PNG top). Dest rects are pixel-snapped.
