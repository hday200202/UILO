CXX      := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -I include -I include/elements
AR       := ar
ARFLAGS  := rcs

SRCS := \
	include/UILO.cpp \
	include/Page.cpp \
	include/elements/Element.cpp \
	include/elements/Modifier.cpp \
	include/elements/containers/Container.cpp \
	include/elements/containers/Column.cpp \
	include/elements/containers/Row.cpp \
	include/elements/decoration/Spacer.cpp \
	include/elements/decoration/Image.cpp \
	include/elements/decoration/Text.cpp \
	include/elements/interactible/Button.cpp

OBJS := $(patsubst include/%.cpp,obj/%.o,$(SRCS))

.PHONY: lib clean

lib: lib/libuilo.a

lib/libuilo.a: $(OBJS) | lib/
	$(AR) $(ARFLAGS) $@ $^

obj/%.o: include/%.cpp
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

lib/:
	mkdir -p lib

clean:
	rm -rf obj/ lib/
