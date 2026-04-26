;;; A hello world program using the (at the time of writing) limit set of
;;; instructions supported by 0x864.
;;;
;;;  How to run it?
;;;
;;; 1. First compile the assembler with `make`
;;; 2. Then assemble the hello world program to a relocatable elf file
;;;     ```
;;;     ./src/0x864 tests/demos/06-hello-world.s -o tests/demos/06-hello-world.o -felf64
;;;     ```
;;; 3. Now link the relocatable file as an executable file
;;;     ```
;;;     ld -m elf_x86_64 tests/demos/06-hello-world.o -o hello-world
;;;     ```
;;; 4. Execute the program
;;;     ```
;;;     $ ./hello-world
;;;     Hello, world!
;;;     ```

        global _start

_start:
        push rbp
        mov rbp, rsp

        ;; There is no sub instruction yet, reserve 16 bytes on the stack using
        ;; push
        push rax
        push rax

        ;; Write "Hello, world!\n" on the stack starting at [rbp - 16]
        mov rax, 0x77202c6f6c6c6548
        mov [rbp - 16], rax
        mov rax, 0x00000a21646c726f
        mov [rbp - 8], rax

        ;; Syscall - write
        mov eax, 1              ; sys_write
        mov edi, 1              ; unsigned int fd = 1 //stdout
        lea rsi, [rbp - 16]     ; const char *buf
        mov edx, 15             ; size_t count = 15 // len("Hello, world!\n") + 1
        syscall

        ;; Syscall - exit
        mov eax, 60
        mov edi, 0
        syscall
