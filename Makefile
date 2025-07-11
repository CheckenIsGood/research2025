CXX = g++
CXXFLAGS = -std=c++17 -O2

# Set this to the path of your extracted LibTorch directory
LIBTORCH_PATH = /mnt/c/Users/Stephen/libtorch-cxx11-abi-shared-with-deps-2.7.1+cu118/libtorch

INCLUDES = -I$(LIBTORCH_PATH)/include -I$(LIBTORCH_PATH)/include/torch/csrc/api/include
LDFLAGS = -L$(LIBTORCH_PATH)/lib
LDLIBS = -ltorch -lc10_cuda -ltorch_cuda -ltorch_cpu -lc10 -Wl,-rpath,$(LIBTORCH_PATH)/lib

# Targets
MODEL_TARGET = simple_resnet
DEBUG_TARGET = debug_driver

# Source files
MODEL_SRC = model.cpp
DEBUG_SRC = debug_driver.cc weight_api.cc

all: $(MODEL_TARGET) $(DEBUG_TARGET)

$(MODEL_TARGET): $(MODEL_SRC)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $^ -o $@ $(LDFLAGS) $(LDLIBS)

$(DEBUG_TARGET): $(DEBUG_SRC)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $^ -o $@ $(LDFLAGS) $(LDLIBS)

clean:
	rm -f $(MODEL_TARGET) $(DEBUG_TARGET)
