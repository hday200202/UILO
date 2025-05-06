# Compiler and flags
CXX = g++
CXXFLAGS = -std=c++17 -Wall -I. 
LDFLAGS = -lsfml-graphics -lsfml-window -lsfml-system -fexpensive-optimizations
INCLUDE = ../

all:

containers:
	clear
	@echo "building containers.cpp..."
	$(CXX) $(CXXFLAGS) -I$(INCLUDE) -O3 -o bin/containers containers.cpp $(LDFLAGS)
	./bin/containers

clean:
