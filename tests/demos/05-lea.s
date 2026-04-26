        global demo_05_lea

;;; Test the `lea` instruction
demo_05_lea:
        push rbp
        mov rbp, rsp

        lea rsi, [rbp - 8]
        lea rax, [rsi + 512]

        lea ax, [rbp - 16]
        lea r8d, [rbp - 24]

        mov rsp, rbp
        pop rbp
        retn
