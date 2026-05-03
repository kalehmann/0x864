        global demo_12_xchg

;;; Test the xchg instruction
demo_12_xchg:
        xchg rdi, rsi
        xchg al, ah
        xchg bx, cx
        xchg ah, [rbp - 1]
        xchg r15d, r8d

        xchg ax, ax
        xchg eax, eax
        xchg rax, rax
        xchg rax, rbx
        xchg rbx, rax

        xchg rax, [rbp - 8]
        xchg [rbp - 8], rax

        retn
