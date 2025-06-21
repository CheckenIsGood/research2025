CXX = g++
CXXFLAGS = -std=c++17 -O2

# Set this to the path of your extracted LibTorch directory
LIBTORCH_PATH = /mnt/c/Users/Stephen/libtorch-cxx11-abi-shared-with-deps-2.7.1+cu118/libtorch

INCLUDES = -I$(LIBTORCH_PATH)/include -I$(LIBTORCH_PATH)/include/torch/csrc/api/include
LDFLAGS = -L$(LIBTORCH_PATH)/lib
LDLIBS = -ltorch -ltorch_cpu -lc10 -Wl,-rpath,$(LIBTORCH_PATH)/lib

# Target executable
TARGET = simple_resnet
SRC = model.cpp

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $^ -o $@ $(LDFLAGS) $(LDLIBS)

clean:
	rm -f $(TARGET)