# Supported instructions

| Instruction | Notes                                                           |
|-------------|-----------------------------------------------------------------|
| `add`       | One operand must be a register                                  |
| `and`       | At least one operand must be a register direct access           |
| `call`      | Only `call rel32` to a label                                    |
| `cmp`       | At least one operand must be a register direct access           |
| `dec`       | Only registers supported as destination operand                 |
| `div`       | Only registers supported as operand                             |
| `inc`       | Only registers supported as destination operand                 |
| `int`       |                                                                 |
| `ja`        | Only `ja rel32` to a label                                      |
| `jae`       | Only `jae rel32` to a label                                     |
| `jb`        | Only `jb rel32` to a label                                      |
| `jbe`       | Only `jbe rel32` to a label                                     |
| `je`        | Only `je rel32` to a label                                      |
| `jg`        | Only `jg rel32` to a label                                      |
| `jl`        | Only `jl rel32` to a label                                      |
| `jmp`       | Only `jmp rel32` to a label                                     |
| `jne`       | Only `jne rel32` to a label                                     |
| `jnz`       | Only `jnz rel32` to a label                                     |
| `jz`        | Only `jz rel32` to a label                                      |
| `lea`       |                                                                 |
| `mov`       | At least one operand must be a register direct access           |
| `movs`      | Only no-operands forms `movsb`, `movsd`, `movsq`, `movsw`       |
| `mul`       | Only registers supported as operand                             |
| `neg`       | Only registers supported as destination operand                 |
| `nop`       |                                                                 |
| `or`        | At least one operand must be a register direct access           |
| `pop`       | Only `pop r64`                                                  |
| `push`      | Only `push r64`                                                 |
| `rep movs`  | Only no-operands forms `movsb`, `movsd`, `movsq`, `movsw`       |
| `retn`      |                                                                 |
| `shl`       | Only `shl r/m*, imm8`                                           |
| `shr`       | Only `shr r/m*, imm8`                                           |
| `sub`       | One operand must be a register                                  |
| `syscall`   |                                                                 |
| `xchg`      |                                                                 |
| `xor`       | At least one operand must be a register direct access           |
