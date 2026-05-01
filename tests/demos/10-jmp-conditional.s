        global demo_10_jmp_conditional

;;; Test the cmp and j* instructions
demo_10_jmp_conditional:
        cmp eax, 0xaabbccdd
        je .end
.end:
        retn
