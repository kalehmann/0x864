NASM = nasm

all: src/0x864.o

clean:
	rm -rf src/0x864.o

src/0x864.o: src/0x864.s
	@$(NASM) -felf64 src/0x864.s -o src/0x864.o

.PHONY: all clean
