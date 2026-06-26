# Host unit tests for Master firmware (no Arduino toolchain required).
CXX      ?= g++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -I. -lm
.PHONY: all test clean overtime_test moon_test

all: overtime_tests moon_tests

overtime_tests: test_overtime.cpp TestHarness.h ../lib/Overtime.h
	$(CXX) $(CXXFLAGS) -o $@ test_overtime.cpp

moon_tests: test_moon.cpp TestHarness.h ../lib/Moon.h
	$(CXX) $(CXXFLAGS) -o $@ test_moon.cpp

test: overtime_tests moon_tests
	./overtime_tests
	./moon_tests

clean:
	rm -f overtime_tests moon_tests
