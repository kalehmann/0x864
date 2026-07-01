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

        cmp eax, 1
        cmp ecx, 1
        cmp ax, 128
        cmp edx, -128

        jz .end
        jne .end
        jnz .end
.end:
        retn
