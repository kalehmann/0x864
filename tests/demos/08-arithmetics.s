        global demo_08_arithmetics

;;; Test the arithmetic instructions
demo_08_arithmetics:
        push rbp
        mov rbp, rsp

        add al, 8
        add ax, 0x1234
        add eax, 0x12345678
        add rax, 0x12345678
        add rbx, 2

        add ah, 0x1f
        add bx, 0xabcd
        add ecx, 0xabcdef
        add r15, 0xabcdef

        add al, ah
        add ah, cl
        add bx, dx
        add rdi, rsi

        add [rdi], rax
        add [rsi + 16], r15
        add [rbp - 16], rcx

        add al, [rbp - 1]
        add bx, [rbp - 2]
        add ecx, [rbp - 4]
        add rdx, [rbp - 8]

        neg al
        neg bx
        neg ecx
        neg rdx

        sub al, 8
        sub ax, 0x1234
        sub eax, 0x12345678
        sub rax, 0x12345678

        sub ah, 0x1f
        sub bx, 0xabcd
        sub ecx, 0xabcdef
        sub r15, 0xabcdef
        sub rsp, 16

        sub al, ah
        sub ah, cl
        sub bx, dx
        sub rdi, rsi

        sub [rdi], rax
        sub [rsi + 16], r15
        sub [rbp - 16], rcx

        sub al, [rbp - 1]
        sub bx, [rbp - 2]
        sub ecx, [rbp - 4]
        sub rdx, [rbp - 8]

        mul ch
        mul bl
        mul cx
        mul edx
        mul r8

        div ch
        div bl
        div cx
        div edx
        div r8

        mov rsp, rbp
        pop rbp
        retn
