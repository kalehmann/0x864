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
	global  as_retn
	global  cklb
	global  readnlbl
	global  rslvref
	global  skp2lbinst
	global  strsymtabntr

	section .text

;;; rdi: `struct AsmCtx *ctx`
assemble:
	push rbp
	mov rbp, rsp
	sub rsp, 8
	mov [rbp - 8], rdi

.parse_loop:
	mov rdi, [rbp - 8]	; Stores &ctx->assembly in rdi
	call skp2lbinst

.check_label:
	mov rdi, [rbp - 8]	; Stores &ctx->assembly in rdi
	mov rdi, [rdi]		; Stores ctx->assembly in rdi
	call cklb
	cmp rax, 1
	jne .parse_instruction

	mov rdi, [rbp - 8]	; char **assembly = &ctx->assembly
	mov rsi, 0		; char *label = NULL
	mov rdx, 0		; size_t n = 0
	call readnlbl
	mov rdi, [rbp - 8]	; char **assembly = &ctx->assembly
	call skp2lbinst
	jmp .check_label

.parse_instruction:
	mov rdi, [rbp - 8]	; Stores &ctx->assembly in rdi
	mov rsi, [rdi]		; Stores ctx->assembly in rsi
	mov cl, 0
	cmp [rsi], cl
	je .end

	mov rdi, [rbp - 8]	; Stores ctx in rsi
	call as_snginst

	jmp .parse_loop

.end:
	mov rsp, rbp
	pop rbp
	retn

;;; rdi: `struct AsmCtx *ctx`
as_snginst:
	push rbp
	mov rbp, rsp

	mov rsi, [rdi] 		; Stores ctx->assembly in rsi
	mov al, 0x6e		; Ascii n ('n')
	cmp [rsi], al
	je .n
	mov al, 0x72		; Ascii r ('r')
	cmp [rsi], al
	je .r

.n:
	inc rsi
	mov al, 0x6f		; Ascii o ('o')
	cmp [rsi], al
	je .no

.no:
	inc rsi
	mov al, 0x70		; Ascii p ('p')
	cmp [rsi], al
	je .nop

.nop:
	inc rsi
	mov [rdi], rsi		; ctx->assembly = rsi
	call as_nop
	jmp .end

.r:
	inc rsi
	mov al, 0x65		; Ascii e ('e')
	cmp [rsi], al
	je .re

.re:
	inc rsi
	mov al, 0x74		; Ascii t ('t')
	cmp [rsi], al
	je .ret

.ret:
	inc rsi
	mov al, 0x6e		; Ascii n ('n')
	cmp [rsi], al
	je .retn

.retn:
	inc rsi
	mov [rdi], rsi		; ctx->assembly = rsi
	call as_retn
	jmp .end

.end:
	mov rsp, rbp
	pop rbp
	retn

;;; rdi: `struct AsmCtx *ctx`
as_nop:
	mov rsi, [rdi + 8]	; rsi = ctx->bintxt
	mov r8, [rdi + 16]	; r8 = ctx->max_bintxt_size
	mov r9, [rdi + 24]	; r9 = ctx->bintxt_size
	add rsi, r9
	inc r9			; Plan to write one byte
	cmp r9, r8
	ja .end
	mov al, 0x90
	mov [rsi], al		; ctx->bintxt[ctx->bintxt_size + 1] = 0x90
	mov r8, 1
	add [rdi + 24], r8	; ctx->bintxt_size += 1

.end:
	retn

;;; rdi: `struct AsmCtx *ctx`
as_retn:
	mov rsi, [rdi + 8]	; rsi = ctx->bintxt
	mov r8, [rdi + 16]	; r8 = ctx->max_bintxt_size
	mov r9, [rdi + 24]	; r9 = ctx->bintxt_size
	add rsi, r9
	inc r9			; Plan to write one byte
	cmp r9, r8
	ja .end
	mov al, 0xc3
	mov [rsi], al		; ctx->bintxt[ctx->bintxt_size + 1] = 0xc3
	mov r8, 1
	add [rdi + 24], r8	; ctx->bintxt_size += 1

.end:
	retn

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
	retn

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
	mov rsp, rbp
	pop rbp
	retn

;;; rdi: `char *label`
;;; rsi: `uint8_t (*symtab)[256]`
;;; rdx: `size_t n`
;;; rcx: `uint32_t *offset`
rslvref:
	push rbp
	mov rbp, rsp

	mov al, 0
	mov r9, rdi
	mov r10, rsi

.check_label:
	mov ah, [rdi]
	cmp [rsi], ah
	jne .check_next_entry
	cmp al, ah
	je .load_offset
	inc rdi
	inc rsi
	jmp .check_label

.check_next_entry:
	add r10, 256
	mov rdi, r9
	mov rsi, r10
	dec rdx
	jz .ret_err
	jmp .check_label

.load_offset:
	mov eax, [r10 + 251]
	mov [rcx], eax
	jmp .ret_suc

.ret_err:
	mov rax, 1
	jmp .end

.ret_suc:
	mov rax, 0

.end:
	mov rsp, rbp
	pop rbp
	retn

;;; rdi: `uint8_t (*symtab)[256]`
;;; rsi: `size_t n`
;;; rdx: `char *label`
;;; rcx: `uint32_t offset`
strsymtabntr:
	push rbp
	mov rbp, rsp

	mov al, 0
	mov r8, 256
.find_free_entry_loop:
	cmp [rdi], al
	je .store_offset
	add rdi, r8
	dec rsi
	jz .ret_err
	jmp .find_free_entry_loop

.store_offset:
	mov [rdi + 251], ecx

.store_label:
	cmp [rdx], al
	je .ret_suc
	mov ah, [rdx]
	mov [rdi], ah
	inc rdx
	inc rdi
	jmp .store_label

.ret_err:
	mov rax, 1
	jmp .end

.ret_suc:
	mov rax, 0

.end:
	mov rsp, rbp
	pop rbp
	retn

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
	retn
