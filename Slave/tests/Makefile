# Host unit tests for Slave firmware (no Arduino toolchain required).
CXX      ?= g++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -I. -lm
.PHONY: all test clean

all: slave_tests

slave_tests: test_slave.cpp TestHarness.h ../lib/SlaveLogic.h
	$(CXX) $(CXXFLAGS) -o $@ test_slave.cpp

test: slave_tests
	./slave_tests

clean:
	rm -f slave_tests
