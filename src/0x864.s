;;; Copyright (c) 2026 by Karsten Lehmann <mail@kalehmann.de>
;;;
;;; This file is part of 0x864.
;;;
;;; 0x864 is free software: you can redistribute it and/or modify
;;; it under the terms of the GNU General Public License as published by
;;; the Free Software Foundation, either version 3 of the License, or
;;; (at your option) any later version.
;;;
;;; 0x864 is distributed in the hope that it will be useful,
;;; but WITHOUT ANY WARRANTY; without even the implied warranty of
;;; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;;; GNU General Public License for more details.
;;;
;;; You should have received a copy of the GNU General Public License
;;; long with 0x864. If not, see <http://www.gnu.org/licenses/>.

	global	assemble
	global  as_snglinst
	global  as_nop
	global  cklb
	global  readnlbl
	global  skp2lbinst

	section .text

assemble:
	push rbp
	mov rbp, rsp
	sub rsp, 40

	;; Receive args per System V ABI
	;; Pointer to assembly text is in rdi register
	;; Pointer to binary output is in rsi register
	;; Max binary output size is in rdx register
	;; Pointer to produced binary output size is in rcx register
	mov [rbp - 8], rdi
	mov [rbp - 16], rsi
	mov [rbp - 24], rdx
	mov [rbp - 32], rcx

	mov [rcx], 0 		; Start with 0 bytes of binary output

.parse_loop:
	lea rdi, [rbp - 8]
	call skp2lbinst

.check_label:
	mov rdi, [rbp - 8]
	call cklb
	cmp rax, 1
	jne .parse_instruction

	lea rdi, [rbp - 8]
	mov rsi, 0
	mov rdx, 0
	call readnlbl
	lea rdi, [rbp - 8]
	call skp2lbinst
	jmp .check_label

.parse_instruction:
	mov rdi, [rbp - 8]
	mov cl, 0
	cmp [rdi], cl
	je .end

	lea rdi, [rbp - 8]
	mov rsi, [rbp - 16]
	mov rdx, [rbp - 24]
	lea rcx, [rbp - 40]
	call as_snginst

	mov rcx, [rbp - 40]
	add [rbp - 16], rcx
	mov rdi, [rbp - 32]
	add [rdi], rcx

	jmp .parse_loop

.end:
	mov rsp, rbp
	pop rbp
	ret

as_snginst:
	push rbp
	mov rbp, rsp

	mov r8, rdi
	mov rdi, [rdi]
	mov al, 0x6e		; Ascii n ('n')
	cmp [rdi], al
	je .n

.n:
	inc rdi
	mov al, 0x6f		; Ascii o ('o')
	cmp [rdi], al
	je .no

.no:
	inc rdi
	mov al, 0x70		; Ascii p ('p')
	cmp [rdi], al
	je .nop

.nop:
	inc rdi
	mov [r8], rdi
	mov rdi, r8
	call as_nop

.end:
	mov rsp, rbp
	pop rbp
	ret

as_nop:
	mov r8, 0
	mov [rcx], r8
	cmp rdx, 0
	je .end
	mov al, 0x90
	mov [rsi], al
	mov r8, 1
	mov [rcx], r8
.end:
	ret
	
cklb:
	push rbp
	mov rbp, rsp
	push rdi
	mov rbp, rsp
	xor rax, rax
	mov bl, 0xa		; Ascii linebreak ('\n')
	mov bh, 0		; Null terminator ('\0')
	mov cl, 0x3b		; Ascii semicolon (';')
	mov ch, 0x3a		; Ascii colon (':')

.loop:
	cmp [rdi], bl
	je .end
	cmp [rdi], bh
	je .end
	cmp [rdi], cl
	je .end
	cmp [rdi], ch
	je .label
	inc rdi
	jmp .loop

.label:
	mov rax, 1		; Return a 1

.end:
	pop rdi
	pop rbp
	ret

readnlbl:
	push rbp
	mov rbp, rsp
	mov rax, 0
	mov r9, 0
	mov r10, rdi

	mov rdi, [rdi]
.loop:
	mov cl, [rdi]
	cmp cl, 0x3a		; Ascii colon
	je .end
	inc rax
	cmp rax, rdx
	ja .no_copy
	mov [rsi], cl
.loop_end:
	inc rdi
	inc rsi
	jmp .loop
.no_copy:
	mov r9, 1
	jmp .loop_end

.end:
	inc rdi
	mov [r10], rdi

	cmp r9, 1
	je .no_terminator
	inc rax
	cmp rax, rdx
	ja .no_terminator
	inc rsi
	mov cl, 0
	mov [rsi], cl
	jmp .ret

.no_terminator:
	mov rax, -1
.ret:
	pop rbp
	ret

skp2lbinst:
	push rbp
	mov rbp, rsp
	mov rsi, [rdi]

.skip_whitespace:
	mov al, 0x20		; Ascii whitespace
	mov ah, 0x9		; Ascii tab
	mov cl, 0xa		; Ascii newline
	mov ch, 0x0		; Ascii null terminator
.skip_whitespace_loop:
	cmp [rsi], al
	je .inc_and_loop
	cmp [rsi], ah
	je .inc_and_loop
	cmp [rsi], cl
	je .inc_and_loop
	cmp [rsi], ch
	je .end
	jmp .analyze
.inc_and_loop:
	inc rsi
	jmp .skip_whitespace_loop

.skip_till_next_line:
	mov al, 0xa		; Ascii newline
	mov ah, 0x0		; Ascii null terminator
.skip_till_next_line_loop:
	cmp [rsi], al
	je .skip_whitespace
	cmp [rsi], ah
	je .end
	inc rsi
	jmp .skip_till_next_line_loop

.analyze:
	mov al, 0x3b		; Ascii semicolon
	cmp [rsi], al
	je .skip_till_next_line

.end:
	mov [rdi], rsi
	pop rbp
	ret
