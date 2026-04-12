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
	global  cklb
	global  skp2lbinst

	section .text

assemble:
	push rbp
	mov rbp, rsp

	;; Receive args per System V ABI
	;; Pointer to assembly text is in rdi register
	;; Pointer to binary output is in rsi register
	;; Max binary output size is in rdx register
	;; Pointer to produced binary output size is in rcx register
	mov [rcx], 0

	pop rbp
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
