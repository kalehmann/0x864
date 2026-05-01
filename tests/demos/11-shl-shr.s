        global demo_11_shl_shr

;;; Test the shl and shr instructions
demo_11_shl_shr:
        shl al, 1
        shl bx, 1
        shl ecx, 1
        shl rdx, 1
        shl al, 2
        shl bx, 2
        shl ecx, 2
        shl rdx, 2

        shr al, 1
        shr bx, 1
        shr ecx, 1
        shr rdx, 1
        shr al, 2
        shr bx, 2
        shr ecx, 2
        shr rdx, 2

        retn
