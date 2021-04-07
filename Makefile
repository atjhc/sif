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
CODEGEN := $(DSTROOT)/yyParser.cc $(DSTROOT)/yyScanner.cc

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

# Add the code generation files.
SRC := $(CODEGEN) $(SRC)

# Generate the .o files for the framework.
OBJ := $(patsubst $(SRCROOT)/%.cc,$(DSTROOT)/%.o,$(SRC))
OBJ := $(patsubst $(DSTROOT)/%.cc,$(DSTROOT)/%.o,$(OBJ))

# TODO: I want to get rid of this, but I don't yet know how to
# create a build rule that matches target sub directories.
VPATH := $(SRCROOT)

# TODO: Remove exceptions for implicit-function-declaration, unused-function,
# and no-register. I need these for the time being because they flag generated code.
WNO := -Wno-unneeded-internal-declaration -Wno-unused-function -Wno-register

CPPFLAGS := -I$(DSTROOT) -I$(SRCROOT) -Wall -Werror $(WNO) -std=c++17

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

install-support:
	cp Chatter.sublime-syntax "$(HOME)/Library/Application Support/Sublime Text 3/Packages/User"

format:
	find src -name '*.cc' -exec clang-format -i --style=file {} \;
	find src -name '*.h' -exec clang-format -i --style=file {} \;

dstroot:
	mkdir -p $(DSTROOT)

$(DSTROOT)/test: $(DSTROOT)/$(LIBNAME) $(TEST_OBJ) $(SRCROOT)/tools/tests.cc
	$(CC) $(CPPFLAGS) -g -o $(DSTROOT)/test $(SRCROOT)/tools/tests.cc $(TEST_OBJ) $(DSTROOT)/$(LIBNAME)

$(DSTROOT)/$(TOOLNAME): $(SRCROOT)/tools/chatter.cc $(DSTROOT)/$(LIBNAME)
	$(CC) $(CPPFLAGS) -o $(DSTROOT)/$(TOOLNAME) $< $(DSTROOT)/$(LIBNAME)

$(DSTROOT)/$(LIBNAME): $(OBJ)
	ar rc $(DSTROOT)/$(LIBNAME) $(OBJ)
	ranlib $(DSTROOT)/$(LIBNAME)

$(DSTROOT)/yyScanner.cc: $(SRCROOT)/parser/yy_scanner.l
	@mkdir -p $(dir $@)
	$(FLEX) --outfile=$(DSTROOT)/yyScanner.cc --header-file=$(DSTROOT)/yyScanner.h $(SRCROOT)/parser/yy_scanner.l

$(DSTROOT)/yyParser.cc: $(SRCROOT)/parser/yy_parser.y
	@mkdir -p $(dir $@)
	$(BISON) --output-file=$(DSTROOT)/yyParser.cc --defines=$(DSTROOT)/yyParser.h $(BISONFLAGS) $(SRCROOT)/parser/yy_parser.y

$(DSTROOT)/tests/%.o: $(SRCROOT)/tests/%.cc $(COMMON_HEADERS)
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) -c -o $@ $<

$(DSTROOT)/%.o: %.cc %.h $(COMMON_HEADERS)
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) -c -o $@ $<

clean:
	rm -rf $(DSTROOT)

.PHONY: all format test clean dstroot install install-support
