# Vendored libraries

Kids: these folders are other people's code that we *use*, not code we wrote.

| Folder | What it does | How we build it |
|---|---|---|
| `glad/` | Loads OpenGL 3.3 function pointers | compiled (`glad.c`) |
| `glm/` | Vector / matrix math (header-only) | `#include` |
| `stb/` | `stb_image.h` loads PNG files | one `.cpp` defines the implementation |
| `miniaudio/` | Plays WAV SFX and hosts the audio callback | one `.cpp` defines the implementation |
| `tsf/` | TinySoundFont + TinyMidiLoader (MIDI + SF2) | header-only, implemented in `src/Audio.cpp` |

## GLFW (window + input)

GLFW is **not** copied here. Use the system package:

- Linux: `sudo apt install libglfw3-dev` then `pkg-config glfw3`
- Windows MinGW: `pacman -S mingw-w64-x86_64-glfw` and link `-lglfw3 -lopengl32 -lgdi32`

OpenGL itself comes from your GPU driver / Mesa.
