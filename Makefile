CC=clang
CFLAGS=-g `llvm-config --cflags` -Wall -Wextra -Wpedantic -Wno-gnu-zero-variadic-macro-arguments
LD=clang++
LDFLAGS=`llvm-config --cxxflags --ldflags --libs analysis bitwriter core executionengine interpreter mcjit native passes --system-libs`

all: nickel lint test

test: nickel
	./test_runner.sh

GENERATED_FILES=lex.yy.c parser.tab.c
LINT_FILES=$(filter-out ${GENERATED_FILES},$(wildcard *.c *.h))

lint: nickel
	clang-tidy ${LINT_FILES} -- $(CFLAGS)
	flawfinder ${LINT_FILES}

nickel: nickel.o jit.o interpreter.o parser.tab.o parser_helpers.o lex.yy.o helpers.o
	$(LD) $^ $(LDFLAGS) -o $@

parser.tab.c parser.tab.h: parser.y parser_helpers.h
	bison -d parser.y

lex.yy.c: lexer.l parser.tab.h
	flex lexer.l

%.o: %.c
	$(CC) $(CFLAGS) -c $<

nickel.o: nickel.c parser.tab.h interpreter.h jit.h
	$(CC) $(CFLAGS) -c $<

parser_helpers.o: parser_helpers.c interpreter.h parser_helpers.h
	$(CC) $(CFLAGS) -c $<

interpreter.o: interpreter.c interpreter.h helpers.h syntax.h
	$(CC) $(CFLAGS) -c $<

jit.o: jit.c jit.h interpreter.h helpers.h syntax.h
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f nickel ${GENERATED_FILES} parser.tab.h parser.output *.o
