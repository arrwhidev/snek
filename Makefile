# Define the compiler
CXX = clang++

# Define the flags for the compiler
CXXFLAGS = -I/opt/homebrew/opt/raylib/include -std=c++20

# Define the libraries to link
LDFLAGS = -L/opt/homebrew/opt/raylib/lib -lraylib

# Define the source and target files
SRC = game.cpp
TARGET = a.out

# Default target to build the project
all: $(TARGET)

# Rule to build the executable
$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) $(SRC) $(LDFLAGS) -o $(TARGET)

# Clean the build
clean:
	rm -f $(TARGET)
