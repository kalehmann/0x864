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
	global	as_snglinst
	global	as_call
	global	as_nop
	global	as_retn
	global	cklb
	global	clr
	global	cpy
	global	isint
	global	isopdlm
	global	isr8
	global	isr16
	global	isr32
	global	isr64
	global	isreg
	global	len
	global	readnlbl
	global	rslvref
	global	scndpss
	global	skp2lbinst
	global	strlbl
	global	strsymtabntr

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

	mov rdi, [rbp - 8]	; Stores ctx in rdi
	call strlbl

	mov rdi, [rbp - 8]	; Stores ctx in rdi
	mov rsi, [rdi + 40]	; size_t n = ctx->max_symtab_entries
	lea rdx, [rdi + 64]	; char *label = ctx->label
	mov rcx, [rdi + 24]	; uint32_t offset = ctx->bintxt_size
	mov rdi, [rdi + 32]	; struct SymTabNtr *symtab = ctx->symtab
	mov r8, 0		; uint32_t flags = 0
	mov r9, 0		; uint32_t rel_target = 0
	call strsymtabntr

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
	mov rdi, [rbp - 8]	; Stores ctx in rdi
	call scndpss

	mov rsp, rbp
	pop rbp
	retn

;;; rdi: `struct AsmCtx *ctx`
as_snginst:
	push rbp
	mov rbp, rsp

	mov rsi, [rdi]		; Stores ctx->assembly in rsi
	mov al, 0x63		; Ascii c ('c')
	cmp [rsi], al
	je .c
	mov al, 0x6e		; Ascii n ('n')
	cmp [rsi], al
	je .n
	mov al, 0x72		; Ascii r ('r')
	cmp [rsi], al
	je .r

.c:
	inc rsi
	mov al, 0x61		; Ascii a ('a')
	cmp [rsi], al
	je .ca

.ca:
	inc rsi
	mov al, 0x6c		; Ascii l ('l')
	cmp [rsi], al
	je .cal

.cal:
	inc rsi
	mov al, 0x6c		; Ascii l ('l')
	cmp [rsi], al
	je .call

.call:
	inc rsi
	mov [rdi], rsi		; ctx->assembly = rsi
	call as_call
	jmp .end

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
as_call:
	push rbp
	mov rbp, rsp
	sub rsp, 8

	mov [rbp - 8], rdi
	call skp2lbinst

	mov rdi, [rbp - 8]
	mov rdi, [rdi]		; char *assembly = ctx->assembly
	call isreg
	cmp rax, 1
	je .call_reg

	mov rdi, [rbp - 8]
	mov rdi, [rdi]		; char *assembly = ctx->assembly
	call isint
	cmp rax, 1
	je .call_int

.call_label:
	mov rdi, [rbp - 8]
	call strlbl

	mov rdi, [rbp - 8]
	mov rsi, [rdi + 8]	; rsi = ctx->bintxt
	mov r8, [rdi + 16]	; r8 = ctx->max_bintxt_size
	mov r9, [rdi + 24]	; r9 = ctx->bintxt_size
	add rsi, r9
	add r9, 5		; Plan to write five bytes
	cmp r9, r8
	ja .label_no_output

	mov al, 0xe8
	mov [rsi], al		; ctx->bintxt[ctx->bintxt_size] = 0xe8
	mov r10, 5
	add [rdi + 24], r10	; ctx->bintxt_size += 5

	mov rsi, [rdi + 56]	; size_t n = ctx->max_reftab_entries
	lea rdx, [rdi + 64]	; char *label = ctx->label
	mov rcx, [rdi + 24]
	sub rcx, 4		; uint32_t offset = ctx->bintxt_size - 4
	mov r8, 0x1		; uint32_t flags = FLAG_RELATIVE
	mov r9, [rdi + 24]	; uint32_t rel_target = ctx->bintxt_size
	mov rdi, [rdi + 48]	; struct SymTabNtr *symtab = ctx->reftab
	call strsymtabntr

	jmp .end

