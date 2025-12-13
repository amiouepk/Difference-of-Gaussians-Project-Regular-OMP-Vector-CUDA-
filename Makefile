# Makefile for Difference-of-Gaussians project (CPU + CUDA)

# 1. SETUP COMPILERS AND PATHS
CXX       ?= g++
NVCC      ?= nvcc
CUDA_PATH ?= /usr/local/cuda

# 2. FLAGS
# CXXFLAGS: Add -I for CUDA headers so main.cpp can find <cuda_runtime.h>
CXXFLAGS  ?= -std=c++17 -march=native -O3 -Wall -Wextra -fopenmp -I$(CUDA_PATH)/include

# LDFLAGS: Add -L and -l for linking the CUDA runtime library
LDFLAGS   ?= -fopenmp -L$(CUDA_PATH)/lib64 -lcudart

# NVCCFLAGS: Flags specifically for the CUDA compiler
NVCCFLAGS ?= -O3 -std=c++11

RM        ?= rm -f
TARGET    := diff_gauss

# 3. GATHER SOURCE FILES
CPP_SRCS  := $(wildcard *.cpp)
CUDA_SRCS := $(wildcard *.cu)

# 4. CREATE OBJECT LIST
# We need .o files for both .cpp and .cu files
CPP_OBJS  := $(CPP_SRCS:.cpp=.o)
CUDA_OBJS := $(CUDA_SRCS:.cu=.o)
ALL_OBJS  := $(CPP_OBJS) $(CUDA_OBJS)

.PHONY: all release debug run clean

all: release

release: CXXFLAGS += -DNDEBUG
release: $(TARGET)

debug: CXXFLAGS += -g -O0
debug: NVCCFLAGS += -g -G
debug: $(TARGET)

# 5. LINKING STEP
# Links both C++ objects and CUDA objects into one executable
$(TARGET): $(ALL_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

# 6. COMPILATION RULES
# Rule for C++ files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Rule for CUDA files (Compiles .cu to .o)
%.o: %.cu
	$(NVCC) $(NVCCFLAGS) -c $< -o $@

run: $(TARGET)
	./$(TARGET)

clean:
	$(RM) $(TARGET) $(ALL_OBJS)