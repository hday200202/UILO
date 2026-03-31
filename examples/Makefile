CXX      := g++
CXXFLAGS := -std=c++20 -Wall -Wextra -I ../include

SRCS     := ../include/elements/Element.cpp \
            ../include/elements/containers/Container.cpp \
            ../include/elements/containers/Column.cpp \
            ../include/elements/containers/Row.cpp

# SFML backend
SFML_TARGET  := bin/sfml_example
SFML_SRC     := sfml_example.cpp
SFML_LDFLAGS := -lsfml-graphics -lsfml-window -lsfml-system -lGL

# SDL2 backend
SDL_TARGET   := bin/sdl_example
SDL_SRC      := sdl_example.cpp
SDL_LDFLAGS  := -lSDL2 -lSDL2_ttf -lSDL2_image

# SDL2 basic (new UILO)
SDL_BASIC_TARGET := bin/sdl_basic
SDL_BASIC_SRC    := sdl_basic.cpp
SDL_BASIC_LDFLAGS := $(shell pkg-config --libs sdl2)

# Raylib backend
RL_TARGET    := bin/raylib_example
RL_SRC       := raylib_example.cpp
RL_LDFLAGS   := -lraylib -lGL -lm -lpthread -ldl

.PHONY: all clean sfml sdl sdl_basic raylib

all: sfml sdl sdl_basic raylib

sfml: $(SFML_TARGET)
sdl: $(SDL_TARGET)
sdl_basic: $(SDL_BASIC_TARGET)
raylib: $(RL_TARGET)

$(SFML_TARGET): $(SFML_SRC)
	$(CXX) $(CXXFLAGS) $< $(SFML_LDFLAGS) -o $@

$(SDL_TARGET): $(SDL_SRC)
	$(CXX) $(CXXFLAGS) $< $(SDL_LDFLAGS) -o $@

$(SDL_BASIC_TARGET): $(SDL_BASIC_SRC) $(SRCS)
	$(CXX) $(CXXFLAGS) $^ $(SDL_BASIC_LDFLAGS) -o $@

$(RL_TARGET): $(RL_SRC)
	$(CXX) $(CXXFLAGS) $< $(RL_LDFLAGS) -o $@

clean:
	rm -f $(SFML_TARGET) $(SDL_TARGET) $(SDL_BASIC_TARGET) $(RL_TARGET)
