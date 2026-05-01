        global demo_10_jmp_conditional

;;; Test the cmp and j* instructions
demo_10_jmp_conditional:
        cmp eax, 0xaabbccdd
        ja .end
        jb .end
        cmp r15d, 0
        je .end
        jne .end
.end:
        retn
