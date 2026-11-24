CC ?= gcc
CFLAGS ?= -std=c11 -Wall -Wextra -Werror -O2
LDLIBS ?= -lm

TARGET = entropy
SOURCES = main.c bitio.c
HEADERS = bitio.h

.PHONY: all test clean memcheck

all: $(TARGET)

$(TARGET): $(SOURCES) $(HEADERS)
	$(CC) $(CFLAGS) $(SOURCES) -o $@ $(LDLIBS)

# The script builds its own isolated test binary, so it also works when run directly.
test:
	bash tests/run.sh

memcheck: all
	@if command -v valgrind >/dev/null 2>&1; then \
		valgrind --leak-check=full --show-leak-kinds=all --error-exitcode=1 ./$(TARGET) tests/tekst.txt; \
	else \
		echo "Valgrind nie jest zainstalowany – pominięto kontrolę pamięci."; \
	fi

clean:
	rm -f $(TARGET) $(TARGET).exe tests/test_bitio tests/test_bitio.exe tests/test_bitio_tmp.bin
