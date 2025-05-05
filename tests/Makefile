# Compiler and flags
CXX = g++
CXXFLAGS = -std=c++17 -Wall -I. 
LDFLAGS = -lsfml-graphics -lsfml-window -lsfml-system
INCLUDE = ../

all:

text:
	clear
	@echo "building text.cpp..."
	$(CXX) $(CXXFLAGS) -I$(INCLUDE) -o bin/text text.cpp $(LDFLAGS)
	./bin/text

clean:
