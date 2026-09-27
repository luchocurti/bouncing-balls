# ==============================================================================
# @file Makefile
# @brief Build configuration for the bouncing-ball application.
#
# Defines the compiler settings, source and object file discovery, dependency
# generation, linking, execution, and cleanup targets used to build the project.
# ==============================================================================

# ------------------------------------------------------------------------------
# Toolchain and compiler flags
# ------------------------------------------------------------------------------

# Allow the caller to override the compiler while keeping g++ as the default.
CXX ?= g++

# Build warnings are intentionally strict so common type/sign issues are caught
# during compilation rather than becoming runtime surprises.
CXXFLAGS ?= -std=c++20 -O2 -Wall -Wextra
CXXFLAGS += -Wpedantic -Wconversion -Wsign-conversion
CXXFLAGS += -Wshadow -Wdouble-promotion

# SDL provides its include and library flags through sdl2-config.
CPPFLAGS ?= $(shell sdl2-config --cflags)
CPPFLAGS += -Iinclude

LDFLAGS ?=
LDLIBS ?= $(shell sdl2-config --libs)

# ------------------------------------------------------------------------------
# Project Directories and Files
# ------------------------------------------------------------------------------

SRC_DIR   := src
BUILD_DIR := build
BIN_DIR   := bin

TARGET := $(BIN_DIR)/bouncing_balls

# Collect all C++ translation units below SRC_DIR/ so new source files are
# picked up automatically without updating the Makefile.
SRC := $(wildcard $(SRC_DIR)/*.cpp)

# Store object and dependency files under BUILD_DIR/ while preserving the
# source-file names.
OBJ := $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(SRC))
DEP := $(patsubst $(BUILD_DIR)/%.o,$(BUILD_DIR)/%.d,$(OBJ))

# ------------------------------------------------------------------------------
# Default and utility targets
# ------------------------------------------------------------------------------

.PHONY: all clean run

# Build the application binary.
all: $(TARGET)

# Link all object files into the final executable.
$(TARGET): $(OBJ) | $(BIN_DIR)
	$(CXX) $(LDFLAGS) $^ -o $@ $(LDLIBS)

# Compile each source file and emit a companion dependency file.
#   -MMD omits system-header dependencies from the generated .d file.
#   -MP adds phony targets for removed headers so stale dependency files do not
# break subsequent builds.
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -MMD -MP -c $< -o $@

# Create the output directories when they do not already exist.
$(BUILD_DIR) $(BIN_DIR):
	mkdir -p $@

# Include compiler-generated header dependencies when they exist. The leading
# '-' keeps the first build working before any .d files have been generated.
-include $(DEP)

# Run the application after building it.
run: $(TARGET)
	./$(TARGET)

# Remove the executable, object files, and generated dependency files.
clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)
