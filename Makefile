CC := clang++
# BISON := bison
# FLEX := flex
BISON := /usr/local/opt/bison/bin/bison
FLEX := /usr/local/opt/flex/bin/flex

LIBNAME := chatter.a
TOOLNAME := chatter
INSTALL_DIR := /usr/local/bin

DSTROOT := build
SRCROOT := src

TOOLS := $(SRCROOT)/tools/chatter.cc $(SRCROOT)/tools/tests.cc

COMMON_HEADERS := $(SRCROOT)/Common.h $(SRCROOT)/Utilities.h

# Find all .cc files under the SRCROOT directory.
SRC := $(shell find src -name '*.cc' | xargs)

# Filter out tools from the framework.
SRC := $(filter-out $(TOOLS),$(SRC))

# Find all test case files.
TEST_SRC := $(filter $(SRCROOT)/tests/%,$(SRC))

# Generate .o files for test suite.
TEST_OBJ := $(patsubst $(SRCROOT)/%.cc,$(DSTROOT)/%.o,$(TEST_SRC))

# Filter out test cases from framework.
SRC := $(filter-out $(TEST_SRC),$(SRC))

# Generate the .o files for the framework.
OBJ := $(patsubst $(SRCROOT)/%.cc,$(DSTROOT)/%.o,$(SRC))
OBJ := $(patsubst $(DSTROOT)/%.cc,$(DSTROOT)/%.o,$(OBJ))

# TODO: I want to get rid of this, but I don't yet know how to
# create a build rule that matches target sub directories.
VPATH := $(SRCROOT)

CPPFLAGS := -I$(DSTROOT) -I$(SRCROOT) -Wall -Werror -std=c++17

all: dstroot $(DSTROOT)/$(LIBNAME) $(DSTROOT)/$(TOOLNAME)

debug: CPPFLAGS += -g -DYYDEBUG=1 -DDEBUG=1
debug: all

test: CPPFLAGS += -g -DYYDEBUG=1 -DDEBUG=1
test: $(DSTROOT)/test $(TEST_OBJ)
	$(DSTROOT)/test $(SRCROOT)/tests

install: all
	mkdir -p $(INSTALL_DIR)
	cp $(DSTROOT)/$(TOOLNAME) $(INSTALL_DIR)
	chmod +x $(INSTALL_DIR)/$(TOOLNAME)

format:
	find src -name '*.cc' -exec clang-format -i --style=file {} \;
	find src -name '*.h' -exec clang-format -i --style=file {} \;

dstroot:
	mkdir -p $(DSTROOT)

$(DSTROOT)/test: $(DSTROOT)/$(LIBNAME) $(TEST_OBJ) $(SRCROOT)/tools/tests.cc
	$(CC) $(CPPFLAGS) -o $(DSTROOT)/test $(SRCROOT)/tools/tests.cc $(TEST_OBJ) $(DSTROOT)/$(LIBNAME)

$(DSTROOT)/$(TOOLNAME): $(DSTROOT)/$(LIBNAME) $(SRCROOT)/tools/chatter.cc
	$(CC) $(CPPFLAGS) -o $(DSTROOT)/$(TOOLNAME) $(SRCROOT)/tools/chatter.cc $(DSTROOT)/$(LIBNAME)

$(DSTROOT)/$(LIBNAME): $(OBJ)
	ar rc $(DSTROOT)/$(LIBNAME) $(OBJ)
	ranlib $(DSTROOT)/$(LIBNAME)

$(DSTROOT)/tests/%.o: $(SRCROOT)/tests/%.cc $(COMMON_HEADERS)
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) -c -o $@ $<

$(DSTROOT)/%.o: %.cc %.h $(COMMON_HEADERS)
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) -c -o $@ $<

clean:
	rm -rf $(DSTROOT)

.PHONY: all format test clean dstroot install install-support
