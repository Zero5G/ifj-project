CC ?= clang-22
CFLAGS := -Wall -Wextra -std=c23 -Iinclude
LDFLAGS :=

TARGET := bin/interpreter

SRC := $(shell find src -name '*.c')
OBJ := $(patsubst src/%.c,build/%.o,$(SRC))

.PHONY: all clean doc

all: $(TARGET)

$(TARGET): $(OBJ)
	@mkdir -p $(@D)
	$(CC) $(OBJ) -o $@ $(LDFLAGS)

build/%.o: src/%.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

-include $(OBJ:.o=.d)

doc:
	doxygen Doxyfile

clean:
	rm -rf build/* bin/interpreter doc/* doxygen_warnings.log