.label_no_output:
	jmp .end

.call_int:
.call_reg:
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

;;; rdi: `void *buf`
;;; rsi: `size_t n`
clr:
	mov rcx, rsi
	cmp rcx, 0
	je .end
	mov al, 0
.loop:
	mov [rdi], al
	inc rdi
	loop .loop
.end:
	retn

;;; rdi: `void *src`
;;; rsi: `void *dst`
;;; rdx: `size_t n`
cpy:
	xchg rdi, rsi
	mov rcx, rdx
	rep movsb
	retn

;;; rdi: `char *assembly`
isint:
	push rdi
	call isopdlm
	pop rdi
	cmp rax, 1
	je .ret_false

	mov al, 0x30		; Ascii zero ('0')
	mov ah, 0x78		; Ascii lowercase letter x
	cmp [rdi], al
	jne .check_decimal
	cmp [rdi + 1], ah
	jne .check_decimal

.check_hexdecimal:
	add rdi, 2
	push rdi
	call isopdlm
	pop rdi
	cmp rax, 1
	je .ret_false

	mov al, 0x30		; Ascii zero ('0')
	mov ah, 0x39		; Ascii nine ('9')
	mov cl, 0x61		; Ascii lowercase letter a
	mov ch, 0x66		; Ascii lowercase letter f
.check_hex_loop:
	cmp [rdi], al
	jb .check_token_delim
	cmp [rdi], ch
	ja .check_token_delim
	cmp [rdi], ah
	jbe .check_hex_next
	cmp [rdi], cl
	jae .check_hex_next
	jmp .check_token_delim
.check_hex_next:
	inc rdi
	jmp .check_hex_loop

.check_decimal:
	mov al, 0x30		; Ascii zero ('0')
	mov ah, 0x39		; Ascii nine ('9')
	cmp [rdi], al
	jb .check_token_delim
	cmp [rdi], ah
	ja .check_token_delim
	inc rdi
	jmp .check_decimal

.ret_false:
	mov rax, 0
	retn

.check_token_delim:
	call isopdlm
	retn

;;; rdi: `char *assembly`
isopdlm:
	mov al, 0x9		; Ascii tabulator ('\t')
	mov ah, 0xa		; Ascii newline ('\n')
	mov cl, 0x20		; Ascii space
	mov ch, 0x2c		; Ascii comma
	mov dl, 0x3b		; Ascii semicolon
	mov dh, 0		; Null terminator
	cmp [rdi], al
	je .ret_true
	cmp [rdi], ah
	je .ret_true
	cmp [rdi], cl
	je .ret_true
	cmp [rdi], ch
	je .ret_true
	cmp [rdi], dl
	je .ret_true
	cmp [rdi], dh
	je .ret_true

.ret_false:
	mov rax, 0
	jmp .end
.ret_true:
	mov rax, 1

.end:
	retn

;;; rdi: `char *assembly`
isr8:
	mov al, 0x61		; Ascii lowercase letter a
	mov ah, 0x62		; Ascii lowercase letter b
	mov cl, 0x63		; Ascii lowercase letter c
	mov ch, 0x64		; Ascii lowercase letter d

	cmp [rdi], al
	je .a_b_c_d
	cmp [rdi], ah
	je .a_b_c_d
	cmp [rdi], cl
	je .a_b_c_d
	cmp [rdi], ch
	je .a_b_c_d
	jmp .ret_false

.a_b_c_d:
	mov al, 0x6c		; Ascii lowercase letter l
	mov ah, 0x68		; Ascii lowercase letter h
	inc rdi
	cmp [rdi], al
	je .check_token_delim
	cmp [rdi], ah
	je .check_token_delim
	jmp .ret_false

.check_token_delim:
	inc rdi
	call isopdlm
	retn

.ret_false:
	mov eax, 0
	retn

