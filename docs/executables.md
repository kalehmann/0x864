# Creating executables

0x864 cannot create an ELF executable directly.
Instead, a file with a global label `_start` as entrypoint can be created like

```asm
        global _start
_start:
        ;; Syscall - exit
        mov eax, 60
        mov edi, 0
        syscall
```

then afterwards be assembled to an ELF relocatable file with

```
0x864 -o my-prog.o -felf64 my-prog.s
```

and finally be linked as an ELF executable with

```
ld -m elf_x86_64 -o my-prog my-prog.o
```

