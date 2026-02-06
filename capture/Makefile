TARGET = capture_dis
SRCS := capture_dis.cpp

# 获取 OpenCV 编译选项
OPENCV_FLAGS := $(shell pkg-config --cflags --libs opencv4)

# 增加 -lstdc++ 因为使用了 C++
LDFLAGS := -lspcdev -lm -lstdc++ $(OPENCV_FLAGS)

$(TARGET): $(SRCS)
	$(CXX) -O3 -o $@ $^ $(LDFLAGS)

clean:
	rm -rf $(TARGET)