CXX      := g++
CXXFLAGS := -std=c++20 -Wall -Wextra -I ../include

SRCS     := ../include/UILO.cpp \
            ../include/Page.cpp \
            ../include/elements/Element.cpp \
            ../include/elements/Modifier.cpp \
            ../include/elements/containers/Container.cpp \
            ../include/elements/containers/Column.cpp \
            ../include/elements/containers/Row.cpp \
            ../include/elements/decor/Spacer.cpp \
            ../include/elements/decor/Text.cpp

# SFML basic (new UILO)
SFML_BASIC_TARGET  := bin/sfml_basic
SFML_BASIC_SRC     := sfml_basic.cpp
SFML_PREFIX       := /opt/homebrew/Cellar/sfml/3.0.2
SFML_BASIC_CFLAGS  := -I$(SFML_PREFIX)/include
SFML_BASIC_LDFLAGS := -L$(SFML_PREFIX)/lib -lsfml-graphics -lsfml-window -lsfml-system \
                     -Wl,-sectcreate,__TEXT,__info_plist,Info.plist

# Metal basic (macOS only)
METAL_BASIC_TARGET  := bin/metal_basic
METAL_BASIC_SRC     := metal_basic.mm
METAL_BASIC_LDFLAGS := -framework Metal -framework Cocoa -framework QuartzCore

HDRS := $(shell find ../include -name '*.hpp')

.PHONY: all clean sfml_basic metal_basic

all: sfml_basic metal_basic

sfml_basic: $(SFML_BASIC_TARGET)
metal_basic: $(METAL_BASIC_TARGET)

$(SFML_BASIC_TARGET): $(SFML_BASIC_SRC) $(SRCS) $(HDRS)
	mkdir -p bin
	$(CXX) $(CXXFLAGS) $(SFML_BASIC_CFLAGS) $(SFML_BASIC_SRC) $(SRCS) $(SFML_BASIC_LDFLAGS) -o $@

$(METAL_BASIC_TARGET): $(METAL_BASIC_SRC) $(SRCS) $(HDRS)
	mkdir -p bin
	clang++ -std=c++20 -Wall -Wextra -I ../include $(METAL_BASIC_SRC) $(SRCS) $(METAL_BASIC_LDFLAGS) -o $@

clean:
	rm -f $(SFML_BASIC_TARGET) $(METAL_BASIC_TARGET)
