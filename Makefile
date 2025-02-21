CC := clang++

LIBNAME := sif.a
TOOLNAME := sif
INSTALL_DIR := /usr/local/bin

DSTROOT := build
SRCROOT := src
FUZZROOT := fuzzer

TOOLS := $(SRCROOT)/tools/sif.cc $(SRCROOT)/tools/tests.cc

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

CPPFLAGS := -I$(DSTROOT) -I$(SRCROOT) -Wall -Werror -std=c++20 -stdlib=libc++

EXAMPLES := $(shell find examples -name '*.sif')

all: dstroot $(DSTROOT)/$(LIBNAME) $(DSTROOT)/$(TOOLNAME)

release: CPPFLAGS += -O3
release: all

debug: CPPFLAGS += -g -DDEBUG=1
debug: all

test: CPPFLAGS += -g -DDEBUG=1
test: $(DSTROOT)/test $(TEST_OBJ) examples
	$(DSTROOT)/test $(SRCROOT)/tests
	$(SRCROOT)/tests/repl_tests.sh

fuzz: CPPFLAGS += -g -fsanitize=fuzzer,address -DFUZZER -Wno-unused-function -Wno-unused-variable
fuzz: $(DSTROOT)/fuzz_test
	mkdir -p $(FUZZROOT)
	cp -r $(SRCROOT)/tests/transcripts $(FUZZROOT)/corpus
	ASAN_OPTIONS=detect_container_overflow=0 LLVMFuzzer_DATA_PATH=$(FUZZROOT) build/fuzz_test -artifact_prefix=$(FUZZROOT)/ $(FUZZROOT)/corpus

.PHONY: examples $(EXAMPLES)

examples: $(EXAMPLES)
$(EXAMPLES): $(DSTROOT)/$(TOOLNAME)
	$(DSTROOT)/$(TOOLNAME) -b $@ >/dev/null

install: all
	mkdir -p $(INSTALL_DIR)
	cp $(DSTROOT)/$(TOOLNAME) $(INSTALL_DIR)
	chmod +x $(INSTALL_DIR)/$(TOOLNAME)

format:
	find src -type f -name '*.cc' -not -path 'src/tests/*' -not -path 'src/vendor/*' -exec clang-format -i --style=file {} \;
	find src -type f -name '*.h' -not -path 'src/tests/*' -not -path 'src/vendor/*' -exec clang-format -i --style=file {} \;

dstroot:
	mkdir -p $(DSTROOT)

$(DSTROOT)/test: $(DSTROOT)/$(LIBNAME) $(TEST_OBJ) $(SRCROOT)/tools/tests.cc
	$(CC) $(CPPFLAGS) -o $(DSTROOT)/test $(SRCROOT)/tools/tests.cc $(TEST_OBJ) $(DSTROOT)/$(LIBNAME)

$(DSTROOT)/fuzz_test: $(DSTROOT)/$(LIBNAME) $(SRCROOT)/tools/sif.cc
	$(CC) $(CPPFLAGS) -o $(DSTROOT)/fuzz_test $(SRCROOT)/tools/sif.cc $(DSTROOT)/$(LIBNAME)

$(DSTROOT)/$(TOOLNAME): $(DSTROOT)/$(LIBNAME) $(SRCROOT)/tools/sif.cc
	$(CC) $(CPPFLAGS) -o $(DSTROOT)/$(TOOLNAME) $(SRCROOT)/tools/sif.cc $(DSTROOT)/$(LIBNAME)

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
	rm -rf $(FUZZROOT)

.PHONY: all format test clean dstroot install install-support
