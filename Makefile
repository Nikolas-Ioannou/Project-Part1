CXX      := g++
CXXFLAGS := -std=c++17 -O2 -Wall -Wextra -Iinclude $(shell pkg-config --cflags opencv4)
LDLIBS   := -lopencv_core -lopencv_imgcodecs -lopencv_imgproc -lopencv_features2d

SRC    := $(wildcard src/*.cpp)
OBJ    := $(SRC:src/%.cpp=build/%.o)
DEP    := $(OBJ:.o=.d)
TARGET := search

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDLIBS)

build/%.o: src/%.cpp | build
	$(CXX) $(CXXFLAGS) -MMD -MP -c $< -o $@

build:
	mkdir -p build

clean:
	rm -rf build $(TARGET)

-include $(DEP)

.PHONY: all clean