isr16:
	mov al, 0x61		; Ascii lowercase letter a
	mov ah, 0x62		; Ascii lowercase letter b
	mov cl, 0x63		; Ascii lowercase letter c
	mov ch, 0x64		; Ascii lowercase letter d

	cmp [rdi], al
	je .a_b_c_d
	cmp [rdi], ah
	je .a_b_c_d
	cmp [rdi], cl
	je .a_b_c_d
	cmp [rdi], ch
	je .a_b_c_d
	jmp .ret_false

.a_b_c_d:
	mov al, 0x78		; Ascii lowercase letter x
	inc rdi
	cmp [rdi], al
	je .check_token_delim
	jmp .ret_false

.check_token_delim:
	inc rdi
	call isopdlm
	retn

.ret_false:
	mov eax, 0
	retn

isr32:
	mov al, 0x65		; Ascii lowercase letter e
	mov ah, 0x72		; Ascii lowercase letter r

	cmp [rdi], al
	je .e
	cmp [rdi], ah
	je .r
	jmp .ret_false

.e:
	inc rdi
	mov al, 0x61		; Ascii lowercase letter a
	cmp [rdi], al
	je .ea_eb_ec
	mov al, 0x62		; Ascii lowercase letter b
	cmp [rdi], al
	je .ea_eb_ec
	mov al, 0x63		; Ascii lowercase letter c
	cmp [rdi], al
	je .ea_eb_ec
	mov al, 0x64		; Ascii lowercase letter d
	cmp [rdi], al
	je .ed
	mov al, 0x73		; Ascii lowercase letter s
	cmp [rdi], al
	je .es
	jmp .ret_false

.ea_eb_ec:
	inc rdi
	mov al, 0x78		; Ascii lowercase letter x
	cmp [rdi], al
	je .check_token_delim
	jmp .ret_false

.ed:
	inc rdi
	mov al, 0x69		; Ascii lowercase letter i
	cmp [rdi], al
	je .check_token_delim
	mov al, 0x78		; Ascii lowercase letter x
	cmp [rdi], al
	je .check_token_delim
	jmp .ret_false

.es:
	inc rdi
	mov al, 0x69		; Ascii lowercase letter i
	cmp [rdi], al
	je .check_token_delim
	jmp .ret_false

.r:
	inc rdi
	mov al, 0x38		; Ascii number 8
	mov ah, 0x39		; Ascii number 9
	mov cl, 0x31		; Ascii number 1
	cmp [rdi], al
	je .r8_r9_r1x
	cmp [rdi], ah
	je .r8_r9_r1x
	cmp [rdi], cl
	je .r1
	jmp .ret_false

.r1:
	inc rdi
	mov al, 0x30		; Ascii number 0
	cmp [rdi], al
	je .r8_r9_r1x
	mov al, 0x31		; Ascii number 1
	cmp [rdi], al
	je .r8_r9_r1x
	mov al, 0x32		; Ascii number 2
	cmp [rdi], al
	je .r8_r9_r1x
	mov al, 0x33		; Ascii number 3
	cmp [rdi], al
	je .r8_r9_r1x
	mov al, 0x34		; Ascii number 4
	cmp [rdi], al
	je .r8_r9_r1x
	mov al, 0x35		; Ascii number 5
	cmp [rdi], al
	je .r8_r9_r1x
	jmp .ret_false

.r8_r9_r1x:
	inc rdi
	mov al, 0x64		; Ascii lowercase letter d
	cmp [rdi], al
	je .check_token_delim
	jmp .ret_false
	jmp .ret_false

.check_token_delim:
	inc rdi
	call isopdlm
	retn

.ret_false:
	mov eax, 0
	retn

;;; rdi: `char *assembly`
isr64:
	mov al, 0x72		; Ascii lowercase letter r
	cmp [rdi], al
	je .r
	jmp .ret_false

