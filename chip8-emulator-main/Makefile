# Cosmic Chip-8 Mission Control Makefile
# Mission Control Status: Stellar

CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2
SOURCES = src/main.cpp src/chip8.cpp
OBJECTS = $(SOURCES:.cpp=.o)

ifeq ($(OS),Windows_NT)
    TARGET = chip8.exe
    LDFLAGS = -lmingw32 -lSDL2main -lSDL2 -mconsole
else
    TARGET = chip8
    LDFLAGS = -lSDL2
endif

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJECTS) $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(TARGET) chip8 chip8.exe

.PHONY: all clean
