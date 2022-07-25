ifeq ($(CPP),true)
	CFLAGS := -std=c++11
	C_LANG := -x c++
else
	CFLAGS := -std=c99
endif

# Mode configuration.
ifeq ($(MODE),debug)
	CFLAGS += -O0 -DDEBUG -g
	BUILD_DIR := build/debug
else
	CFLAGS += -O3 -flto
	BUILD_DIR := build/release
endif

CFLAGS += -Wall -Wextra -Werror -Wno-unused-parameter

TARGET_EXEC := dojo

BUILD_DIR := ./build
SRC_DIRS := ./src

# Find all the C and C++ files we want to compile
# Note the single quotes around the * expressions. Make will incorrectly expand these otherwise.
SRCS := $(shell find $(SRC_DIRS) -name '*.cpp' -or -name '*.c' -or -name '*.s')

# String substitution for every C/C++ file.
# As an example, hello.cpp turns into ./build/hello.cpp.o
OBJS := $(SRCS:%=$(BUILD_DIR)/%.o)

# String substitution (suffix version without %).
# As an example, ./build/hello.cpp.o turns into ./build/hello.cpp.d
DEPS := $(OBJS:.o=.d)

# Every folder in ./src will need to be passed to GCC so that it can find header files
INC_DIRS := $(shell find $(SRC_DIRS) -type d)
# Add a prefix to INC_DIRS. So moduleA would become -ImoduleA. GCC understands this -I flag
INC_FLAGS := $(addprefix -I,$(INC_DIRS))

# The -MMD and -MP flags together generate Makefiles for us!
# These files will have .d instead of .o as the output.
CPPFLAGS := $(INC_FLAGS) -MMD -MP

.PHONY: build

build: $(BUILD_DIR)/$(TARGET_EXEC)

# The final build step.
$(BUILD_DIR)/$(TARGET_EXEC): $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

# Build step for C source
$(BUILD_DIR)/%.c.o: %.c
	mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

# Build step for C++ source
$(BUILD_DIR)/%.cpp.o: %.cpp
	mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

.PHONY: test

TESTS_DIR := ./tests

TESTS := $(shell find $(TESTS_DIR) -name '*.cpp' -or -name '*.c')

TESTS_OBJ := $(TESTS:%=$(BUILD_DIR)/%.o)

TESTS_EXEC := $(TESTS:$(TESTS_DIR)/%.c=$(BUILD_DIR)/%)

TESTFLAGS = -lcheck -lm -lpthread -lrt -lsubunit

test: $(TESTS_EXEC)
	$(TESTS_EXEC)


$(TESTS_EXEC): $(TESTS_OBJ) $(OBJS)
	@mkdir -p $(dir $@)
	@echo [TEST EXEC] $(TESTS)
	@echo [BUILDING TEST EXEC] $@
	@echo [TEST EXEC DIR] $(@D)
	@echo [CURR DIR] $(shell pwd)
	$(CC) $(@:build/check_%=build/src/%.c.o) $(@:build%=build/tests/%.c.o) $(CFLAGS) $(CPPFLAGS) $(TESTFLAGS) -o $@

$(TESTS_OBJ):
	@mkdir -p $(dir $@)
	@echo [BUILDING TESTS OBJECTES] $@ 
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $(@:build/%.o=%) -o $@

.PHONY: clean
clean:
	rm -r $(BUILD_DIR)

# Include the .d makefiles. The - at the front suppresses the errors of missing
# Makefiles. Initially, all the .d files will be missing, and we don't want those
# errors to show up.
-include $(DEPS)