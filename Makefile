CC := clang-23
CFLAGS := -Wall -Werror -Wextra -std=c23 -Isrc
LDFLAGS :=

DEBUG_FLAGS := -g -O0

TEST_CFLAGS := $(CFLAGS) -g $(shell pkg-config --cflags criterion)
TEST_LDFLAGS := $(shell pkg-config --libs criterion)

TARGET := compiler
TEST_TARGET := test_runner

OBJ_DIR := obj

INCLUDES := $(wildcard src/*.h)
SRC := $(wildcard src/*.c)
TEST_SRC := $(wildcard tests/*.c)

OBJ := $(patsubst src/%.c,$(OBJ_DIR)/%.o,$(SRC))
TEST_OBJ := $(patsubst tests/%.c,$(OBJ_DIR)/tests/%.o,$(TEST_SRC))

ARCHIVE_TARGET := xmanouj00.zip
DOC_TARGET := dokumentace.pdf

.PHONY: all build debug debug-test test run doc format check-format check-tidy check clean

all: clean build

build: $(TARGET)

debug: CFLAGS += $(DEBUG_FLAGS)
debug: $(TARGET)

debug-test: TEST_CFLAGS += $(DEBUG_FLAGS)
debug-test: $(TEST_TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

$(TEST_TARGET): $(TEST_OBJ) $(filter-out $(OBJ_DIR)/main.o,$(OBJ))
	$(CC) $(TEST_CFLAGS) $^ -o $@ $(TEST_LDFLAGS)

$(OBJ_DIR)/%.o: src/%.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/tests/%.o: tests/%.c
	@mkdir -p $(@D)
	$(CC) $(TEST_CFLAGS) -c $< -o $@

test: $(TARGET) $(TEST_TARGET)
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

build-check: check build

# POZOR pdflatex je nastaven aby chyby byly přeskočené, takže spadnou do exit kódu, který ignorujeme
doxygen:
	doxygen Doxyfile
	sed -i 's/\\usepackage\[T2A\]{fontenc}/\\usepackage[T1]{fontenc}/' doc/gen/latex/refman.tex
	cp -r doc/assets/. doc/gen/latex/assets/
	make LATEX_CMD="pdflatex -interaction=nonstopmode" -C doc/gen/latex || true
	cp doc/gen/latex/refman.pdf refman.pdf

doc:
	quarto render doc/pages/doc.qmd
	cp doc/pages/$(DOC_TARGET) ./$(DOC_TARGET)

pack:
	zip -r $(ARCHIVE_TARGET) \
		src/ Makefile rozdeleni rozsireni $(DOC_TARGET)

clean:
	rm -rf $(OBJ_DIR) test_runner interpreter doc/gen/* $(ARCHIVE_TARGET)

is_it_ok: pack
	cp $(ARCHIVE_TARGET) is_it_ok/
	cd is_it_ok/ && ./is_it_ok.sh $(ARCHIVE_TARGET)
