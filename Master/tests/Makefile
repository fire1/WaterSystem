# Host unit tests for Master firmware (no Arduino toolchain required).
CXX      ?= g++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -I. -lm
.PHONY: all test clean overtime_test moon_test main_test well_topoff_test

all: overtime_tests moon_tests airlift_tests main_tests well_topoff_tests

overtime_tests: test_overtime.cpp TestHarness.h ../lib/Overtime.h
	$(CXX) $(CXXFLAGS) -o $@ test_overtime.cpp

moon_tests: test_moon.cpp TestHarness.h ../lib/Moon.h
	$(CXX) $(CXXFLAGS) -o $@ test_moon.cpp

airlift_tests: test_airlift.cpp TestHarness.h ../lib/AirliftOpt.h
	$(CXX) $(CXXFLAGS) -o $@ test_airlift.cpp

main_tests: test_main.cpp TestHarness.h ../lib/Main.h
	$(CXX) $(CXXFLAGS) -o $@ test_main.cpp

well_topoff_tests: test_well_topoff.cpp TestHarness.h ../lib/WellTopOff.h
	$(CXX) $(CXXFLAGS) -o $@ test_well_topoff.cpp

test: overtime_tests moon_tests airlift_tests main_tests well_topoff_tests
	./overtime_tests
	./moon_tests
	./airlift_tests
	./main_tests
	./well_topoff_tests

clean:
	rm -f overtime_tests moon_tests airlift_tests main_tests well_topoff_tests
