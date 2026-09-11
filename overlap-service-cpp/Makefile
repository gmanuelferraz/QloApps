# Makefile for QLO-FEAT-007: Overlap Detector Service (overlap-service-cpp)
# Stack: C++17, cpp-httplib, nlohmann/json, doctest, POSIX threads

CXX ?= g++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -O2
CPPFLAGS ?= -I$(INC_DIR) -MMD -MP
LDFLAGS ?=
LDLIBS ?= -lpthread

SRC_DIR ?= src
INC_DIR ?= include
TEST_DIR ?= tests
BUILD_DIR ?= build

TARGET ?= overlap_service
TEST_TARGET ?= test_overlaps

# Third-party header-only dependencies
HTTPLIB_URL ?= https://raw.githubusercontent.com/yhirose/cpp-httplib/dc41dbd9548b4ab2077680ced2c03d4ce962cfa5/httplib.h
JSON_URL ?= https://raw.githubusercontent.com/nlohmann/json/0452641c186bdd64c245cfe57a185dcbcb7a6d30/single_include/nlohmann/json.hpp
DOCTEST_URL ?= https://raw.githubusercontent.com/doctest/doctest/d44d4f6e66232d716af82f00a063759e9d0e50d6/doctest/doctest.h

HTTPLIB_H := $(INC_DIR)/httplib.h
JSON_HPP := $(INC_DIR)/json.hpp
DOCTEST_H := $(INC_DIR)/doctest.h
DEPS_HEADERS := $(HTTPLIB_H) $(JSON_HPP) $(DOCTEST_H)

SRCS := $(wildcard $(SRC_DIR)/*.cpp)
OBJS := $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(SRCS))
DEPS := $(OBJS:.o=.d)

TEST_SRCS := $(wildcard $(TEST_DIR)/*.cpp)
TEST_OBJS := $(patsubst $(TEST_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(TEST_SRCS))
TEST_DEPS := $(TEST_OBJS:.o=.d)
NON_MAIN_OBJS := $(filter-out $(BUILD_DIR)/main.o,$(OBJS))

.PHONY: all clean run test test-unit test-integration help deps clean-deps

all: $(TARGET)

deps: $(DEPS_HEADERS)

$(HTTPLIB_H):
	@mkdir -p $(INC_DIR)
	@echo "Downloading httplib.h..."
	@curl -sSL "$(HTTPLIB_URL)" -o "$@" || wget -q -O "$@" "$(HTTPLIB_URL)"

$(JSON_HPP):
	@mkdir -p $(INC_DIR)
	@echo "Downloading json.hpp..."
	@curl -sSL "$(JSON_URL)" -o "$@" || wget -q -O "$@" "$(JSON_URL)"

$(DOCTEST_H):
	@mkdir -p $(INC_DIR)
	@echo "Downloading doctest.h..."
	@curl -sSL "$(DOCTEST_URL)" -o "$@" || wget -q -O "$@" "$(DOCTEST_URL)"

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) $(LDFLAGS) $(LDLIBS) -o $@

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp | $(BUILD_DIR) $(DEPS_HEADERS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: $(TEST_DIR)/%.cpp | $(BUILD_DIR) $(DEPS_HEADERS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

-include $(DEPS)
-include $(TEST_DEPS)

run: $(TARGET)
	./$(TARGET)

test: $(TARGET)
	@if [ -n "$(TEST_SRCS)" ]; then \
		echo "Building test binary $(TEST_TARGET)..."; \
		$(MAKE) $(TEST_TARGET); \
		echo "Running all tests (unit & integration)..."; \
		./$(TEST_TARGET); \
	else \
		echo "No test sources found in $(TEST_DIR)/. When tests are added, 'make test' will build and run them."; \
	fi

test-unit: $(TARGET)
	@$(MAKE) $(TEST_TARGET)
	@echo "Running unit tests..."; \
	./$(TEST_TARGET) --test-suite=unit

test-integration: $(TARGET)
	@$(MAKE) $(TEST_TARGET)
	@echo "Running integration tests..."; \
	./$(TEST_TARGET) --test-suite=integration

$(TEST_TARGET): $(NON_MAIN_OBJS) $(TEST_OBJS)
	$(CXX) $(CXXFLAGS) $(NON_MAIN_OBJS) $(TEST_OBJS) $(LDFLAGS) $(LDLIBS) -o $@

clean:
	rm -rf $(BUILD_DIR) $(TARGET) $(TEST_TARGET)

clean-deps:
	rm -f $(DEPS_HEADERS)

help:
	@echo "Available targets:"
	@echo "  all              : Compile and build the $(TARGET) binary (default)"
	@echo "  deps             : Download third-party headers ($(DEPS_HEADERS))"
	@echo "  run              : Build and launch the $(TARGET) service"
	@echo "  test             : Build and run all unit and integration tests"
	@echo "  test-unit        : Build and run only unit tests"
	@echo "  test-integration : Build and run only integration tests"
	@echo "  clean            : Remove build artifacts and compiled binaries"
	@echo "  clean-deps       : Remove downloaded third-party headers"
	@echo "  help             : Display this help message"
