# Makefile for ./search
#   make        build the program
#   make clean  remove the build files and the program

# Compiler and flags. -Iinclude finds our headers; pkg-config adds the OpenCV include path.
# -O3 -march=native -fopenmp-simd let the distance loops use the CPU's SIMD instructions
# (only the "omp simd" pragma is used, not the OpenMP runtime). -pthread for std::thread.
CXX      := g++
CXXFLAGS := -std=c++17 -O3 -march=native -fopenmp-simd -pthread -Wall -Wextra -Iinclude \
            $(shell pkg-config --cflags opencv4)
# Only the OpenCV modules we use: core, image reading, image processing, SIFT (features2d).
LDLIBS   := -lopencv_core -lopencv_imgcodecs -lopencv_imgproc -lopencv_features2d

# Every src/X.cpp is compiled to build/X.o; .d files track header dependencies.
SRC    := $(wildcard src/*.cpp)
OBJ    := $(SRC:src/%.cpp=build/%.o)
DEP    := $(OBJ:.o=.d)
TARGET := search

all: $(TARGET)

# Link all object files into the program.
$(TARGET): $(OBJ)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDLIBS)

# Compile one source file. -MMD -MP write build/X.d, so editing a header rebuilds
# the files that include it.
build/%.o: src/%.cpp | build
	$(CXX) $(CXXFLAGS) -MMD -MP -c $< -o $@

build:
	mkdir -p build

clean:
	rm -rf build $(TARGET)

-include $(DEP)

.PHONY: all clean
