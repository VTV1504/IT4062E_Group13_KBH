CXX      := g++
CXXFLAGS := -std=c++17 -O2 -Wall -Wextra

TARGET   := keyboard_hero_title.exe

SRC_DIR  := src
BUILD_DIR:= build

# lấy tất cả .cpp trong src (kể cả subfolder)
SRCS := $(shell find $(SRC_DIR) -type f -name "*.cpp")
OBJS := $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(SRCS))

SDL_CFLAGS := $(shell sdl2-config --cflags)
SDL_LIBS   := $(shell sdl2-config --libs)

IMG_CFLAGS := $(shell pkg-config --cflags SDL2_image)
IMG_LIBS   := $(shell pkg-config --libs SDL2_image)

TTF_CFLAGS := $(shell pkg-config --cflags SDL2_ttf)
TTF_LIBS   := $(shell pkg-config --libs SDL2_ttf)

CXXFLAGS += $(SDL_CFLAGS) $(IMG_CFLAGS) $(TTF_CFLAGS) -I$(SRC_DIR)
LDFLAGS  += $(SDL_LIBS) $(IMG_LIBS) $(TTF_LIBS) -ljsoncpp

all: $(TARGET)

# link ra binary ở root
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

# compile: src/a/b.cpp -> build/a/b.o
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

run: $(TARGET)
	./$(TARGET)

clean:
	rm -rf $(BUILD_DIR) $(TARGET)

debug: CXXFLAGS := -std=c++17 -O0 -g3 -Wall -Wextra $(SDL_CFLAGS) $(IMG_CFLAGS) $(TTF_CFLAGS) -Isrc
debug: clean $(TARGET)


.PHONY: all run clean
