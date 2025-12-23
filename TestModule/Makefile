CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2

TARGET = typing_engine_test

SRC = \
    main.cpp \
    typing_engine/typing_engine.cpp \
    typing_engine/typing_session.cpp

INC = -Ityping_engine

all: $(TARGET)

$(TARGET):
	$(CXX) $(CXXFLAGS) $(INC) $(SRC) -o $(TARGET)

clean:
	rm -f $(TARGET)