.r:
	inc rdi
	mov al, 0x61		; Ascii lowercase letter a
	cmp [rdi], al
	je .ra_rc
	mov al, 0x62		; Ascii lowercase letter b
	cmp [rdi], al
	je .rb
	mov al, 0x63		; Ascii lowercase letter c
	cmp [rdi], al
	je .ra_rc
	mov al, 0x64		; Ascii lowercase letter d
	cmp [rdi], al
	je .rd
	mov al, 0x73		; Ascii lowercase letter s
	cmp [rdi], al
	je .rs
	mov al, 0x38		; Ascii number 8
	cmp [rdi], al
	je .check_token_delim
	mov al, 0x39		; Ascii number 9
	cmp [rdi], al
	je .check_token_delim
	mov al, 0x31		; Ascii number 1
	cmp [rdi], al
	je .r1
	jmp .ret_false

.r1:
	inc rdi
	mov al, 0x30		; Ascii number 0
	cmp [rdi], al
	je .check_token_delim
	mov al, 0x31		; Ascii number 1
	cmp [rdi], al
	je .check_token_delim
	mov al, 0x32		; Ascii number 2
	cmp [rdi], al
	je .check_token_delim
	mov al, 0x33		; Ascii number 3
	cmp [rdi], al
	je .check_token_delim
	mov al, 0x34		; Ascii number 4
	cmp [rdi], al
	je .check_token_delim
	mov al, 0x35		; Ascii number 5
	cmp [rdi], al
	je .check_token_delim
	jmp .ret_false

.ra_rc:
	inc rdi
	mov al, 0x78		; Ascii lowercase letter x
	cmp [rdi], al
	je .check_token_delim
	jmp .ret_false

.rb:
	inc rdi
	mov al, 0x70		; Ascii lowercase letter p
	cmp [rdi], al
	je .check_token_delim
	mov al, 0x78		; Ascii lowercase letter x
	cmp [rdi], al
	je .check_token_delim
	jmp .ret_false

.rd:
	inc rdi
	mov al, 0x69		; Ascii lowercase letter i
	cmp [rdi], al
	je .check_token_delim
	mov al, 0x78		; Ascii lowercase letter x
	cmp [rdi], al
	je .check_token_delim
	jmp .ret_false

.rs:
	inc rdi
	mov al, 0x69		; Ascii lowercase letter i
	cmp [rdi], al
	je .check_token_delim
	mov al, 0x70		; Ascii lowercase letter p
	cmp [rdi], al
	je .check_token_delim
	jmp .ret_false

.check_token_delim:
	inc rdi
	call isopdlm
	retn

.ret_false:
	mov eax, 0
	retn

;;; rdi: `char *assembly`
isreg:
	mov rsi, rdi
	call isr8
	cmp rax, 1
	je .ret_true

	mov rdi, rsi
	call isr16
	cmp rax, 1
	je .ret_true

	mov rdi, rsi
	call isr32
	cmp rax, 1
	je .ret_true

	mov rdi, rsi
	call isr64
	cmp rax, 1
	je .ret_true

	mov eax, 0
	retn

.ret_true:
	mov eax, 1
	retn

;;; rdi: `char *str`
len:
	mov rax, 0
	mov cl, 0
.loop:
	cmp [rdi], cl
	je .end
	inc rax
	inc rdi
	jmp .loop
.end:
	inc rax
	inc rdi

	retn

;;; rdi: `char const **assembly`
;;; rsi: `char *label`
;;; rdx: `size_t n`
readnlbl:
	push rbp
	mov rbp, rsp
	sub rsp, 40

	mov [rbp - 8], rdi
	mov [rbp - 16], rsi
	mov [rbp - 24], rdx
	mov rax, 0
	mov [rbp - 32], rax	; size_t bytes_written = 0
	mov [rbp - 40], rax	; uint64_t overflow = 0

