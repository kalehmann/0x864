## 0x864 - selfh0sted.x86.4ssembler

### Supported instructions

| Instruction | Notes                                                 |
|-------------|-------------------------------------------------------|
| `call`      | Only `CALL rel32` to a label                          |
| `dec`       | Only registers supported as destination operand       |
| `inc`       | Only registers supported as destination operand       |
| `mov`       | At least one operand must be a register direct access |
| `nop`       |                                                       |
| `retn`      |                                                       |

### How to develop

```
git submodule update --init
```
