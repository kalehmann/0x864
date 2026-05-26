# Capabilities and quirks of the parser

**Some general notes:**
- lines are terminated with a linefeed character (`\n` / Ascii `0xa`)
- carriage returns (`\r` / Ascii `0xd`) are not supported
- whitespace characters are either the space (Ascii `0x20`) or the tabulator
  (`\t` / Ascii `0x9`)

## Comments

If a semicolon (`;`) is encountered, all following characters until the next
linebreak (`\n`) are handled as comment and ignored.

## Globals

Global symbols are declared with the `global` keyword and may be up to 64
characters long.
0x864 does not care about the position of the `global` keyword in the source
file, but for compatibility with other assemblers like NASM, it is recommended
to declare globals at the beginning of the file before the definition of the
global symbol.

## Integers

0x864 can parse integers in binary, hexadecimal and decimal notation.
Integers are always written without delimiters.

Valid binary integers start with `0b` followed by at least a single zero or one.

Hexadecimal integers start with `0x` followed by a single digit or letter between
a and f.
Only lowercase letters are supported for hexadecimal integers.

Decimal integers may be negative by having a hyphen (`-`) immediately before the
first digit.

Integers are always parsed as `int64_t`, but may be cast to a smaller type
depending on the context.

## Labels

Labels start immediately after a new line and may be up to 239 characters long.
0x864 also supports _local labels_.
Local labels start with a dot (`.`) and are automatically appended to the
previous non-local label.

Take for example

```asm
function1:
        jmp .sub_label
.sub_label:
        nop
        retn

function2:
        jmp .sub_label
.sub_label:
        nop
        retn
```

Here the label `.sub_label` is defined twice.
This is valid, as it is appended to the previous label each time.
Therefore, in that example the following labels are stored:

- `function1`
- `function1.sub_label`
- `function2`
- `function2.sub_label`

**Note**, that the limit of 239 characters applies to the label
combined with the sub label.
Therefore a label with 200 characters may be only followed by a sublabel
with a maximum length of 39 characters (including the dot).

## Register-indirect addressing

0x864 partially supports register-indirect memory addressing with the syntax

- `[reg]` - accesses the memory at the address stored in the register `reg`,
  where `reg` is any register except `rip`, `rsp`, `rpb`, `r12` or `r13`.
- `[reg + disp]` - Memory address is calculated by loading the value of the
  register `reg` and adding a signed 8-bit displacement value to it.
  The value `disp` is an binary, decimal or hexadecimal integer between `1` and
  `127`.
  The register `reg` is any register except `rip`, `rsp` or `r12`.  
- `[reg - disp]` - Memory address is calculated by loading the value of the
  register `reg` and subtracting a signed 8-bit displacement value from it.
  The value `disp` is an binary, decimal or hexadecimal integer between `1` and
  `128`.
  The register `reg` is any register except `rip`, `rsp` or `r12`.
- `[reg + disp]` - Memory address is calculated by loading the value of the
  register `reg` and subtracting a signed 32-bit displacement value from it.
  The value `disp` is an binary, decimal or hexadecimal integer between `1` and
  `2,147,483,647`.
  The register `reg` is any register except `rip`, `rsp` or `r12`.
- `[reg - disp]` - Memory address is calculated by loading the value of the
  register `reg` and subtracting a signed 32-bit displacement value from it.
  The value `disp` is an binary, decimal or hexadecimal integer between `1` and
  `2,147,483,648`.
  The register `reg` is any register except `rip`, `rsp` or `r12`.

Some notes about the parsing:
- the register always precedes the displacement value
- the register immediately follows the opening square bracket (`[`)
- there may be whitespace characters between the register and the following
  operator (`+` / `-`) or closing square bracket (`]`).
- there may be whitespace characters between the operator and displacement value
- there may be whitespace characters between the displacement value and closing
  square bracket (`]`)
