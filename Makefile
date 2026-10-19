CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -I.
SRCS = main.cpp src/SpatialGrid.cpp src/Dungeon.cpp
OBJS = $(SRCS:.cpp=.o)
TARGET = engine.exe

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	del /Q $(TARGET) *.o src\*.o 2>nul || exit 0
