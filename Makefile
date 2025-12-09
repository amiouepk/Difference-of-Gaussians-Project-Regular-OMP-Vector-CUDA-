# Makefile for Difference-of-Gaussians project
# - Builds all .cpp files in the project directory
# - Targets: `all` (default), `release`, `debug`, `run`, `clean`

CXX ?= g++
CXXFLAGS ?= -std=c++17 -march=native -O3 -Wall -Wextra -fopenmp
LDFLAGS ?= -fopenmp
RM ?= rm -f

SRCS := $(wildcard *.cpp)
OBJS := $(SRCS:.cpp=.o)
TARGET := diff_gauss

.PHONY: all release debug run clean

all: release

release: CXXFLAGS += -O3 -DNDEBUG
release: $(TARGET)

debug: CXXFLAGS += -g -O0
debug: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

run: $(TARGET)
	./$(TARGET)

clean:
	$(RM) $(TARGET) $(OBJS)

# Optional CUDA support: builds any .cu files with nvcc if present
CUDA_SRCS := $(wildcard *.cu)
ifneq ($(CUDA_SRCS),)
NVCC ?= nvcc
CUDA_TARGETS := $(CUDA_SRCS:.cu=)
.PHONY: cuda
cuda:
	@echo "Building CUDA sources: $(CUDA_SRCS)"
	$(NVCC) -O3 -std=c++11 -o cuda_build $(CUDA_SRCS)
endif

