CXX      := g++
CXXFLAGS := -std=c++20 -Wall -Wextra -I ..

# SFML backend
SFML_TARGET  := bin/sfml_example
SFML_SRC     := sfml_example.cpp
SFML_LDFLAGS := -lsfml-graphics -lsfml-window -lsfml-system -lGL

# SDL2 backend
SDL_TARGET   := bin/sdl_example
SDL_SRC      := sdl_example.cpp
SDL_LDFLAGS  := -lSDL2 -lSDL2_ttf -lSDL2_image

# Raylib backend
RL_TARGET    := bin/raylib_example
RL_SRC       := raylib_example.cpp
RL_LDFLAGS   := -lraylib -lGL -lm -lpthread -ldl

.PHONY: all clean sfml sdl raylib

all: sfml sdl raylib

sfml: $(SFML_TARGET)
sdl: $(SDL_TARGET)
raylib: $(RL_TARGET)

$(SFML_TARGET): $(SFML_SRC)
	$(CXX) $(CXXFLAGS) $< $(SFML_LDFLAGS) -o $@

$(SDL_TARGET): $(SDL_SRC)
	$(CXX) $(CXXFLAGS) $< $(SDL_LDFLAGS) -o $@

$(RL_TARGET): $(RL_SRC)
	$(CXX) $(CXXFLAGS) $< $(RL_LDFLAGS) -o $@

clean:
	rm -f $(SFML_TARGET) $(SDL_TARGET) $(RL_TARGET)
