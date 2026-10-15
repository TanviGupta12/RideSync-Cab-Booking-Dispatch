# Compiler and flags
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2

# Target executable name
TARGET = cab_booking

# Source and Header files
SRCS = main.cpp CityGraph.cpp auth.cpp
OBJS = $(SRCS:.cpp=.o)
HEADERS = CityGraph.hpp auth.hpp

# Default rule
all: $(TARGET)

# Link object files to create the executable
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

# Compile source files into object files
%.o: %.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean up build artifacts
clean:
	rm -f $(OBJS) $(TARGET)

# Build and run the program
run: $(TARGET)
	./$(TARGET)

.PHONY: all clean run
