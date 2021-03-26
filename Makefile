CC := clang++
LIBNAME := chatter.a
TOOLNAME := chatter
INSTALL_DIR := /usr/local/bin

DSTROOT := build
SRCROOT := src

TOOLS := $(SRCROOT)/chatter.cc $(SRCROOT)/tests.cc
CODEGEN := $(DSTROOT)/yyParser.cc $(DSTROOT)/yyScanner.cc

COMMON_HEADERS := $(SRCROOT)/Common.h $(SRCROOT)/Utilities.h

# Find all .cc files under the SRCROOT directory.
SRC := $(shell find src -name '*.cc' | xargs)

# Filter out tools from the framework.
SRC := $(filter-out $(TOOLS),$(SRC))

TEST_SRC := $(filter $(SRCROOT)/tests/%,$(SRC))
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
# debug: BISONFLAGS += --debug
debug: all

install: all
	mkdir -p $(INSTALL_DIR)
	cp $(DSTROOT)/$(TOOLNAME) $(INSTALL_DIR)
	chmod +x $(INSTALL_DIR)/$(TOOLNAME)

format:
	find src -name '*.cc' -exec clang-format -i --style=file {} \;
	find src -name '*.h' -exec clang-format -i --style=file {} \;

test: $(DSTROOT)/test
	$(DSTROOT)/test $(SRCROOT)/tests

dstroot:
	mkdir -p $(DSTROOT)

$(DSTROOT)/test: tests.cc $(TEST_SRC) $(DSTROOT)/$(LIBNAME)
	$(CC) $(CPPFLAGS) -o $(DSTROOT)/test $< $(DSTROOT)/$(LIBNAME)

$(DSTROOT)/$(TOOLNAME): $(SRCROOT)/chatter.cc $(DSTROOT)/$(LIBNAME)
	$(CC) $(CPPFLAGS) -o $(DSTROOT)/$(TOOLNAME) $< $(DSTROOT)/$(LIBNAME)

$(DSTROOT)/$(LIBNAME): $(OBJ)
	ar rc $(DSTROOT)/$(LIBNAME) $(OBJ)
	ranlib $(DSTROOT)/$(LIBNAME)

$(DSTROOT)/yyScanner.cc: $(SRCROOT)/parser/scanner.l
	@mkdir -p $(dir $@)
	flex --outfile=$(DSTROOT)/yyScanner.cc --header-file=$(DSTROOT)/yyScanner.h $(SRCROOT)/parser/scanner.l

$(DSTROOT)/yyParser.cc: $(SRCROOT)/parser/parser.y
	@mkdir -p $(dir $@)
	bison --output-file=$(DSTROOT)/yyParser.cc --defines=$(DSTROOT)/yyParser.h $(BISONFLAGS) $(SRCROOT)/parser/parser.y

$(DSTROOT)/%.o: %.cc %.h $(COMMON_HEADERS)
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) -c -o $@ $<

clean:
	rm -rf $(DSTROOT)

.PHONY: all format test clean dstroot