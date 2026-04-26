        global demo_04_pop_push

;;; Test the `pop` and `push` instructions
demo_04_pop_push:
        push rax
        push rbx
        push rcx
        push rdx
        push rbp
        push rsp
        push rdi
        push rsi
        push r8
        push r9
        push r10
        push r11
        push r12
        push r13
        push r14
        push r15

        pop r15
        pop r14
        pop r13
        pop r12
        pop r11
        pop r10
        pop r9
        pop r8
        pop rsi
        pop rdi
        pop rsp
        pop rbp
        pop rdx
        pop rcx
        pop rbx
        pop rax
        retn
