CXX      := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -O2
SRCS     := main.cpp auth.cpp graph.cpp dispatch.cpp system.cpp

.PHONY: all clean test web
all: ridesync

ridesync: $(SRCS) auth.hpp graph.hpp dispatch.hpp system.hpp
	$(CXX) $(CXXFLAGS) -o $@ $(SRCS)

test_core: test_core.cpp auth.cpp graph.cpp dispatch.cpp system.cpp auth.hpp graph.hpp dispatch.hpp system.hpp
	$(CXX) $(CXXFLAGS) -I. -o $@ test_core.cpp auth.cpp graph.cpp dispatch.cpp system.cpp

test: test_core
	./test_core

web:
	node server.js

clean:
	rm -f ridesync ridesync.exe test_core test_core.exe users.txt bookings.txt
