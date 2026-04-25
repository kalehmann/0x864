;;; Test the `dec` and `inc` instructions
demo_02_dec_inc:
        inc al
        inc bh
        inc cx
        inc edx
        inc rsi

        dec r15
        dec r8d
        dec edi
        dec cl
        dec dh

        retn
