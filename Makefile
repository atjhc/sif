CC := clang++
LIBNAME := hypertalk.a

DSTROOT := build
SRCROOT := src

TOOLS := $(SRCROOT)/hypertalk.cc $(SRCROOT)/tests.cc
CODEGEN := $(DSTROOT)/yyParser.cc $(DSTROOT)/yyScanner.cc

# Find all .cc files under the SRCROOT directory.
SRC := $(shell find src -name '*.cc' | xargs)

# Filter out tools from the framework.
SRC := $(filter-out $(TOOLS),$(SRC))

# Add the code generation files.
SRC := $(CODEGEN) $(SRC)

# Generate the .o files for the framework.
OBJ := $(patsubst $(SRCROOT)/%.cc,$(DSTROOT)/%.o,$(SRC))
OBJ := $(patsubst $(DSTROOT)/%.cc,$(DSTROOT)/%.o,$(OBJ))

# TODO: I want to get rid of this, but I don't yet know how to
# create a build rule that matches target sub directories.
VPATH := $(SRCROOT)

CPPFLAGS := -I$(DSTROOT) -I$(SRCROOT) -std=c++14 -g -Wno-deprecated-register

all: $(DSTROOT)/$(LIBNAME) $(DSTROOT)/hypertalk

format:
	find src -name '*.cc' -exec clang-format -i --style=file {} \;
	find src -name '*.h' -exec clang-format -i --style=file {} \;

test: $(DSTROOT)/test
	$(DSTROOT)/test $(SRCROOT)/tests

$(DSTROOT):
	mkdir -p $(DSTROOT)

$(DSTROOT)/test: tests.cc $(DSTROOT)/$(LIBNAME)
	$(CC) $(CPPFLAGS) -o $(DSTROOT)/test $< $(DSTROOT)/$(LIBNAME)

$(DSTROOT)/hypertalk: $(SRCROOT)/hypertalk.cc $(DSTROOT)/$(LIBNAME)
	$(CC) $(CPPFLAGS) -o $(DSTROOT)/hypertalk $< $(DSTROOT)/$(LIBNAME)

$(DSTROOT)/$(LIBNAME): $(DSTROOT) $(OBJ)
	ar rc $(DSTROOT)/$(LIBNAME) $(OBJ)
	ranlib $(DSTROOT)/$(LIBNAME)

$(DSTROOT)/yyScanner.cc: $(DSTROOT) $(SRCROOT)/parser/scanner.l
	flex --outfile=$(DSTROOT)/yyScanner.cc --header-file=$(DSTROOT)/yyScanner.h $(SRCROOT)/parser/scanner.l

$(DSTROOT)/yyParser.cc: $(DSTROOT) $(SRCROOT)/parser/parser.y
	bison --output-file=$(DSTROOT)/yyParser.cc --defines=$(DSTROOT)/yyParser.h $(SRCROOT)/parser/parser.y

$(DSTROOT)/%.o: %.cc %.h $(DSTROOT)
	mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) -c -o $@ $<

clean:
	rm -rf $(DSTROOT)

.PHONY: all format test clean