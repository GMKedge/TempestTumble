# Tempest Tumble — one Makefile for Linux gcc and Windows MinGW.
# Linux:  sudo apt install g++ make pkg-config libglfw3-dev libgl1-mesa-dev
#         make
# Windows MinGW: install MSYS2, `pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-glfw`
#         mingw32-make
# If pkg-config cannot find GLFW, set GLFW_LIBS yourself, e.g.
#   make GLFW_LIBS="-lglfw3 -lopengl32 -lgdi32"

CXX      ?= g++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra
CXXFLAGS += -Iinclude -Ithird_party/glad/include -Ithird_party/glm -Ithird_party/stb -Ithird_party/miniaudio -Ithird_party/tsf
CXXFLAGS += -Ithird_party/glm/glm -Ithird_party/glm

SRC = \
	src/main.cpp \
	src/Application.cpp \
	src/Window.cpp \
	src/Shader.cpp \
	src/Texture.cpp \
	src/Renderer.cpp \
	src/Camera.cpp \
	src/Input.cpp \
	src/Audio.cpp \
	src/Config.cpp \
	src/Tilemap.cpp \
	src/Collision.cpp \
	src/Puppet.cpp \
	src/Combat.cpp \
	src/Skills.cpp \
	src/Menu.cpp \
	src/Game.cpp \
	src/Level.cpp \
	third_party/glad/src/glad.c

OBJ = $(SRC:.cpp=.o)
OBJ := $(OBJ:.c=.o)

ifeq ($(OS),Windows_NT)
  EXE  := tempest-tumble.exe
  LIBS ?= $(GLFW_LIBS)
  ifeq ($(LIBS),)
    LIBS := -lglfw3 -lopengl32 -lgdi32 -lwinmm
  endif
else
  EXE  := tempest-tumble
  GLFW_CFLAGS := $(shell pkg-config --cflags glfw3 2>/dev/null)
  GLFW_LFLAGS := $(shell pkg-config --libs glfw3 2>/dev/null)
  ifeq ($(GLFW_LFLAGS),)
    GLFW_LFLAGS := -lglfw
  endif
  CXXFLAGS += $(GLFW_CFLAGS)
  LIBS := $(GLFW_LFLAGS) -ldl -lpthread -lm
endif

.PHONY: all clean run check assets

all: $(EXE)

$(EXE): $(OBJ)
	$(CXX) -o $@ $(OBJ) $(LIBS)

src/%.o: src/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

third_party/glad/src/glad.o: third_party/glad/src/glad.c
	$(CXX) $(CXXFLAGS) -c -o $@ $<

run: $(EXE)
	./$(EXE)

check: tools/check_levels
	./tools/check_levels .

tools/check_levels: tools/check_levels.cpp src/Level.cpp src/Tilemap.cpp include/Level.hpp include/Tilemap.hpp
	$(CXX) $(CXXFLAGS) -o $@ tools/check_levels.cpp src/Level.cpp src/Tilemap.cpp

assets:
	python3 tools/slice_gale_sheet.py

clean:
	rm -f $(OBJ) $(EXE) tempest-tumble.exe tools/check_levels
