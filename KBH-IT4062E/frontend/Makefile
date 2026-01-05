CXX      := g++
CXXFLAGS := -std=c++17 -O2 -Wall -Wextra

TARGET   := keyboard_hero_title
SRCS     := main.cpp
OBJS     := $(SRCS:.cpp=.o)

SDL_CFLAGS := $(shell sdl2-config --cflags)
SDL_LIBS   := $(shell sdl2-config --libs)

IMG_CFLAGS := $(shell pkg-config --cflags SDL2_image)
IMG_LIBS   := $(shell pkg-config --libs SDL2_image)

TTF_CFLAGS := $(shell pkg-config --cflags SDL2_ttf)
TTF_LIBS   := $(shell pkg-config --libs SDL2_ttf)

CXXFLAGS += $(SDL_CFLAGS) $(IMG_CFLAGS) $(TTF_CFLAGS)
LDFLAGS  += $(SDL_LIBS) $(IMG_LIBS) $(TTF_LIBS)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean run
