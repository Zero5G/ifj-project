CC := clang-23
CFLAGS := -Wall -Werror -Wextra -std=c23 -Isrc
LDFLAGS :=

DEBUG_FLAGS := -g -O0

TEST_CFLAGS := $(CFLAGS) -g $(shell pkg-config --cflags criterion)
TEST_LDFLAGS := $(shell pkg-config --libs criterion)

TARGET := interpreter
TEST_TARGET := test_runner

INCLUDES := $(wildcard src/*.h)
SRC := $(wildcard src/*.c)
TEST_SRC := $(wildcard tests/*.c)

.PHONY: all build debug debug-test test run doc format check-format check-tidy check clean

all: clean build

build: $(TARGET)

debug: CFLAGS += $(DEBUG_FLAGS)
debug: $(TARGET)

debug-test: TEST_CFLAGS += $(DEBUG_FLAGS)
debug-test: $(TEST_TARGET)

$(TARGET): $(SRC)
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

$(TEST_TARGET): $(TEST_SRC)
	@mkdir -p $(@D)
	$(CC) $(TEST_CFLAGS) $^ $(filter-out src/main.c,$(SRC)) -o $@ $(TEST_LDFLAGS)

test: $(TEST_TARGET)
	./$(TEST_TARGET)

run: $(TARGET)
	./$(TARGET)

format:
	clang-format-23 -i $(SRC) $(TEST_SRC) $(INCLUDES)

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
	rm -rf bin/* doc/gen/* dokumentace.pdf
