;;; Test the `mov` instruction
demo_03_mov:
	;; Move register to r/m
	mov [rdi], rax
	mov [rsi + 8], r15d
	mov [rbp - 128], al
	mov [r9 + 128], eax
	mov [rcx], ah
	mov [r15], bx

	;; Move register to register
	mov bl, ah
	mov cx, dx
	mov edi, esi
	mov rbp, rsp

	;; Move r/m to register
	mov bl, [rsi]
	mov cx, [rbp - 2]
	mov edi, [r11 + 0xffff]
	mov r15, [r15 - 0xffff]

	;; Move immediate to register
	mov al, 0
	mov bh, 0xff
	mov bx, 0xffff
	mov edi, 0xdeadc0de
	mov r15, 0xaabbccddeeff1122

	retn

