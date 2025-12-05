# ================================
# Keyboard Heroes - Single-build Makefile
# ================================

CXX := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -O2
LDFLAGS := -ljsoncpp -lpqxx -lpq

TARGET := kbh_server

# Lấy tất cả file .cpp trong thư mục
SRCS := $(wildcard *.cpp)

all: $(TARGET)

$(TARGET):
	$(CXX) $(CXXFLAGS) $(SRCS) -o $(TARGET) $(LDFLAGS)
	@echo "Build complete → ./$(TARGET)"

clean:
	rm -f $(TARGET)
	@echo "Cleaned."

.PHONY: all clean
