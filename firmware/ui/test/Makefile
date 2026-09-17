CXX ?= c++
CXXFLAGS ?= -std=c++20 -Wall -Wextra -Werror -pedantic -O2

BUILD_DIR := build
INCLUDES := -I../inc
GENERATOR_SOURCE := ../src/wave_signal_generator.cc
TEST_BINARY := $(BUILD_DIR)/wave_signal_generator_test
CSV_BINARY := $(BUILD_DIR)/generate_wave_csv

.PHONY: all test wave_csv clean

all: test $(CSV_BINARY)

test: $(TEST_BINARY)
	$(TEST_BINARY)

wave_csv: $(CSV_BINARY)
	$(CSV_BINARY) $(ARGS)

$(TEST_BINARY): wave_signal_generator_test.cc $(GENERATOR_SOURCE) ../inc/wave_signal_generator.hh
	mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(INCLUDES) wave_signal_generator_test.cc $(GENERATOR_SOURCE) -o $@

$(CSV_BINARY): generate_wave_csv.cc $(GENERATOR_SOURCE) ../inc/wave_signal_generator.hh
	mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(INCLUDES) generate_wave_csv.cc $(GENERATOR_SOURCE) -o $@

clean:
	rm -rf $(BUILD_DIR)
