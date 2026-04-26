## 0x864 - selfh0sted.x86.4ssembler

### Milestones

- 2234b86bbc: Assembled a relocatable elf file for hello world.

### Supported instructions

| Instruction | Notes                                                 |
|-------------|-------------------------------------------------------|
| `call`      | Only `CALL rel32` to a label                          |
| `dec`       | Only registers supported as destination operand       |
| `inc`       | Only registers supported as destination operand       |
| `int`       |                                                       |
| `lea`       |                                                       |
| `mov`       | At least one operand must be a register direct access |
| `nop`       |                                                       |
| `pop`       | Only `pop r64`                                        |
| `push`      | Only `push r64`                                       |
| `retn`      |                                                       |
| `syscall`   |                                                       |

### Executing assembled programs

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

### How to develop

```
git submodule update --init
```
