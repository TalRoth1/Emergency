# Compiler and Flags
CXX := g++
CFLAGS := -c -Wall -Weffc++ -g -std=c++11 -Iinclude
LDFLAGS := -lboost_system -lpthread

# Directories
SRCDIR := src
BINDIR := bin
INCLUDEDIR := include

# Target Executable
TARGET := bin/StompWCIClient

# Source and Object Files
SOURCES := $(wildcard $(SRCDIR)/*.cpp)
OBJECTS := $(patsubst $(SRCDIR)/%.cpp,$(BINDIR)/%.o,$(SOURCES))

# Default Target
all: $(TARGET)

# Build Target Executable
$(TARGET): $(OBJECTS)
	$(CXX) -o $@ $^ $(LDFLAGS)

# Build Object Files
$(BINDIR)/%.o: $(SRCDIR)/%.cpp $(INCLUDEDIR)/%.h
	$(CXX) $(CFLAGS) -o $@ $<

# Clean Build Files
.PHONY: clean
clean:
	rm -f $(BINDIR)/*