.loop:
	mov rdi, [rbp - 8]
	mov rdi, [rdi]
	call isopdlm
	cmp rax, 0x1		; if (isopdlm(*assembly))
	je .end

	mov rdi, [rbp - 8]
	mov rdi, [rdi]
	mov cl, 0x3a		; Ascii colon
	cmp [rdi], cl		; if ((*assembly)[0] == ':')
	je .end

	mov rax, [rbp - 32]
	inc rax
	mov [rbp - 32], rax	; bytes_written++
	mov rdx, [rbp - 24]
	cmp rax, rdx		; if (bytes_written > n)
	ja .no_copy

	mov cl, [rdi]
	mov rsi, [rbp - 16]
	mov [rsi], cl		; label[0] = (*assembly)[0]
.loop_end:
	inc rdi			; (*assembly)++
	inc rsi			; label++
	mov r9, [rbp - 8]
	mov [r9], rdi
	mov [rbp - 16], rsi
	jmp .loop
.no_copy:
	mov r9, 1
	mov [rbp - 40], r9	; overflow = 1
	jmp .loop_end

.end:
	mov rsi, [rbp - 8]
	mov rdi, [rsi]
	inc rdi
	mov [rsi], rdi		; (*assembly)++

	mov r9, [rbp - 40]
	cmp r9, 1		; if (overflow == 1)
	je .no_terminator
	mov rax, [rbp - 32]
	inc rax			; bytes_written++
	mov rdx, [rbp - 24]
	cmp rax, rdx		; if (bytes_written > n)
	ja .no_terminator

	mov rsi, [rbp - 16]
	inc rsi			; label++
	mov cl, 0
	mov [rsi], cl		; label[0] = '\0'
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
;;; r8: `uint32_t *flags`
;;; r9: `uint32_t *rel_target`
rslvref:
	push rbp
	mov rbp, rsp

	mov al, 0
	mov r10, rdi
	mov r11, rsi

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
	add r11, 256
	mov rdi, r10
	mov rsi, r11
	dec rdx
	jz .ret_err
	jmp .check_label

.load_offset:
	mov eax, [r11 + 252]
	mov [rcx], eax
.load_flags:
	cmp r8, 0
	je .load_relative_target
	mov eax, [r11 + 244]
	mov [r8], eax
.load_relative_target:
	cmp r9, 0
	je .ret_suc
	mov eax, [r11 + 248]
	mov [r9], eax
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

;;; rdi: `struct AsmCtx *ctx`
scndpss:
	push rbp
	mov rbp, rsp
	sub rsp, 40

	mov rax, 0
	mov [rbp - 8], rdi
	mov [rbp - 12], eax	; uint32_t flags
	mov [rbp - 16], eax	; uint32_t rel_target
	mov [rbp - 20], eax	; uint32_t offset
	mov [rbp - 24], eax	; uint32_t target_offset
	mov rsi, [rdi + 48]
	mov [rbp - 32], rsi	; struct SymTabNtr *reftab
	mov rsi, [rdi + 56]
	mov [rbp - 40], rsi	; struct size_t max_reftab_entries

.loop_reftab:
	mov rsi, [rbp - 32]
	mov rcx, [rbp - 40]
	cmp rcx, 0
	je .end
	mov dl, 0
	cmp [rsi], dl		; if (reftab[0].label == '\0')
	je .end

	mov eax, [rsi + 244]
	mov [rbp - 12], eax	; flags = reftab[0].flags
	mov eax, [rsi + 248]
	mov [rbp - 16], eax	; rel_target = reftab[0].rel_target
	mov eax, [rsi + 252]
	mov [rbp - 20], eax	; offset = reftab[0].offset

	mov rdi, [rbp - 32]	; char *label = &reftab[0].label
	mov r8, [rbp - 8]
	mov rsi, [r8 + 32]	; struct SymTabNtr *symtab = ctx->symtab
	mov rdx, [r8 + 40]	; size_t n = ctx->max_symtab_entries
	lea rcx, [rbp - 24]	; uint32_t *offset = &target_offset
	mov r8, 0		; uint32_t *flags = 0
	mov r9, 0		; uint32_t *rel_target = 0
	call rslvref

	mov eax, [rbp - 12]
	cmp eax, 0x1
	je .resolve_relative

