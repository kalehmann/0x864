## 0x864 - selfh0sted.x86.4ssembler

### Milestones

- 2234b86bbc: Assembled a relocatable elf file for hello world.

### Supported instructions

| Instruction | Notes                                                           |
|-------------|-----------------------------------------------------------------|
| `and`       | One operand must be a register                                  |
| `and`       | At least one operand must be a register direct access           |
| `call`      | Only `call rel32` to a label                                    |
| `cmp`       | At least one operand must be a register direct access           |
| `dec`       | Only registers supported as destination operand                 |
| `div`       | Only registers supported as operand                             |
| `inc`       | Only registers supported as destination operand                 |
| `int`       |                                                                 |
| `ja`        | Only `ja rel32` to a label                                      |
| `jb`        | Only `jb rel32` to a label                                      |
| `je`        | Only `je rel32` to a label                                      |
| `jmp`       | Only `jmp rel32` to a label                                     |
| `jne`       | Only `jne rel32` to a label                                     |
| `jnz`       | Only `jnz rel32` to a label                                     |
| `lea`       |                                                                 |
| `mov`       | At least one operand must be a register direct access           |
| `mul`       | Only registers supported as operand                             |
| `nop`       |                                                                 |
| `or`        | At least one operand must be a register direct access           |
| `pop`       | Only `pop r64`                                                  |
| `push`      | Only `push r64`                                                 |
| `retn`      |                                                                 |
| `shl`       | Only `shl r/m*, imm8`                                           |
| `shr`       | Only `shr r/m*, imm8`                                           |
| `sub`       | One operand must be a register                                  |
| `syscall`   |                                                                 |
| `xor`       | At least one operand must be a register direct access           |

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
