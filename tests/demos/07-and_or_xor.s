        global demo_07_and_or_xor

;;; Test the `and`, `or` and `xor` instructions
demo_07_and_or_xor:
        push rbp
        mov rbp, rsp

        and al, 0x12
        and ax, 0x1234
        and eax, 0x12345678
        and rax, 0x12345678
        and bl, 0xff
        and cx, 0xabcd
        and edx, 0x12abcdef
        and rsi, 0x12abcdef
        and rdx, 127

        and cl, ah
        and bx, dx
        and r8d, r15d
        and rdi, rsi

        and [rsi], rax
        and [rdi], ebx
        and [r8], cx
        and [r15], dl

        and r14d, [rsi]
        and r15, [rdi]
        and cx, [rax]
        and al, [r15]

        or al, 0x12
        or ax, 0x1234
        or eax, 0x12345678
        or rax, 0x12345678
        or bl, 0xff
        or cx, 0xabcd
        or edx, 0x12abcdef
        or rsi, 0x12abcdef

        or cl, ah
        or bx, dx
        or r8d, r15d
        or rdi, rsi

        or [rsi], rax
        or [rdi], ebx
        or [r8], cx
        or [r15], dl

        or r14d, [rsi]
        or r15, [rdi]
        or cx, [rax]
        or al, [r15]

        xor al, 0x12
        xor ax, 0x1234
        xor eax, 0x12345678
        xor rax, 0x12345678
        xor bl, 0xff
        xor cx, 0xabcd
        xor edx, 0x12abcdef
        xor rsi, 0x12abcdef

        xor cl, ah
        xor bx, dx
        xor r8d, r15d
        xor rdi, rsi

        xor [rsi], rax
        xor [rdi], ebx
        xor [r8], cx
        xor [r15], dl

        xor r14d, [rsi]
        xor r15, [rdi]
        xor cx, [rax]
        xor al, [r15]

        mov rsp, rbp
        pop rbp
        retn
