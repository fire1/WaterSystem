# Host unit tests for Master firmware (no Arduino toolchain required).
CXX      ?= g++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -I.
TARGET   := overtime_tests
SRC      := test_overtime.cpp

.PHONY: all test clean

all: $(TARGET)

$(TARGET): $(SRC) TestHarness.h ../lib/Overtime.h
	$(CXX) $(CXXFLAGS) -o $@ $(SRC)

test: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)
