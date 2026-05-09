        global demo_10_jmp_conditional

;;; Test the cmp and j* instructions
demo_10_jmp_conditional:
        cmp eax, 0xaabbccdd
        ja .end
        jae .end
        jb .end
        jbe .end
        cmp r15d, 0
        je .end
        jg .end
        jl .end
        jz .end
        jne .end
        jnz .end
.end:
        retn
