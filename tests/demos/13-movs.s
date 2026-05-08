        global demo_13_movs

;;; Test the movs* instructions
demo_13_movs:
        movsb
        movsd
        movsq
        movsw

        mov ecx, 1
        rep movsb
        mov ecx, 2
        rep movsd
        mov ecx, 4
        rep movsq
        mov ecx, 8
        rep movsw

        retn
