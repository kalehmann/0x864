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
        call foobar
        jmp .backward
.end:
        retn

foobar:
        retn
