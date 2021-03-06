SRC_DIR=src
OBJ_DIR=obj
INC_DIR=include

SRCS=$(wildcard $(SRC_DIR)/*.c)
OBJS=$(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRCS))
DEPS=$(OBJS:.o=.d)

GENERATED_FILES=$(SRC_DIR)/lexer.c $(SRC_DIR)/parser.c include/y.tab.h
LINT_FILES=$(filter-out ${GENERATED_FILES},$(wildcard $(SRC_DIR)/*.c $(INC_DIR)/*.h))
TEST_RUNNER=test_runner.sh

LEX=flex
YACC=bison -y
YFLAGS=--defines=include/y.tab.h
CC=clang
CFLAGS=-g `llvm-config --cflags` -MD -MP -Wall -Wextra -Wpedantic -Wno-gnu-zero-variadic-macro-arguments -I$(INC_DIR)
LD=clang++
LDFLAGS=`llvm-config --cxxflags --ldflags --libs analysis bitwriter core executionengine interpreter mcjit native passes --system-libs`

.PHONY: all
all: nickel lint test

.PHONY: clean
clean:
	rm -rf nickel ${GENERATED_FILES} $(OBJ_DIR)

.PHONY: test
test: nickel
	./$(TEST_RUNNER)

.PHONY: lint
lint: nickel
	clang-tidy -warnings-as-errors='*' ${LINT_FILES} -- $(CFLAGS)
	flawfinder ${LINT_FILES}
	shellcheck $(TEST_RUNNER)

# Ensure we build parser first, to generate y.tab.h, used by later files
nickel: $(OBJ_DIR)/parser.o $(OBJ_DIR)/lexer.o $(OBJS)
	$(LD) $^ $(LDFLAGS) -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR):
	mkdir $(OBJ_DIR)

-include $(DEPS)