.resolve_absolute:
	mov rax, 0
	mov eax, [rbp - 20]
	mov rdi, [rbp - 8]
	mov rsi, [rdi + 8]
	add rsi, rax
	mov eax, [rbp - 24]
	mov [rsi], eax		; memcpy(ctx->bintxt + offset, target_offset, 4)
	jmp .loop_reftab_end

.resolve_relative:
	mov rax, 0
	mov eax, [rbp - 20]
	mov ecx, [rbp - 24]
	mov edx, [rbp - 16]
	sub ecx, edx		; target_offset -= rel_target
	mov rdi, [rbp - 8]
	mov rsi, [rdi + 8]
	add rsi, rax
	mov [rsi], ecx		; memcpy(ctx->bintxt + offset, target_offset, 4)

.loop_reftab_end:
	mov rsi, [rbp - 32]
	add rsi, 256
	mov [rbp - 32], rsi	; reftab++
	mov rcx, [rbp - 40]
	dec rcx
	mov [rbp - 40], rcx	; max_reftab_entries--
	jmp .loop_reftab

.end:
	mov rsp, rbp
	pop rbp
	retn

;;; rdi: `struct AsmCtx *ctx`
strlbl:
	push rbp
	mov rbp, rsp
	sub rsp, 8
	mov [rbp - 8], rdi

	mov rsi, [rdi]		; Stores ctx->assembly in ri
	mov al, 0x2e		; Ascii dot ('.')
	cmp [rsi], al
	jne .store_top_label

.store_sub_label:
	mov rdi, [rbp - 8]	; Stores ctx in rdi
	lea rsi, [rdi + 64]	; void *dst = ctx->label
	lea rdi, [rdi + 304]	; void *src = ctx->_label
	mov rdx, 240		; size_t n = 240
	call cpy

	mov rdi, [rbp - 8]
	lea rdi, [rdi + 304]	; void *str = ctx->_label
	call len

	mov rdi, [rbp - 8]	; char **assembly = &ctx->assembly
	lea rsi, [rdi + 64]	; char *label = ctx->label
	add rsi, rax		; sublabel += len(ctx->_label)
	dec rsi
	mov rdx, 240		; size_t n = 240
	sub rdx, rax		; n -= len(ctx->_label)
	call readnlbl

	jmp .end

.store_top_label:
	lea rdi, [rdi + 64]
	mov rsi, 480
	call clr

	mov rdi, [rbp - 8]	; char **assembly = &ctx->assembly
	lea rsi, [rdi + 64]	; char *label = ctx->label
	mov rdx, 240		; size_t n = 240
	call readnlbl

	mov rdi, [rbp - 8]	; Stores ctx in rdi
	lea rsi, [rdi + 304]	; void *dst = ctx->_label
	lea rdi, [rdi + 64]	; void *src = ctx->label
	mov rdx, 240		; size_t n = 240
	call cpy
.end:
	mov rsp, rbp
	pop rbp
	retn

;;; rdi: `uint8_t (*symtab)[256]`
;;; rsi: `size_t n`
;;; rdx: `char *label`
;;; rcx: `uint32_t offset`
;;; r8: `uint32_t flags`
;;; r9: `uint32_t rel_target`
strsymtabntr:
	push rbp
	mov rbp, rsp

	mov al, 0
	mov r10, 256
.find_free_entry_loop:
	cmp [rdi], al
	je .store_offset
	add rdi, r10
	dec rsi
	jz .ret_err
	jmp .find_free_entry_loop

.store_offset:
	mov [rdi + 252], ecx
	mov [rdi + 244], r8d
	mov [rdi + 248], r9d

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
