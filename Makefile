CC = gcc
CC_FLAGS = -Wall
NASM = nasm
CC_TEST_FLAGS = -Isrc -Ivendor/acutest/include

all: src/0x864

clean:
	rm -rf src/0x864 src/0x864.o tests/test

test: src/0x864
	@$(CC) $(CC_FLAGS) $(CC_TEST_FLAGS) tests/parser.c src/0x864.o -o tests/test
	@./tests/test

src/0x864: src/harness.c src/0x864.o
	@$(CC) $(CC_FLAGS) src/harness.c src/0x864.o -o src/0x864

src/0x864.o: src/0x864.s
	@$(NASM) -felf64 src/0x864.s -o src/0x864.o

.PHONY: all clean test
