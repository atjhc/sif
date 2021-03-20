CC = clang++
LIBNAME = hypertalk.a

BUILD = build
SRC = src
OBJECTS =  Parser.o Scanner.o ast/Expressions.o ast/Statements.o ast/Handlers.o

VPATH = src
OFILES = $(OBJECTS:%.o=$(BUILD)/%.o)
CPPFLAGS = -I$(BUILD) -I./ -I$(SRC) -std=c++11 -g -Wno-deprecated-register

all: $(BUILD)/$(LIBNAME) $(BUILD)/tests $(BUILD)/hypertalk

format:
	clang-format -i --style=file *.c *.h

tests: $(BUILD)/tests
	$(BUILD)/tests $(SRC)/tests

$(BUILD):
	mkdir -p $(BUILD)

$(BUILD)/tests: tests.cc $(BUILD)/$(LIBNAME)
	$(CC) $(CPPFLAGS) -o $(BUILD)/tests $< $(BUILD)/$(LIBNAME)

$(BUILD)/hypertalk: hypertalk.cc $(BUILD)/$(LIBNAME)
	$(CC) $(CPPFLAGS) -o $(BUILD)/hypertalk $< $(BUILD)/$(LIBNAME)

$(BUILD)/$(LIBNAME): $(BUILD) $(OFILES)
	ar rc $(BUILD)/$(LIBNAME) $(OFILES)
	ranlib $(BUILD)/$(LIBNAME)

$(BUILD)/Scanner.cc: $(BUILD) $(SRC)/scanner.l
	flex --outfile=$(BUILD)/Scanner.cc --header-file=$(BUILD)/Scanner.h $(SRC)/scanner.l

$(BUILD)/Parser.cc: $(BUILD) $(SRC)/parser.y
	bison --output-file=$(BUILD)/Parser.cc --defines=$(BUILD)/Parser.h $(SRC)/parser.y

$(BUILD)/%.o: %.cc %.h $(BUILD)
	mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) -c -o $@ $<

clean:
	rm -rf $(BUILD)

.PHONY: all format clean