# ===============================
# Keyboard Heroes Backend
# ===============================

CXX := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -O2

# Include paths
CXXFLAGS += \
	-Iserver \
	-Iroom \
	-Ityping_engine \
	-Igamemode \
	-Igamemode/arena \
	-Idatabase \
	-Iconfig

# Libraries
LDFLAGS := -ljsoncpp -lpqxx -lpq

# Target
TARGET := kbh_server

# Source directories
SRC_DIRS := \
	config \
	database \
	gamemode \
	gamemode/arena \
	room \
	server \
	typing_engine

# Main
MAIN_SRCS := main.cpp

# Collect sources
SRCS := $(MAIN_SRCS)
SRCS += $(foreach dir,$(SRC_DIRS),$(wildcard $(dir)/*.cpp))

# ===============================
# Build rules
# ===============================

all: $(TARGET)

$(TARGET):
	$(CXX) $(CXXFLAGS) $(SRCS) -o $(TARGET) $(LDFLAGS)
	@echo "Build complete → ./$(TARGET)"

clean:
	rm -f $(TARGET)
	@echo "Cleaned."

.PHONY: all clean
