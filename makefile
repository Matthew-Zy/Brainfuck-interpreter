CXX? = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -Wunused

ifeq ($(OS),Windows_NT)
    EXE = .exe
else
    EXE = 
endif

TARGET = bf$(EXE)

$(TARGET): brainfuck.cpp
	$(CXX) $^ $(CXXFLAGS) -o $@
