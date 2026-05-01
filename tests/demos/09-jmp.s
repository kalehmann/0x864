        global demo_09_jmp

;;; Test the jmp instruction
demo_09_jmp:
        jmp .forward
        nop
.backward:
        nop
        jmp .end
        nop
.forward:
        nop
        jmp .backward
.end:
        retn
