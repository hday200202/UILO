CXX      := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -I ../include -I ../include/elements
LDFLAGS  := -L ../lib -luilo -lsfml-graphics -lsfml-window -lsfml-system

.PHONY: containers clean

containers: bin/containers

bin/containers: containers.cpp | bin/
	$(CXX) $(CXXFLAGS) $< -o $@ $(LDFLAGS)

bin/:
	mkdir -p bin

clean:
	rm -rf bin/
