CC := clang-23
CFLAGS := -Wall -Werror -Wextra -std=c23 -Iinclude
LDFLAGS :=

DEBUG_FLAGS := -g -O0

TEST_CFLAGS := $(CFLAGS) -g $(shell pkg-config --cflags criterion)
TEST_LDFLAGS := $(shell pkg-config --libs criterion)

TARGET := bin/interpreter
TEST_TARGET := bin/test_runner

INCLUDES := $(shell find include -name '*.h')

SRC := $(shell find src -name '*.c' ! -name 'main.c')
OBJ := $(SRC:src/%.c=build/%.o)

MAIN_OBJ := build/main.o

TEST_SRC := $(shell find tests -name '*.c')

.PHONY: all build debug clean test doc format check-format check-tidy check

all: clean build

build: $(TARGET)

debug: CFLAGS += $(DEBUG_FLAGS)
debug: $(TARGET)

debug-test: TEST_CFLAGS += $(DEBUG_FLAGS)
debug-test: $(TEST_TARGET)

$(TARGET): $(MAIN_OBJ) $(OBJ)
	@mkdir -p $(@D)
	$(CC) $^ -o $@ $(LDFLAGS)

$(TEST_TARGET): $(TEST_SRC) $(OBJ)
	@mkdir -p $(@D)
	$(CC) $(TEST_CFLAGS) $^ -o $@ $(TEST_LDFLAGS)

test: $(TEST_TARGET)
	./$(TEST_TARGET)

build/%.o: src/%.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

build/main.o: src/main.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

-include $(OBJ:.o=.d)
-include $(MAIN_OBJ:.o=.d)

run: $(TARGET)
	./$(TARGET)

format:
	clang-format-23 -i --Werror $(SRC) $(TEST_SRC) $(INCLUDES)

check-format:
	clang-format-23 --dry-run --Werror $(SRC) $(TEST_SRC) $(INCLUDES)

check-tidy:
	@run-clang-tidy-23 -p .

check: check-tidy check-format

doc:
	doxygen Doxyfile
	sed -i 's/\\usepackage\[T2A\]{fontenc}/\\usepackage[T1]{fontenc}/' doc/gen/latex/refman.tex
	make -C doc/gen/latex
	cp doc/gen/latex/refman.pdf dokumentace.pdf

clean:
	rm -rf build/* bin/* doc/gen/* dokumentace.pdf
