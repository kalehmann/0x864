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

        global  algn16
        global  assemble
        global  assemble_op
        global  as_snglinst
        global  as_add
        global  as_and
        global  as_call
        global  as_cmp
        global  as_dec
        global  as_div
        global  as_genjmp
        global  as_genop1rm
        global  as_genop2ax32
        global  as_inc
        global  as_int
        global  as_ja
        global  as_jb
        global  as_je
        global  as_jmp
        global  as_jne
        global  as_lea
        global  as_mov
        global  as_mul
        global  as_nop
        global  as_or
        global  as_pop
        global  as_push
        global  as_retn
        global  as_shl
        global  as_shr
        global  as_sub
        global  as_syscall
        global  as_xchg
        global  as_xor
        global  cklb
        global  ckln
        global  ckopsize
        global  ckoptps
        global  clr
        global  cpy
        global  elf64_clcshstrtabsz
        global  elf64_clcstrtabsz
        global  elf64_clcsymtabsz
        global  elf64_clctextsz
        global  elf64_dump
        global  elf64_dump_header
        global  genop2aximm32
        global  genop2rimm32
        global  genop2rmr
        global  genop2rrm
        global  isglbl
        global  isint
        global  isopdlm
        global  isrgndrct
        global  isreg
        global  len
        global  pint
        global  pr8
        global  pr16
        global  pr32
        global  pr64
        global  preg
        global  prgndrct
        global  readnlbl
        global  rslvref
        global  scndpss
        global  skp2lbinst
        global  skp2nxtop
        global  strdspmodrmmod
        global  strglbl
        global  strlbl
        global  strsymtabntr
        global  symtablen
        global  symtabnglbls

;;; rdi: `size_t off`
algn16:
        mov rax, rdi
        and rdi, 0xf
        cmp rdi, 0
        je .ret
        xor rdi, 0xf
        inc rdi
        add rax, rdi
.ret:
        retn

;;; rdi: `struct AsmCtx *ctx`
assemble:
        push rbp
        mov rbp, rsp
        sub rsp, 8
        mov [rbp - 8], rdi

.parse_loop:
        mov rdi, [rbp - 8]      ; Stores &ctx->assembly in rdi
        call skp2lbinst

.check_label:
        mov rdi, [rbp - 8]      ; Stores &ctx->assembly in rdi
        mov rdi, [rdi]          ; Stores ctx->assembly in rdi
        call cklb
        cmp rax, 1
        jne .parse_instruction

        mov rdi, [rbp - 8]      ; Stores ctx in rdi
        call strlbl

        mov rdi, [rbp - 8]      ; Stores ctx in rdi
        mov rsi, [rdi + 40]     ; size_t n = ctx->max_symtab_entries
        lea rdx, [rdi + 80]     ; char *label = ctx->label
        mov rcx, [rdi + 24]     ; uint32_t offset = ctx->bintxt_size
        mov rdi, [rdi + 32]     ; struct SymTabNtr *symtab = ctx->symtab
        mov r8, 0               ; uint32_t flags = 0
        mov r9, 0               ; uint32_t rel_target = 0
        call strsymtabntr

        mov rdi, [rbp - 8]      ; char **assembly = &ctx->assembly
        call skp2lbinst
        jmp .check_label

.parse_instruction:
        mov rdi, [rbp - 8]      ; Stores &ctx->assembly in rdi
        mov rsi, [rdi]          ; Stores ctx->assembly in rsi
        mov cl, 0
        cmp [rsi], cl
        je .end

        mov rdi, [rbp - 8]      ; Stores ctx in rsi
        call as_snginst
        cmp rax, 0
        jne .ret_err

        jmp .parse_loop

.end:
        mov rdi, [rbp - 8]      ; Stores ctx in rdi
        call scndpss

        mov rsp, rbp
        pop rbp
        retn

.ret_err:
        mov rsp, rbp
        pop rbp
        retn

;;; rdi: `struct AsmCtx *ctx`
;;; rsi: `struct AsmOp *op`
assemble_op:
        push rbp
        mov rbp, rsp
        sub rsp, 40
        mov [rbp - 8], rdi      ; Store ctx
        mov [rbp - 16], rsi     ; Store op
        mov al, 0
        mov [rbp - 33], al      ; uint8_t bytes_to_write = 0
        mov [rbp - 34], al      ; uint8_t rex = 0
        mov [rbp - 35], al      ; uint8_t modrm = 0

        lea rdi, [rbp - 32]     ; uint8_t buffer[16]
        mov rsi, 16
        call clr                ; clr(buffer, 16)

        mov rdi, [rbp - 16]
        mov al, [rdi]
        cmp al, 0               ; if (op->encoding == ENCODING_ZO)
        je .zo
        cmp al, 1               ; if (op->encoding == ENCODING_D)
        je .d
        cmp al, 2               ; if (op->encoding == ENCODING_I)
        je .i
        cmp al, 3               ; if (op->encoding == ENCODING_M)
        je .m
        cmp al, 4               ; if (op->encoding == ENCODING_MI)
        je .mi
        cmp al, 5               ; if (op->encoding == ENCODING_MR)
        je .mr
        cmp al, 6               ; if (op->encoding == ENCODING_O)
        je .o
        cmp al, 7               ; if (op->encoding == ENCODING_OI)
        je .oi
        cmp al, 8               ; if (op->encoding == ENCODING_RM)
        je .rm

;;; rdi: `struct AsmOp *op`
;;; rsi: `uint8_t *buffer`
;;; rdx: `uint8_t *bytes_to_write`
.store_disp:
        xor rax, rax
        mov al, [rdx]
        add rsi, rax
        mov al, [rdi + 4]
        cmp al, 0x0             ; if (op->modrm_mod == MOD_INDIRECT)
        je .store_disp_end
        cmp al, 0x1             ; if (op->modrm_mod == MOD_INDIRECT_8)
        je .store_disp8
        cmp al, 0x2             ; if (op->modrm_mod == MOD_INDIRECT_32)
        je .store_disp32
        cmp al, 0x3             ; if (op->modrm_mod == MOD_DIRECT)
        je .store_disp_end

.store_disp8:
        mov al, [rdi + 12]
        mov [rsi], al
        mov al, [rdx]
        inc al
        mov [rdx], al           ; bytes_to_write++
        jmp .store_disp_end

.store_disp32:
        mov eax, [rdi + 12]
        mov [rsi], eax
        mov al, [rdx]
        add al, 4
        mov [rdx], al           ; bytes_to_write += 4
        jmp .store_disp_end

.store_disp_end:
        retn

;;; rdi: `struct AsmOp *op`
;;; rsi: `uint8_t *buffer`
;;; rdx: `uint8_t *bytes_to_write`
.store_immediate:
        xor ecx, ecx
        mov cl, [rdx]           ; rcx = *bytes_to_write
        add rsi, rcx            ; buffer += bytes_to_write

        mov al, [rdi + 9]       ; al = op->imm_size
        cmp al, 8
        je .store_immediate_8
        cmp al, 16
        je .store_immediate_16
        cmp al, 32
        je .store_immediate_32
        cmp al, 64
        je .store_immediate_64

.store_immediate_8:
        mov al, [rdi + 16]      ; al = op->imm.imm8
        mov [rsi], al
        inc cl
        mov [rdx], cl
        jmp .store_immediate_end

.store_immediate_16:
        mov ax, [rdi + 16]      ; ax = op->imm.imm16
        mov [rsi], ax
        add cl, 2
        mov [rdx], cl
        jmp .store_immediate_end

.store_immediate_32:
        mov eax, [rdi + 16]     ; eax = op->imm.imm32
        mov [rsi], eax
        add cl, 4
        mov [rdx], cl
        jmp .store_immediate_end

.store_immediate_64:
        mov rax, [rdi + 16]     ; rax = op->imm.imm64
        mov [rsi], rax
        add cl, 8
        mov [rdx], cl
        jmp .store_immediate_end

.store_immediate_end:
        retn

;;; rdi: `struct AsmOp *op`
;;; rsi: `uint8_t *rex`
;;; rdx: `uint8_t *modrm`
;;; rcx: `uint8_t register`
.store_modrm_rm:
        cmp cl, 8               ; if (register < 8)
        jb .store_modrm_rm_skip_rex_b
        mov al, [rsi]
        or al, 0x41             ; Set the REX.B bit
        mov [rsi], al
.store_modrm_rm_skip_rex_b:
        mov al, [rdx]           ; al = modrm
        mov ah, [rdi + 4]       ; ah = op->modrm_mod
        shl ah, 6
        or al, ah
        and cl, 0x07
        or al, cl
        mov [rdx], al
        retn

;;; rdi: `uint8_t *modrm`
;;; rsi: `uint8_t *buffer`
;;; rdx: `uint8_t *bytes_to_write`
.store_modrm:
        mov al, [rdi]
        xor rcx, rcx
        mov cl, [rdx]
        add rsi, rcx            ; buffer += *bytes_to_write
        mov [rsi], al
        inc cl
        mov [rdx], cl
        retn

;;; rdi: `struct AsmOp *op`
;;; rsi: `uint8_t *rex`
;;; rdx: `uint8_t *modrm`
;;; rcx: `uint8_t register`
.store_modrm_reg:
        cmp cl, 8               ; if (register < 8)
        jb .store_modrm_reg_skip_rex_r
        mov al, [rsi]
        or al, 0x44             ; Set the REX.R bit
        mov [rsi], al
.store_modrm_reg_skip_rex_r:
        mov al, [rdx]           ; al = modrm
        and cl, 0x07
        shl cl, 3
        or al, cl
        mov [rdx], al
        retn

;;; rdi: `struct AsmOp *op`
;;; rsi: `uint8_t *buffer`
;;; rdx: `uint8_t *bytes_to_write`
.store_opcodes:
        mov cl, [rdi + 5]       ; op->n_opcodes
        lea rdi, [rdi + 6]      ; op->opcodes
        xor rax, rax
        mov al, [rdx]
        add rsi, rax
.store_opcodes_loop:
        mov al, [rdi]
        mov [rsi], al
        inc rdi
        inc rsi
        mov al, [rdx]
        inc al
        mov [rdx], al           ; bytes_to_write++
        dec cl
        jnz .store_opcodes_loop
        retn

;;; rdi: `struct AsmOp *op`
;;; rsi: `uint8_t *buffer`
;;; rdx: `uint8_t *bytes_to_write`
;;; rcx: `uint8_t register`
.store_opcodes_rd:
        mov r8b, [rdi + 5]      ; op->n_opcodes
        lea rdi, [rdi + 6]      ; op->opcodes
        xor rax, rax
        mov al, [rdx]
        add rsi, rax            ; buffer += bytes_to_write
.store_opcodes_rd_loop:
        mov al, [rdi]
        mov [rsi], al
        inc rdi
        inc rsi
        mov al, [rdx]
        inc al
        mov [rdx], al           ; bytes_to_write++
        dec r8b
        jz .store_opcodes_rd_encode_reg
        jmp .store_opcodes_rd_loop

.store_opcodes_rd_encode_reg:
        and cl, 0x07
        add [rsi - 1], cl
        retn

;;; rdi: `struct AsmOp *op`
;;; rsi: `uint8_t *rex`
;;; rdx: `uint8_t *buffer`
;;; rcx: `uint8_t *bytes_to_write`
.store_prefixes:
        xor rax, rax
        mov al, [rcx]
        add rdx, rax            ; buffer += *bytes_to_write

        mov ah, [rdi + 11]
        and ah, 0x01
        cmp ah, 0               ; if ((op->prefix & PREFIX_LOCK) == 0)
        je .store_prefixes_repne
        mov ah, 0xf0
        mov [rdx], ah
        inc al
        inc rdx

.store_prefixes_repne:
        mov ah, [rdi + 11]
        and ah, 0x02
        cmp ah, 0               ; if ((op->prefix & PREFIX_REPNE_REPNZ) == 0)
        je .store_prefixes_repe
        mov ah, 0xf2
        mov [rdx], ah
        inc al
        inc rdx

.store_prefixes_repe:
        mov ah, [rdi + 11]
        and ah, 0x04
        cmp ah, 0               ; if ((op->prefix & PREFIX_REPE_REPZ) == 0)
        je .store_prefixes_op_size
        mov ah, 0xf3
        mov [rdx], ah
        inc al
        inc rdx

.store_prefixes_op_size:
        mov ah, [rdi + 11]
        and ah, 0x08
        cmp ah, 0               ; if ((op->prefix & PREFIX_OP_SIZE_OVERRIDE) == 0)
        je .store_prefixes_rex
        mov ah, 0x66
        mov [rdx], ah
        inc al
        inc rdx

.store_prefixes_rex:
        mov ah, [rsi]
        cmp ah, 0               ; if (*rex == 0)
        je .store_prefixes_end
        mov [rdx], ah
        inc al

.store_prefixes_end:
        mov [rcx], al
        retn

;;; rdi: `struct AsmOp *op`
;;; rsi: `uint8_t *rex`
.store_rex_w:
        mov cl, [rdi + 1]
        cmp cl, 64              ; if (op->opsize != 64)
        jne .store_rex_w_end
        mov al, [rsi]
        or al, 0x48
        mov [rsi], al
.store_rex_w_end:
        retn

.zo:
        lea rsi, [rbp - 32]     ; uint8_t *buffer = &buffer
        lea rdx, [rbp - 33]     ; uint8_t *bytes_to_write = &bytes_to_write
        call .store_opcodes
        jmp .write_buffer

.d:
        lea rsi, [rbp - 32]     ; uint8_t *buffer = &buffer
        lea rdx, [rbp - 33]     ; uint8_t *bytes_to_write = &bytes_to_write
        call .store_opcodes

        mov rdi, [rbp - 16]
        mov al, 0
        cmp [rdi + 10], al      ; if (op->d_label == D_LABEL_NONE)
        je .d_immediate
        mov al, 1
        cmp [rdi + 10], al      ; if (op->d_label == D_LABEL_ABSOLUTE)
        je .d_absolute_label
        mov al, 2
        cmp [rdi + 10], al      ; if (op->d_label == D_LABEL_RELATIVE)
        je .d_relative_label

.d_absolute_label:
        mov rdi, [rbp - 8]
        mov rsi, [rdi + 56]     ; size_t n = ctx->max_reftab_entries
        lea rdx, [rdi + 80]     ; char *label = ctx->label
        mov rcx, [rdi + 24]
        xor rax, rax
        mov al, [rbp - 33]
        add rcx, rax            ; uint32_t offset = ctx->bintxt_size + bytes_to_write
        mov r8, 0x0             ; uint32_t flags = 0
        mov r9, 0               ; uint32_t rel_target = 0
        mov rdi, [rdi + 48]     ; struct SymTabNtr *symtab = ctx->reftab
        call strsymtabntr
        jmp .d_immediate

.d_relative_label:
        mov rdi, [rbp - 8]
        mov rsi, [rdi + 56]     ; size_t n = ctx->max_reftab_entries
        lea rdx, [rdi + 80]     ; char *label = ctx->label
        mov rcx, [rdi + 24]
        xor rax, rax
        mov al, [rbp - 33]
        add rcx, rax            ; uint32_t offset = ctx->bintxt_size + bytes_to_write
        mov r8, 0x1             ; uint32_t flags = FLAG_RELATIVE
        mov r9, rcx
        add r9, 4               ; uint32_t rel_target = offset + 4
        mov rdi, [rdi + 48]     ; struct SymTabNtr *symtab = ctx->reftab
        call strsymtabntr
        ;; Still store the immediate value to act as a dummy value until the
        ;; reference to the label is resolved.
.d_immediate:
        mov rdi, [rbp - 16]
        lea rsi, [rbp - 32]
        lea rdx, [rbp - 33]
        call .store_immediate
        jmp .write_buffer

.i:
        lea rsi, [rbp - 34]
        call .store_rex_w

        mov rdi, [rbp - 16]     ; struct AsmOp *op = op
        lea rsi, [rbp - 34]
        lea rdx, [rbp - 32]
        lea rcx, [rbp - 33]
        call .store_prefixes

        lea rsi, [rbp - 32]     ; uint8_t *buffer = &buffer
        lea rdx, [rbp - 33]     ; uint8_t *bytes_to_write = &bytes_to_write
        call .store_opcodes

        mov rdi, [rbp - 16]     ; struct ASmOp *op = op
        lea rsi, [rbp - 32]     ; uint8_t *buffer = &buffer
        lea rdx, [rbp - 33]     ; uint8_t *bytes_to_write = &bytes_to_write
        call .store_immediate
        jmp .write_buffer

.m:
        lea rsi, [rbp - 34]
        call .store_rex_w

        lea rsi, [rbp - 34]
        lea rdx, [rbp - 35]
        xor rcx, rcx
        mov cl, [rdi + 2]
        call .store_modrm_reg

        lea rsi, [rbp - 34]
        lea rdx, [rbp - 35]
        xor rcx, rcx
        mov cl, [rdi + 3]
        call .store_modrm_rm

        mov rdi, [rbp - 16]     ; struct AsmOp *op = op
        lea rsi, [rbp - 34]
        lea rdx, [rbp - 32]
        lea rcx, [rbp - 33]
        call .store_prefixes

        mov rdi, [rbp - 16]     ; struct AsmOp *op = op
        lea rsi, [rbp - 32]     ; uint8_t *buffer = &buffer
        lea rdx, [rbp - 33]     ; uint8_t *bytes_to_write = &bytes_to_write
        call .store_opcodes

        lea rdi, [rbp - 35]
        lea rsi, [rbp - 32]
        lea rdx, [rbp - 33]
        call .store_modrm

        jmp .write_buffer

.mi:
        lea rsi, [rbp - 34]
        call .store_rex_w

        lea rsi, [rbp - 34]
        lea rdx, [rbp - 35]
        xor rcx, rcx
        mov cl, [rdi + 2]
        call .store_modrm_reg

        lea rsi, [rbp - 34]
        lea rdx, [rbp - 35]
        xor rcx, rcx
        mov cl, [rdi + 3]
        call .store_modrm_rm

        mov rdi, [rbp - 16]     ; struct AsmOp *op = op
        lea rsi, [rbp - 34]
        lea rdx, [rbp - 32]
        lea rcx, [rbp - 33]
        call .store_prefixes

        mov rdi, [rbp - 16]     ; struct AsmOp *op = op
        lea rsi, [rbp - 32]     ; uint8_t *buffer = &buffer
        lea rdx, [rbp - 33]     ; uint8_t *bytes_to_write = &bytes_to_write
        call .store_opcodes

        lea rdi, [rbp - 35]
        lea rsi, [rbp - 32]
        lea rdx, [rbp - 33]
        call .store_modrm

        mov rdi, [rbp - 16]
        lea rsi, [rbp - 32]
        lea rdx, [rbp - 33]
        call .store_immediate
        jmp .write_buffer

.mr:
        lea rsi, [rbp - 34]
        call .store_rex_w

        lea rsi, [rbp - 34]
        lea rdx, [rbp - 35]
        xor rcx, rcx
        mov cl, [rdi + 2]
        call .store_modrm_reg

        lea rsi, [rbp - 34]
        lea rdx, [rbp - 35]
        xor rcx, rcx
        mov cl, [rdi + 3]
        call .store_modrm_rm

        mov rdi, [rbp - 16]     ; struct AsmOp *op = op
        lea rsi, [rbp - 34]
        lea rdx, [rbp - 32]
        lea rcx, [rbp - 33]
        call .store_prefixes

        mov rdi, [rbp - 16]     ; struct AsmOp *op = op
        lea rsi, [rbp - 32]     ; uint8_t *buffer = &buffer
        lea rdx, [rbp - 33]     ; uint8_t *bytes_to_write = &bytes_to_write
        call .store_opcodes

        lea rdi, [rbp - 35]
        lea rsi, [rbp - 32]
        lea rdx, [rbp - 33]
        call .store_modrm

        mov rdi, [rbp - 16]     ; struct AsmOp *op = op
        lea rsi, [rbp - 32]
        lea rdx, [rbp - 33]
        call .store_disp
        jmp .write_buffer

.o:
        lea rsi, [rbp - 34]
        call .store_rex_w

        lea rsi, [rbp - 34]
        lea rdx, [rbp - 35]
        xor rcx, rcx
        mov cl, [rdi + 3]
        call .store_modrm_rm

        mov rdi, [rbp - 16]     ; struct AsmOp *op = op
        lea rsi, [rbp - 34]
        lea rdx, [rbp - 32]
        lea rcx, [rbp - 33]
        call .store_prefixes

        mov rdi, [rbp - 16]     ; struct AsmOp *op = op
        lea rsi, [rbp - 32]     ; rsi = buffer
        lea rdx, [rbp - 33]     ; rdx = &bytes_to_write
        mov rcx, [rdi + 3]      ; rcx = op->dest_reg
        call .store_opcodes_rd

        jmp .write_buffer

.oi:
        lea rsi, [rbp - 34]
        call .store_rex_w

        lea rsi, [rbp - 34]
        lea rdx, [rbp - 35]
        xor rcx, rcx
        mov cl, [rdi + 3]
        call .store_modrm_rm

        mov rdi, [rbp - 16]     ; struct AsmOp *op = op
        lea rsi, [rbp - 34]
        lea rdx, [rbp - 32]
        lea rcx, [rbp - 33]
        call .store_prefixes

        mov rdi, [rbp - 16]     ; struct AsmOp *op = op
        lea rsi, [rbp - 32]     ; rsi = buffer
        lea rdx, [rbp - 33]     ; rdx = &bytes_to_write
        mov rcx, [rdi + 3]      ; rcx = op->dest_reg
        call .store_opcodes_rd

        mov rdi, [rbp - 16]
        lea rsi, [rbp - 32]
        lea rdx, [rbp - 33]
        call .store_immediate
        jmp .write_buffer

.rm:
        lea rsi, [rbp - 34]
        call .store_rex_w

        lea rsi, [rbp - 34]
        lea rdx, [rbp - 35]
        xor rcx, rcx
        mov cl, [rdi + 3]
        call .store_modrm_reg

        lea rsi, [rbp - 34]
        lea rdx, [rbp - 35]
        xor rcx, rcx
        mov cl, [rdi + 2]
        call .store_modrm_rm

        mov rdi, [rbp - 16]     ; struct AsmOp *op = op
        lea rsi, [rbp - 34]
        lea rdx, [rbp - 32]
        lea rcx, [rbp - 33]
        call .store_prefixes

        mov rdi, [rbp - 16]     ; struct AsmOp *op = op
        lea rsi, [rbp - 32]     ; uint8_t *buffer = &buffer
        lea rdx, [rbp - 33]     ; uint8_t *bytes_to_write = &bytes_to_write
        call .store_opcodes

        lea rdi, [rbp - 35]
        lea rsi, [rbp - 32]
        lea rdx, [rbp - 33]
        call .store_modrm

        mov rdi, [rbp - 16]     ; struct AsmOp *op = op
        lea rsi, [rbp - 32]
        lea rdx, [rbp - 33]
        call .store_disp
        jmp .write_buffer

.write_buffer:
        mov rdi, [rbp - 8]
        mov r10, [rdi + 8]      ; rsi = ctx->bintxt
        mov r8, [rdi + 16]      ; r8 = ctx->max_bintxt_size
        mov r9, [rdi + 24]      ; r9 = ctx->bintxt_size
        xor ecx, ecx
        mov cl, [rbp - 33]      ; cl = bytes_to_write
        add r10, r9
        add r9, rcx             ; r9 += bytes_to_write
        cmp r9, r8
        ja .end
        mov [rdi + 24], r9
        lea rsi, [rbp - 32]     ; rsi = buffer
.write_buffer_copy:
        mov al, [rsi]
        mov [r10], al
        inc rsi
        inc r10
        dec cl
        jne .write_buffer_copy
.end:
        mov rsp, rbp
        pop rbp
        retn

;;; rdi: `struct AsmCtx *ctx`
as_snginst:
        push rbp
        mov rbp, rsp
        sub rsp, 32
        mov [rbp - 8], rdi

        lea rdi, [rbp - 32]
        mov rsi, 24
        call clr

        mov rdi, [rbp - 8]
.check_add:
        mov rsi, 0x646461   ; add
        call .testinst
        cmp rax, 1
        jne .check_and
        lea rsi, [rbp - 32]
        call as_add
        jmp .assemble
.check_and:
        mov rsi, 0x646e61   ; and
        call .testinst
        cmp rax, 1
        jne .check_call
        lea rsi, [rbp - 32]
        call as_and
        jmp .assemble
.check_call:
        mov rsi, 0x006c6c6163   ; call
        call .testinst
        cmp rax, 1
        jne .check_cmp
        lea rsi, [rbp - 32]
        call as_call
        jmp .assemble
.check_cmp:
        mov rsi, 0x706d63       ; cmp
        call .testinst
        cmp rax, 1
        jne .check_dec
        lea rsi, [rbp - 32]
        call as_cmp
        jmp .assemble
.check_dec:
        mov rsi, 0x00636564     ; dec
        call .testinst
        cmp rax, 1
        jne .check_div
        lea rsi, [rbp - 32]
        call as_dec
        jmp .assemble
.check_div:
        mov rsi, 0x00766964     ; div
        call .testinst
        cmp rax, 1
        jne .check_global
        lea rsi, [rbp - 32]
        call as_div
        jmp .assemble
.check_global:
        mov rsi, 0x6c61626f6c67 ; global
        call .testinst
        cmp rax, 1
        jne .check_inc
        call strglbl
        jmp .end
.check_inc:
        mov rsi, 0x00636e69     ; inc
        call .testinst
        cmp rax, 1
        jne .check_int
        lea rsi, [rbp - 32]
        call as_inc
        jmp .assemble
.check_int:
        mov rsi, 0x746e69       ; int
        call .testinst
        cmp rax, 1
        jne .check_ja
        lea rsi, [rbp - 32]
        call as_int
        jmp .assemble
.check_ja:
        mov rsi, 0x616a         ; ja
        call .testinst
        cmp rax, 1
        jne .check_jb
        lea rsi, [rbp - 32]
        call as_ja
        jmp .assemble
.check_jb:
        mov rsi, 0x626a         ; jb
        call .testinst
        cmp rax, 1
        jne .check_je
        lea rsi, [rbp - 32]
        call as_jb
        jmp .assemble
.check_je:
        mov rsi, 0x656a         ; je
        call .testinst
        cmp rax, 1
        jne .check_jmp
        lea rsi, [rbp - 32]
        call as_je
        jmp .assemble
.check_jmp:
        mov rsi, 0x706d6a       ; jmp
        call .testinst
        cmp rax, 1
        jne .check_jne
        lea rsi, [rbp - 32]
        call as_jmp
        jmp .assemble
.check_jne:
        mov rsi, 0x656e6a       ; jne
        call .testinst
        cmp rax, 1
        jne .check_jnz
        lea rsi, [rbp - 32]
        call as_jne
        jmp .assemble
.check_jnz:
        mov rsi, 0x7a6e6a       ; jnz
        call .testinst
        cmp rax, 1
        jne .check_jz
        lea rsi, [rbp - 32]
        call as_jne             ; jnz is same as jne
        jmp .assemble
.check_jz:
        mov rsi, 0x7a6a         ; jz
        call .testinst
        cmp rax, 1
        jne .check_lea
        lea rsi, [rbp - 32]
        call as_je              ; jz is same as je
        jmp .assemble
.check_lea:
        mov rsi, 0x0061656c     ; lea
        call .testinst
        cmp rax, 1
        jne .check_mov
        lea rsi, [rbp - 32]
        call as_lea
        jmp .assemble
.check_mov:
        mov rsi, 0x00766f6d     ; mov
        call .testinst
        cmp rax, 1
        jne .check_mul
        lea rsi, [rbp - 32]
        call as_mov
        jmp .assemble
.check_mul:
        mov rsi, 0x6c756d     ; mul
        call .testinst
        cmp rax, 1
        jne .check_nop
        lea rsi, [rbp - 32]
        call as_mul
        jmp .assemble
.check_nop:
        mov rsi, 0x00706f6e     ; nop
        call .testinst
        cmp rax, 1
        jne .check_or
        lea rsi, [rbp - 32]
        call as_nop
        jmp .assemble
.check_or:
        mov rsi, 0x726f     ; or
        call .testinst
        cmp rax, 1
        jne .check_pop
        lea rsi, [rbp - 32]
        call as_or
        jmp .assemble
.check_pop:
        mov rsi, 0x00706f70     ; pop
        call .testinst
        cmp rax, 1
        jne .check_push
        lea rsi, [rbp - 32]
        call as_pop
        jmp .assemble
.check_push:
        mov rsi, 0x0068737570   ; push
        call .testinst
        cmp rax, 1
        jne .check_retn
        lea rsi, [rbp - 32]
        call as_push
        jmp .assemble
.check_retn:
        mov rsi, 0x006e746572   ; retn
        call .testinst
        cmp rax, 1
        jne .check_shl
        lea rsi, [rbp - 32]
        call as_retn
        jmp .assemble
.check_shl:
        mov rsi, 0x6c6873       ; shl
        call .testinst
        cmp rax, 1
        jne .check_shr
        lea rsi, [rbp - 32]
        call as_shl
        jmp .assemble
.check_shr:
        mov rsi, 0x726873       ; shl
        call .testinst
        cmp rax, 1
        jne .check_sub
        lea rsi, [rbp - 32]
        call as_shr
        jmp .assemble
.check_sub:
        mov rsi, 0x627573       ; sub
        call .testinst
        cmp rax, 1
        jne .check_syscall
        lea rsi, [rbp - 32]
        call as_sub
        jmp .assemble
.check_syscall:
        mov rsi, 0x006c6c6163737973
        call .testinst
        cmp rax, 1
        jne .check_xchg
        lea rsi, [rbp - 32]
        call as_syscall
        jmp .assemble
.check_xchg:
        mov rsi, 0x67686378     ; xchg
        call .testinst
        cmp rax, 1
        jne .check_xor
        lea rsi, [rbp - 32]
        call as_xchg
        jmp .assemble
.check_xor:
        mov rsi, 0x726f78       ; xor
        call .testinst
        cmp rax, 1
        jne .check_err
        lea rsi, [rbp - 32]
        call as_xor
        jmp .assemble

.check_err:
        ;; Return ERR_UNKNOWN_INSTRUCTION
        mov rax, 1
        mov rsp, rbp
        pop rbp
        retn

;;; rdi: `struct AsmCtx *ctx`
;;; rsi: `uint64_t encoded_instruction`
.testinst:
        push rbp
        mov rbp, rsp
        sub rsp, 16
        mov [rbp - 8], rdi

        mov rcx, rsi
        mov rsi, [rdi]          ; rsi = (char *)*(ctx->assembly)
        mov [rbp - 16], rsi

.testinst_loop:
        cmp cl, 0
        je .testinst_end_of_token
        cmp [rsi], cl
        jne .testinst_false
        inc rsi
        shr rcx, 8
        jmp .testinst_loop

.testinst_end_of_token:
        mov [rbp - 16], rsi
        mov rdi, rsi
        call isopdlm
        cmp eax, 0
        je .testinst_false
        mov rdi, [rbp - 8]
        mov rsi, [rbp - 16]
        mov [rdi], rsi
        jmp .testinst_ret

.testinst_false:
        mov rdi, [rbp - 8]
        xor rax, rax

.testinst_ret:
        mov rsp, rbp
        pop rbp
        retn

.assemble:
        cmp rax, 0
        jne .ret_err
        mov rdi, [rbp - 8]
        lea rsi, [rbp - 32]
        call assemble_op

.end:
        xor rax, rax
.ret_err:
        mov rsp, rbp
        pop rbp
        retn

;;; rdi: `struct AsmCtx *ctx`
;;; rsi: `struct AsmOp *op`
as_add:
        push rbp
        mov rbp, rsp
        sub rsp, 16

        mov edx, 0x04           ; uint8_t op_al_imm8 = 0x04
        mov ecx, 0x80           ; uint8_t op_rimm8 = 0x80
        mov r8d, 0x83           ; uint8_t op_rsimm8 = 0x83
        mov r9d, 0x00           ; uint8_t op_rmr8 = 0x00
        mov eax, 0x02
        mov [rbp - 16], rax     ; uint8_t op_rrm8 = 0x02
        xor eax, eax
        mov [rbp - 8], rax      ; uint8_t modrm_mod = 0
        call as_genop2ax32

        mov rsp, rbp
        pop rbp
        retn

;;; rdi: `struct AsmCtx *ctx`
;;; rsi: `struct AsmOp *op`
as_and:
        push rbp
        mov rbp, rsp
        sub rsp, 16

        mov edx, 0x24           ; uint8_t op_al_imm8 = 0x24
        mov ecx, 0x80           ; uint8_t op_rimm8 = 0x80
        mov r8d, 0x83           ; uint8_t op_rsimm8 = 0x83
        mov r9d, 0x20           ; uint8_t op_rmr8 = 0x20
        mov eax, 0x22
        mov [rbp - 16], rax     ; uint8_t op_rrm8 = 0x22
        mov eax, 4
        mov [rbp - 8], rax      ; uint8_t modrm_mod = 4
        call as_genop2ax32

        mov rsp, rbp
        pop rbp
        retn

;;; rdi: `struct AsmCtx *ctx`
;;; rsi: `struct AsmOp *op`
as_call:
        mov dx, 0xe8
        mov cl, 1
        call as_genjmp
        retn

;;; rdi: `struct AsmCtx *ctx`
;;; rsi: `struct AsmOp *op`
as_cmp:
        push rbp
        mov rbp, rsp
        sub rsp, 16

        mov edx, 0x3c           ; uint8_t op_al_imm8 = 0x3c
        mov ecx, 0x80           ; uint8_t op_rimm8 = 0x80
        mov r8d, 0x83           ; uint8_t op_rsimm8 = 0x83
        mov r9d, 0x38           ; uint8_t op_rmr8 = 0x38
        mov eax, 0x3a
        mov [rbp - 16], rax     ; uint8_t op_rrm8 = 0x3a
        mov eax, 7
        mov [rbp - 8], rax     ; uint8_t modrm_mod = 7
        call as_genop2ax32

        mov rsp, rbp
        pop rbp
        retn

;;; rdi: `struct AsmCtx *ctx`
;;; rsi: `struct AsmOp *op`
as_dec:
        mov dl, 0xfe
        mov ecx, 1
        call as_genop1rm
        retn

;;; rdi: `struct AsmCtx *ctx`
;;; rsi: `struct AsmOp *op`
as_div:
        mov dl, 0xf6
        mov ecx, 6
        call as_genop1rm
        retn

;;; rdi: `struct AsmCtx *ctx`
;;; rsi: `struct AsmOp *op`
;;; rdx: `uint16_t opcodes`
;;; rcx: `uint8_t n_opcodes`
as_genjmp:
        push rbp
        mov rbp, rsp
        sub rsp, 32
        mov [rbp - 8], rdi
        mov [rbp - 16], rsi
        mov [rbp - 17], dl

        mov al, 0x01
        mov [rsi], al           ; op->encoding = ENCODING_D
        mov al, 32
        mov [rsi + 1], al       ; op->op_size = 32
        mov [rsi + 5], cl       ; op->n_opcodes = n_opcodes
        mov [rsi + 6], dx       ; op->opcodes[0] = opcodes & 0xff
                                ; op->opcodes[1] = opcodes >> 4
        mov al, 32
        mov [rsi + 9], al       ; op->imm_size = 32
        mov al, 2
        mov [rsi + 10], al      ; op->d_label = D_LABEL_RELATIVE

        call skp2lbinst

        mov rdi, [rbp - 8]
        mov rdi, [rdi]          ; char const *assembly = ctx->assembly
        mov rsi, 1              ; uint8_t n_ops = 1
        call ckoptps

        cmp ax, 3               ; if (chkopts(ctx->assmebly) != OP_TYPE_LBL
        jne .ret_invalid_operands

        mov rdi, [rbp - 8]
        call strlbl

        xor eax, eax            ; Return ERR_NONE
        mov rsp, rbp
        pop rbp
        retn

.ret_invalid_operands:
        mov eax, 2              ; Return ERR_INVALID_OPERANDS
        mov rsp, rbp
        pop rbp
        retn

;;; rdi: `struct AsmCtx *ctx`
;;; rsi: `struct AsmOp *op`
;;; rdx: `uint8_t op_rm8`
;;; rcx: `uint8_t modrm_reg`
as_genop1rm:
        push rbp
        mov rbp, rsp
        sub rsp, 32
        mov [rbp - 8], rdi
        mov [rbp - 16], rsi
        mov [rbp - 17], dl

        mov al, 0x03
        mov [rsi], al           ; op->encoding = ENCODING_M
        mov [rsi + 2], cl       ; op->src_reg = modrm_reg
        mov al, 0b11
        mov [rsi + 4], al       ; op->modrm_mod = MOD_DIRECT
        mov al, 1
        mov [rsi + 5], al       ; op->n_opcodes = 1

        call skp2lbinst

        mov rdi, [rbp - 8]
        mov rdi, [rdi]          ; char const *assembly = ctx->assembly
        mov rsi, 1              ; uint8_t n_ops = 1
        call ckoptps

        cmp ax, 0               ; if (chkopts(ctx->assmebly) != OP_TYPE_REG
        jne .ret_invalid_operands

        mov rdi, [rbp - 8]
        mov rdi, [rdi]          ; char *assembly = ctx->assembly
        mov rsi, 1              ; uint8_t n_ops = 1
        call ckopsize
        mov rsi, [rbp - 16]
        mov [rsi + 1], al       ; op->op_size = al
        cmp al, 8
        je .op8
        cmp al, 16
        je .op16
        jmp .op_gt_16

.op8:
        mov rsi, [rbp - 16]
        mov al, [rbp - 17]
        mov [rsi + 6], al       ; op->opcodes[0] = op_rm8
        jmp .parse_reg

.op16:
        mov rsi, [rbp - 16]
        mov al, [rbp - 17]
        inc al
        mov [rsi + 6], al       ; op->opcodes[0] = op_rm8
        mov al, 0x08
        mov [rsi + 11], al      ; op->prefix = PREFIX_OP_SIZE_OVERRIDE
        jmp .parse_reg

.op_gt_16:
        mov rsi, [rbp - 16]
        mov al, [rbp - 17]
        inc al
        mov [rsi + 6], al       ; op->opcodes[0] = op_rm8
        jmp .parse_reg

.parse_reg:
        mov rdi, [rbp - 8]
        call preg
        mov rsi, [rbp - 16]
        mov [rsi + 3], al       ; op->dst_reg = preg(&(ctx->assembly))

        xor eax, eax            ; Return ERR_NONE
        mov rsp, rbp
        pop rbp
        retn

.ret_invalid_operands:
        mov eax, 2              ; Return ERR_INVALID_OPERANDS
        mov rsp, rbp
        pop rbp
        retn

;;; rdi: `struct AsmCtx *ctx`
;;; rsi: `struct AsmOp *op`
;;; rdx: `uint8_t op_al_imm8`
;;; rcx: `uint8_t op_rimm8`
;;; r8: `uint8_t op_rsimm8`
;;; r9: `uint8_t op_rmr8`
;;; [rbp + 16]: `uint8_t op_rrm8`
;;; [rbp + 24]: `uint8_t modrm_reg`
as_genop2ax32:
        push rbp
        mov rbp, rsp
        sub rsp, 32
        mov [rbp - 8], rdi
        mov [rbp - 16], rsi
        mov [rbp - 17], dl
        mov [rbp - 18], cl
        mov [rbp - 19], r8b
        mov [rbp - 20], r9b
        mov al, [rbp + 16]
        mov [rbp - 21], al
        mov al, [rbp + 24]
        mov [rbp - 22], al

        call skp2lbinst

        mov rdi, [rbp - 8]
        mov rdi, [rdi]          ; char const *assembly = ctx->assembly
        mov rsi, 2              ; uint8_t n_ops = 2
        call ckoptps

        cmp ax, 2               ; (OP_TYPE_REG << 4) | OP_TYPE_IMM
        jne .check_rmr

        ;; Peak register
        mov rdi, [rbp - 8]
        mov r8, [rdi]
        mov [rbp - 32], r8
        lea rdi, [rbp - 32]
        call preg
        cmp rax, 0              ; if (preg(...) != ax)
        jne .rimm

        mov rdi, [rbp - 8]      ; struct AsmCtx *ctx = ctx
        mov rsi, [rbp - 16]     ; struct AsmOp *op = op
        mov dl, [rbp - 17]      ; uint8_t op8 = op_al_imm8
        call genop2aximm32
        jmp .end
.rimm:
        mov rdi, [rbp - 8]      ; struct AsmCtx *ctx = ctx
        mov rsi, [rbp - 16]     ; struct AsmOp *op = op
        mov dl, [rbp - 18]      ; uint8_t op8 = op_rimm8
        mov cl, [rbp - 19]      ; uint8_t op_simm8 = op_rsimm8
        mov r8b, [rbp - 22]     ; uint8_t modrm_reg = modrm_reg
        call genop2rimm32
        jmp .end

.check_rmr:
        mov rdi, [rbp - 8]      ; struct AsmCtx *ctx = ctx
        mov rsi, [rbp - 16]     ; struct AsmOp *op = op
        mov dl, [rbp - 20]      ; uint8_t op8 = op_rmr8

        cmp ax, 0b00010000      ; (OP_TYPE_RGNDRCT << 4) | OP_TYPE_REG
        jne .check_rr
        call genop2rmr
        jmp .end
.check_rr:
        cmp ax, 0               ; (OP_TYPE_REG << 4) | OP_TYPE_REG
        jne .check_rrm
        call genop2rmr
        jmp .end
.check_rrm:
        mov dl, [rbp - 21]      ; uint8_t op8 = op_rrm8
        cmp ax, 1               ; (OP_TYPE_REG << 4) | OP_TYPE_RGNDRCT
        jne .ret_invalid_operands
        call genop2rrm
        jmp .end

.ret_invalid_operands:
        ;; Combination of operands is not supported (yet).
        mov eax, 2              ; Return ERR_INVALID_OPERANDS
        mov rsp, rbp
        pop rbp
        retn

.end:
        xor eax, eax            ; Return ERR_NONE
        mov rsp, rbp
        pop rbp
        retn

;;; rdi: `struct AsmCtx *ctx`
;;; rsi: `struct AsmOp *op`
as_inc:
        mov dl, 0xfe
        xor ecx, ecx
        call as_genop1rm
        retn

;;; rdi: `struct AsmCtx *ctx`
;;; rsi: `struct AsmOp *op`
as_int:
        push rbp
        mov rbp, rsp
        sub rsp, 16
        mov [rbp - 8], rdi
        mov [rbp - 16], rsi

        mov al, 0x02
        mov [rsi], al           ; op->encoding = ENCODING_I
        mov al, 1
        mov [rsi + 5], al       ; op->n_opcodes = 1
        mov al, 0xcd
        mov [rsi + 6], al       ; op->opcodes[0] = 0xcd
        mov al, 8
        mov [rsi + 9], al       ; op->imm_size = 8

        call skp2lbinst

        mov rdi, [rbp - 8]
        mov rdi, [rdi]          ; char const *assembly = ctx->assembly
        mov rsi, 1              ; uint8_t n_ops = 1
        call ckoptps

        cmp ax, 2               ; OP_TYPE_IMM
        jne .ret_invalid_operand

        mov rdi, [rbp - 8]
        call pint
        mov rsi, [rbp - 16]
        mov [rsi + 16], al      ; op->imm.imm8 = pint(ctx->assembly)

        xor eax, eax            ; Return ERR_NONE
        mov rsp, rbp
        pop rbp
        retn

.ret_invalid_operand:
        mov eax, 2              ; Return ERR_INVALID_OPERANDS
        mov rsp, rbp
        pop rbp
        retn

;;; rdi: `struct AsmCtx *ctx`
;;; rsi: `struct AsmOp *op`
as_ja:
        mov dx, 0x870f
        mov cl, 2               ; uint8_t n_opcodes = 2
        call as_genjmp
        retn

;;; rdi: `struct AsmCtx *ctx`
;;; rsi: `struct AsmOp *op`
as_jb:
        mov dx, 0x820f
        mov cl, 2               ; uint8_t n_opcodes = 2
        call as_genjmp
        retn

;;; rdi: `struct AsmCtx *ctx`
;;; rsi: `struct AsmOp *op`
as_je:
        mov dx, 0x840f
        mov cl, 2               ; uint8_t n_opcodes = 2
        call as_genjmp
        retn

;;; rdi: `struct AsmCtx *ctx`
;;; rsi: `struct AsmOp *op`
as_jmp:
        mov dx, 0xe9
        mov cl, 1
        call as_genjmp
        retn

;;; rdi: `struct AsmCtx *ctx`
;;; rsi: `struct AsmOp *op`
as_jne:
        mov dx, 0x850f
        mov cl, 2               ; uint8_t n_opcodes = 2
        call as_genjmp
        retn

;;; rdi: `struct AsmCtx *ctx`
;;; rsi: `struct AsmOp *op`
as_lea:
        push rbp
        mov rbp, rsp
        sub rsp, 16
        mov [rbp - 8], rdi
        mov [rbp - 16], rsi

        call skp2lbinst

        mov rdi, [rbp - 8]
        mov rdi, [rdi]          ; char const *assembly = ctx->assembly
        mov rsi, 2              ; uint8_t n_ops = 2
        call ckoptps

        cmp ax, 1               ; (OP_TYPE_REG << 4) | OP_TYPE_RGNDRCT
        jne .ret_invalid_operand

        ;; Call to genop2rrm is a bit of a hack here, as `lea` has no 8-bit
        ;; opcode, but this still works
        mov rdi, [rbp - 8]      ; struct AsmCtx *ctx = ctx
        mov rsi, [rbp - 16]     ; struct AsmOp *op = op
        mov dl, 0x8c            ; uint8_t op8 = 0x8c
        call genop2rrm

        xor eax, eax            ; Return ERR_NONE
        mov rsp, rbp
        pop rbp
        retn

.ret_invalid_operand:
        mov eax, 2              ; Return ERR_INVALID_OPERANDS
        mov rsp, rbp
        pop rbp
        retn

;;; rdi: `struct AsmCtx *ctx`
;;; rsi: `struct AsmOp *op`
as_mov:
        push rbp
        mov rbp, rsp
        sub rsp, 16
        mov [rbp - 8], rdi
        mov [rbp - 16], rsi

        mov al, 0b11
        mov [rsi + 4], al       ; op->modrm_mod = MOD_DIRECT
        mov al, 1
        mov [rsi + 5], al       ; op->n_opcodes = 1

        call skp2lbinst

        mov rdi, [rbp - 8]
        mov rdi, [rdi]          ; char *assembly = ctx->assembly
        mov rsi, 2              ; uint8_t n_ops = 2
        call ckopsize
        mov rsi, [rbp - 16]
        mov [rsi + 1], al       ; op->op_size = al

        mov rdi, [rbp - 8]
        mov rdi, [rdi]          ; char *assembly = ctx->assembly
        mov rsi, 2              ; uint8_t n_ops = 2
        call ckoptps

        mov rdi, [rbp - 8]      ; struct AsmCtx *ctx = ctx
        mov rsi, [rbp - 16]     ; struct AsmOp *op = op
        mov dl, 0x88            ; uint8_t op8 = 0x88

        cmp ax, 2               ; (OP_TYPE_REG << 4) | OP_TYPE_IMM
        je .mov_r_imm
.check_rmr:
        cmp ax, 0b00010000      ; (OP_TYPE_RGNDRCT << 4) | OP_TYPE_REG
        jne .check_rr
        call genop2rmr
        jmp .end
.check_rr:
        cmp ax, 0               ; (OP_TYPE_REG << 4) | OP_TYPE_REG
        jne .check_rrm
        call genop2rmr
        jmp .end
.check_rrm:
        mov dl, 0x8a            ; uint8_t op8 = 0x8a
        cmp ax, 1               ; (OP_TYPE_REG << 4) | OP_TYPE_RGNDRCT
        jne .ret_invalid_operands
        call genop2rrm
        jmp .end

.mov_r_imm:
        ;; Parse first operand as register
        mov rdi, [rbp - 8]
        call preg
        mov rsi, [rbp - 16]
        mov [rsi + 3], al       ; op->dst_reg = al

        ;; Skip to the next operand
        mov rdi, [rbp - 8]
        call skp2nxtop

        mov rsi, [rbp - 16]
        mov al, 0x07
        mov [rsi], al           ; op->encoding = ENCODING_OI

        ;; Parse second operand as immediate
        mov rdi, [rbp - 8]
        call pint
        mov rsi, [rbp - 16]
        mov cl, [rsi + 1]
        cmp cl, 8               ; if (op->op_size == 8)
        je .mov_r8_imm8
        cmp cl, 16              ; if (op->op_size == 16)
        je .mov_r16_imm16
        cmp cl, 32              ; if (op->op_size == 32)
        je .mov_r32_imm32

.mov_r64_imm64:
        mov [rsi + 16], rax     ; op->imm.imm64 = rax
        mov al, 64
        mov [rsi + 9], al       ; op->imm_size = 64
        mov al, 0xb8
        mov [rsi + 6], al       ; op->opcodes[0] = 0xb8
        jmp .end

.mov_r32_imm32:
        mov [rsi + 16], eax     ; op->imm.imm32 = eax
        mov al, 32
        mov [rsi + 9], al       ; op->imm_size = 32
        mov al, 0xb8
        mov [rsi + 6], al       ; op->opcodes[0] = 0xb8
        jmp .end

.mov_r16_imm16:
        mov [rsi + 16], ax      ; op->imm.imm16 = ax
        mov al, 16
        mov [rsi + 9], al       ; op->imm_size = 16
        mov al, 0xb8
        mov [rsi + 6], al       ; op->opcodes[0] = 0xb8
        mov al, 0x08
        mov [rsi + 11], al      ; op->prefix = PREFIX_OP_SIZE_OVERRIDE
        jmp .end

.mov_r8_imm8:
        mov [rsi + 16], al      ; op->imm.imm8 = al
        mov al, 8
        mov [rsi + 9], al       ; op->imm_size = 8
        mov al, 0xb0
        mov [rsi + 6], al       ; op->opcodes[0] = 0xb0
        jmp .end

.end:
        xor eax, eax            ; Return ERR_NONE
        mov rsp, rbp
        pop rbp
        retn

.ret_invalid_operands:
        mov eax, 2              ; Return ERR_INVALID_OPERANDS
        mov rsp, rbp
        pop rbp
        retn

;;; rdi: `struct AsmCtx *ctx`
;;; rsi: `struct AsmOp *op`
as_mul:
        mov dl, 0xf6
        mov ecx, 4
        call as_genop1rm
        retn

;;; rdi: `struct AsmCtx *ctx`
;;; rsi: `struct AsmOp *op`
as_nop:
        mov al, 0
        mov [rsi], al           ; op->encoding = ENCODING_ZO
        mov al, 1
        mov [rsi + 5], al       ; op->n_opcodes = 1
        mov al, 0x90
        mov [rsi + 6], al       ; op->opcodes[0] = 0x90

        xor eax, eax            ; Return ERR_NONE
        retn

;;; rdi: `struct AsmCtx *ctx`
;;; rsi: `struct AsmOp *op`
as_or:
        push rbp
        mov rbp, rsp
        sub rsp, 16

        mov edx, 0x0c           ; uint8_t op_al_imm8 = 0x0c
        mov ecx, 0x80           ; uint8_t op_rimm8 = 0x80
        mov r8d, 0x83           ; uint8_t op_rsimm8 = 0x83
        mov r9d, 0x08           ; uint8_t op_rmr8 = 0x08
        mov eax, 0x0a
        mov [rbp - 16], rax     ; uint8_t op_rrm8 = 0x0a
        mov eax, 1
        mov [rbp - 8], rax      ; uint8_t modrm_mod = 4
        call as_genop2ax32

        mov rsp, rbp
        pop rbp
        retn

;;; rdi: `struct AsmCtx *ctx`
;;; rsi: `struct AsmOp *op`
as_pop:
        push rbp
        mov rbp, rsp
        sub rsp, 16
        mov [rbp - 8], rdi
        mov [rbp - 16], rsi

        mov al, 0x06
        mov [rsi], al           ; op->encoding = ENCODING_O
        mov al, 1
        mov [rsi + 5], al       ; op->n_opcodes = 1
        mov al, 0x58
        mov [rsi + 6], al       ; op->opcodes[0] = 0x58

        ;; Skip to next token - the register
        call skp2lbinst

        mov rdi, [rbp - 8]
        mov rdi, [rdi]          ; char const *assembly = ctx->assembly
        mov rsi, 1              ; uint8_t n_ops = 1
        call ckoptps

        cmp ax, 0               ; OP_TYPE_REG
        jne .ret_invalid_operand

        mov rdi, [rbp - 8]
        call preg
        mov rsi, [rbp - 16]
        mov [rsi + 3], al       ; op->dst_reg = al

        xor eax, eax            ; Return ERR_NONE
        mov rsp, rbp
        pop rbp
        retn

.ret_invalid_operand:
        mov eax, 2              ; Return ERR_INVALID_OPERANDS
        mov rsp, rbp
        pop rbp
        retn

;;; rdi: `struct AsmCtx *ctx`
;;; rsi: `struct AsmOp *op`
as_push:
        push rbp
        mov rbp, rsp
        sub rsp, 16
        mov [rbp - 8], rdi
        mov [rbp - 16], rsi

        call as_pop
        mov rsi, [rbp - 16]
        mov cl, 0x50
        mov [rsi + 6], cl       ; op->opcodes[0] = 0x50

        mov rsp, rbp
        pop rbp
        retn

;;; rdi: `struct AsmCtx *ctx`
;;; rsi: `struct AsmOp *op`
as_retn:
        mov al, 0
        mov [rsi], al           ; op->encoding = ENCODING_ZO
        mov al, 1
        mov [rsi + 5], al       ; op->n_opcodes = 1
        mov al, 0xc3
        mov [rsi + 6], al       ; op->opcodes[0] = 0xc3

        xor eax, eax            ; Return ERR_NONE
        retn

;;; rdi: `struct AsmCtx *ctx`
;;; rsi: `struct AsmOp *op`
as_syscall:
        mov al, 0
        mov [rsi], al           ; op->encoding = ENCODING_ZO
        mov al, 2
        mov [rsi + 5], al       ; op->n_opcodes = 2
        mov al, 0x0f
        mov [rsi + 6], al       ; op->opcodes[0] = 0x0f
        mov al, 0x05
        mov [rsi + 7], al       ; op->opcodes[1] = 0x05

        xor eax, eax            ; Return ERR_NONE
        retn

;;; rdi: `struct AsmCtx *ctx`
;;; rsi: `struct AsmOp *op`
as_shl:
        push rbp
        mov rbp, rsp
        sub rsp, 16
        mov [rbp - 8], rdi
        mov [rbp - 16], rsi

        mov al, 0x04
        mov [rsi], al           ; op->encoding = ENCODING_MI
        mov al, 4
        mov [rsi + 2], al       ; op->src_reg = 4
        mov al, 0b11
        mov [rsi + 4], al       ; op->modrm_mod = MOD_DIRECT
        mov al, 1
        mov [rsi + 5], al       ; op->n_opcodes = 1
        mov al, 8
        mov [rsi + 9], al       ; op->imm_size = 8


        ;; Skip to next token - the register
        call skp2lbinst

        mov rdi, [rbp - 8]
        mov rdi, [rdi]          ; char *assembly = ctx->assembly
        mov rsi, 1              ; uint8_t n_ops = 1
        call ckopsize
        mov rsi, [rbp - 16]
        mov [rsi + 1], al       ; op->op_size = al

        mov rdi, [rbp - 8]
        mov rdi, [rdi]          ; char const *assembly = ctx->assembly
        mov rsi, 2              ; uint8_t n_ops = 2
        call ckoptps

        cmp ax, 2               ; (OP_TYPE_REG << 4) | OP_TYPE_IMM
        jne .ret_invalid_operands

        mov rdi, [rbp - 8]      ; char **assembly = &ctx->assembly
        call preg
        mov rsi, [rbp - 16]
        mov [rsi + 3], al       ; op->dst_reg = preg(&ctx->assembly)

        mov rdi, [rbp - 8]      ; char **assembly = &ctx->assembly
        call skp2nxtop

        mov rdi, [rbp - 8]      ; char **assembly = &ctx->assembly
        call pint
        mov rsi, [rbp - 16]
        mov [rsi + 16], al      ; op->imm.imm8 = pint(&ctx->assembly)

        cmp al, 1               ; if (op->imm.imm8 == 1)
        je .shift1

        mov al, [rsi + 1]
        cmp al, 8               ; if (op->op_size == 8)
        je .op8
        cmp al, 16              ; if (op->op_size == 16)
        je .op16
        jmp .op32_64

.shift1:
        mov al, 0x03
        mov [rsi], al           ; op->encoding = ENCODING_M
        mov al, [rsi + 1]
        cmp al, 8               ; if (op->op_size == 8)
        je .shift1_op8
        cmp al, 16              ; if (op->op_size == 16)
        je .shift1_op16
        jmp .shift1_op32_64

.shift1_op8:
        mov al, 0xd0
        mov [rsi + 6], al       ; op->opcodes[0] = 0xd0
        jmp .end

.shift1_op16:
        mov al, 0xd1
        mov [rsi + 6], al       ; op->opcodes[0] = 0xd1
        mov al, 0x08
        mov [rsi + 11], al      ; op->prefix = PREFIX_OP_SIZE_OVERRIDE
        jmp .end

.shift1_op32_64:
        mov al, 0xd1
        mov [rsi + 6], al       ; op->opcodes[0] = 0xd1
        jmp .end

.op8:
        mov al, 0xc0
        mov [rsi + 6], al       ; op->opcodes[0] = 0xc0
        jmp .end

.op16:
        mov al, 0xc1
        mov [rsi + 6], al       ; op->opcodes[0] = 0xc1
        mov al, 0x08
        mov [rsi + 11], al      ; op->prefix = PREFIX_OP_SIZE_OVERRIDE
        jmp .end

.op32_64:
        mov al, 0xc1
        mov [rsi + 6], al       ; op->opcodes[0] = 0xc1

.end:
        xor eax, eax            ; Return ERR_NONE
        mov rsp, rbp
        pop rbp
        retn

.ret_invalid_operands:
        mov eax, 2              ; Return ERR_INVALID_OPERANDS
        mov rsp, rbp
        pop rbp
        retn

;;; rdi: `struct AsmCtx *ctx`
;;; rsi: `struct AsmOp *op`
as_shr:
        push rbp
        mov rbp, rsp
        sub rsp, 16
        mov [rbp - 8], rdi
        mov [rbp - 16], rsi

        call as_shl
        mov rsi, [rbp - 16]
        mov cl, 5
        mov [rsi + 2], cl       ; op->src_reg = 5

        mov rsp, rbp
        pop rbp
        retn

;;; rdi: `struct AsmCtx *ctx`
;;; rsi: `struct AsmOp *op`
as_sub:
        push rbp
        mov rbp, rsp
        sub rsp, 16

        mov edx, 0x2c           ; uint8_t op_al_imm8 = 0x2c
        mov ecx, 0x80           ; uint8_t op_rimm8 = 0x80
        mov r8d, 0x83           ; uint8_t op_rsimm8 = 0x83
        mov r9d, 0x28           ; uint8_t op_rmr8 = 0x28
        mov eax, 0x2a
        mov [rbp - 16], rax     ; uint8_t op_rrm8 = 0x2a
        mov eax, 5
        mov [rbp - 8], rax      ; uint8_t modrm_mod = 5
        call as_genop2ax32

        mov rsp, rbp
        pop rbp
        retn

;;; rdi: `struct AsmCtx *ctx`
;;; rsi: `struct AsmOp *op`
as_xchg:
        push rbp
        mov rbp, rsp
        sub rsp, 32
        mov [rbp - 8], rdi
        mov [rbp - 16], rsi
        xor eax, eax
        mov [rbp - 18], ax      ; uint16_t op_tps = 0

        call skp2lbinst

        mov rdi, [rbp - 8]
        mov rdi, [rdi]          ; char *assembly = ctx->assembly
        mov rsi, 2              ; uint8_t n_ops = 2
        call ckoptps
        mov [rbp - 18], ax      ; op_tps = ckoptps(ctx->assembly, 2)

        cmp ax, 0               ; (OP_TYPE_REG << 4) | OP_TYPE_REG
        je .xchg_rrgndrct
        cmp ax, 1               ; (OP_TYPE_REG << 4) | OP_TYPE_RGNDRCT
        je .xchg_rrgndrct
        cmp ax, 0b00010000      ; (OP_TYPE_RGNDRCT << 4) | OP_TYPE_REG
        je .xchg_rgndrctr
        jmp .ret_invalid_operands

.xchg_rgndrctr:
        mov rdi, [rbp - 8]      ; struct AsmCtx *ctx = ctx
        mov rsi, [rbp - 16]     ; struct AsmOp *op = op
        mov dl, 0x86            ; uint8_t op8 = 0x86
        call genop2rmr
        jmp .check_rax

.xchg_rrgndrct:
        mov rdi, [rbp - 8]      ; struct AsmCtx *ctx = ctx
        mov rsi, [rbp - 16]     ; struct AsmOp *op = op
        mov dl, 0x86            ; uint8_t op8 = 0x86
        call genop2rrm
        jmp .check_rax

.ret_invalid_operands:
        mov eax, 2              ; Return ERR_INVALID_OPERANDS
        mov rsp, rbp
        pop rbp
        retn

.check_rax:
        mov rsi, [rbp - 16]
        mov al, [rsi + 1]
        ;; Special xchg encoding for ax/rax only covers 16-bit upwards operation
        cmp al, 8               ; if (op->op_size == 8)
        je .ret_success
        ;; I don't think, this is necessary but it increases the compatibility
        ;; with nasm
        cmp al, 32              ; if (op->op_size == 32)
        je .ret_success
        mov ax, [rbp - 18]
        cmp ax, 0               ; if (op_tps != (OP_TYPE_REG << 4) | OP_TYPE_REG)
        jne .ret_success
        mov al, [rsi + 2]
        cmp al, 0               ; if (op->src_reg == 0) // ax/eax/rax
        je .xchg_rax
        mov al, [rsi + 3]
        cmp al, 0               ; if (op->dst_reg == 0) // ax/eax/rax
        je .xchg_rax
        jmp .ret_success

.xchg_rax:
        mov al, 6
        mov [rsi], al           ; op->encoding = ENCODING_O
        mov al, 0x90
        mov [rsi + 6], al       ; op->opcodes[0] = 0x90
        mov al, [rsi + 3]
        cmp al, 0               ; if (op->dst_reg != 0)
        jne .ret_success
        mov al, [rsi + 2]
        mov [rsi + 3], al       ; op->dst_reg = op->src_reg
        xor eax, eax
        mov [rsi + 2], al       ; op->src_reg = 0

.ret_success:
        xor eax, eax            ; Return `ERR_NONE`
        mov rsp, rbp
        pop rbp
        retn

;;; rdi: `struct AsmCtx *ctx`
;;; rsi: `struct AsmOp *op`
as_xor:
        push rbp
        mov rbp, rsp
        sub rsp, 16

        mov edx, 0x34           ; uint8_t op_al_imm8 = 0x34
        mov ecx, 0x80           ; uint8_t op_rimm8 = 0x80
        mov r8d, 0x83           ; uint8_t op_rsimm8 = 0x83
        mov r9d, 0x30           ; uint8_t op_rmr8 = 0x30
        mov eax, 0x32
        mov [rbp - 16], rax     ; uint8_t op_rrm8 = 0x32
        mov eax, 6
        mov [rbp - 8], rax      ; uint8_t modrm_mod = 4
        call as_genop2ax32

        mov rsp, rbp
        pop rbp
        retn

cklb:
        push rbp
        mov rbp, rsp
        push rdi
        mov rbp, rsp
        xor rax, rax
        mov bl, 0xa             ; Ascii linebreak ('\n')
        mov bh, 0               ; Null terminator ('\0')
        mov cl, 0x3b            ; Ascii semicolon (';')
        mov ch, 0x3a            ; Ascii colon (':')

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
        mov rax, 1              ; Return a 1

.end:
        pop rdi
        pop rbp
        retn

;;; rdi: `char const *assembly`
;;; rsi: `char const *cpos`
ckln:
        mov rax, 1              ; size_t line = 1
.loop:
        cmp rdi, rsi
        je .end
        mov cl, [rdi]
        cmp cl, 0xa             ; if (*assembly != '\n')
        jne .loop_next
        inc rax
.loop_next:
        inc rdi
        jmp .loop
.end:
        retn

;;; rdi: `char *assembly`
;;; rsi: `uint8_t n_ops`
ckopsize:
        push rbp
        mov rbp, rsp
        sub rsp, 16

        xor eax, eax
        mov [rbp - 8], rdi
        mov rcx, rsi
        mov [rbp - 9], cl
        mov [rbp - 10], al      ; uint8_t op_size = 0

.check_op_loop:
        lea rdi, [rbp - 8]
        call skp2lbinst
        mov rdi, [rbp - 8]
        call isrgndrct
        cmp al, 1
        je .check_next_op
        mov rdi, [rbp - 8]
        call isint
        cmp al, 1
        je .check_next_op
        mov rdi, [rbp - 8]
        call isreg
        cmp al, 1
        jne .check_next_op

        lea rdi, [rbp - 8]
        call pr8
        cmp al, 0xff
        je .check_op_size_16
        mov al, [rbp - 10]
        cmp al, 8
        ja .check_next_op
        mov al, 8
        mov [rbp - 10], al
        jmp .check_next_op

.check_op_size_16:
        lea rdi, [rbp - 8]
        call pr16
        cmp al, 0xff
        je .check_op_size_32
        mov al, [rbp - 10]
        cmp al, 16
        ja .check_next_op
        mov al, 16
        mov [rbp - 10], al
        jmp .check_next_op

.check_op_size_32:
        lea rdi, [rbp - 8]
        call pr32
        cmp al, 0xff
        je .check_op_size_64
        mov al, [rbp - 10]
        cmp al, 32
        ja .check_next_op
        mov al, 32
        mov [rbp - 10], al
        jmp .check_next_op

.check_op_size_64:
        lea rdi, [rbp - 8]
        call pr64
        cmp al, 0xff
        je .check_next_op
        mov al, 64
        mov [rbp - 10], al

.check_next_op:
        mov cl, [rbp - 9]
        dec cl
        jz .end
        mov [rbp - 9], cl
        mov rdi, [rbp - 8]
        mov al, 0x2c            ; Ascii comma
.check_comma_loop:
        cmp [rdi], al
        je .check_comma_loop_end
        inc rdi
        jmp .check_comma_loop

.check_comma_loop_end:
        inc rdi
        mov [rbp - 8], rdi
        jmp .check_op_loop

.end:
        xor eax, eax
        mov al, [rbp - 10]
        mov rsp, rbp
        pop rbp
        retn

;;; rdi: `char const *assembly`
;;; rsi: `uint8_t n_ops`
ckoptps:
        push rbp
        mov rbp, rsp
        sub rsp, 24
        mov [rbp - 8], rdi
        mov rax, rsi
        mov [rbp - 9], al
        xor eax, eax
        mov [rbp - 10], al     ; uint8_t i = 0
        mov [rbp - 12], ax     ; uint16_t op_types = 0

        lea rdi, [rbp - 8]
        call skp2lbinst

.check_op:
        mov rdi, [rbp - 8]
        call isreg
        cmp rax, 1
        jne .check_rgndrct
        lea rdi, [rbp - 8]
        call preg
        mov dx, 0               ; dx = OP_TYPE_REG
        jmp .check_op_end
.check_rgndrct:
        mov rdi, [rbp - 8]
        call isrgndrct
        cmp rax, 1
        jne .check_int
        lea rdi, [rbp - 8]      ; char const **assembly = &assembly
        lea rsi, [rbp - 16]
        lea rdx, [rbp - 16]
        call prgndrct
        mov dx, 1               ; dx = OP_TYPE_RGNDRCT
        jmp .check_op_end
.check_int:
        mov rdi, [rbp - 8]      ; char const **assembly = &assembly
        call isint
        cmp rax, 1
        jne .check_label
        lea rdi, [rbp - 8]
        call pint
        mov dx, 2               ; dx = OP_TYPE_IMM
        jmp .check_op_end
.check_label:
        lea rdi, [rbp - 8]      ; char const **assembly = &assembly
        xor rsi, rsi            ; char *label = NULL
        xor rdx, rdx            ; size_t n = 0
        call readnlbl
        mov dx, 3               ; dx = OP_TYPE_LBL
.check_op_end:
        mov ax, [rbp - 12]
        shl ax, 4
        or ax, dx
        mov [rbp - 12], ax      ; op_types = (op_types << 4) | dx
        mov cl, [rbp - 10]
        inc cl
        mov [rbp - 10], cl      ; i++
        mov al, [rbp - 9]
        cmp al, cl              ; if (i == n_ops)
        je .ret
        lea rdi, [rbp - 8]      ; char const **assembly = &assembly
        call skp2nxtop
        jmp .check_op

.ret:
        xor eax, eax
        mov ax, [rbp - 12]
        mov rsp, rbp
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
        dec rcx
        jnz .loop
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

elf64_clcshstrtabsz:
        ;; >>> len("\0.text\0.shstrtab\0.symtab\0.strtab\0")
        ;; 33
        mov rax, 33
        retn

;;; rdi: `struct AsmCtx *ctx`
;;; rsi: `char *filename`
elf64_clcstrtabsz:
        push rbp
        mov rbp, rsp
        sub rsp, 24

        mov [rbp - 8], rdi
        xor eax, eax
        mov [rbp - 24], rax     ; size_t i = 0
        inc rax                 ; Null byte at the start
        mov [rbp - 16], rax     ; size_t n = 1

        mov rdi, rsi
        call len
        mov rdi, [rbp - 16]
        add rax, rdi
        mov [rbp - 16], rax     ; n += len(filename)

.symtab_loop:
        mov rdi, [rbp - 8]
        mov rsi, [rdi + 40]     ; rsi = ctx->max_symtab_entries
        mov rcx, [rbp - 24]     ; rcx = i
        cmp rcx, rsi            ; if (ctx->max_symtab_entries == i)
        je .ret
        mov rax, 256
        mul rcx
        mov rsi, [rdi + 32]
        add rsi, rax            ; rsi = ctx->symtab[i].label
        mov al, 0
        cmp [rsi], al           ; if (ctx->symtab[i].label[0] == 0)
        je .ret
        mov rdi, rsi
        call len
        mov rcx, [rbp - 16]
        add rcx, rax
        mov [rbp - 16], rcx
        mov rcx, [rbp - 24]
        inc rcx
        mov [rbp - 24], rcx
        jmp .symtab_loop

.ret:
        mov rax, [rbp - 16]
        mov rsp, rbp
        pop rbp
        retn

;;; rdi: `struct AsmCtx *ctx`
elf64_clcsymtabsz:
        mov rsi, [rdi + 40]     ; size_t n = ctx->max_symtab_entries
        mov rdi, [rdi + 32]     ; struct SymTabNtr *symtab = ctx->symtab
        call symtablen
        add rax, 3              ; Null entry, file entry, text section entry
        mov rcx, 0x18           ; sizeof(Elf64_Sym)
        mul rcx                 ; multiply by 0x18 bytes per symtab entry in the
                                ; ELF file.
        retn

;;; rdi: `struct AsmCtx *ctx`
elf64_clctextsz:
        mov rax, [rdi + 24]
        retn

;;; rdi: `struct AsmCtx *ctx`
;;; rsi: `void *buffer`
;;; rdx: `size_t n`
;;; rcx: `char *filename`
elf64_dump:
        push rbp
        mov rbp, rsp
        sub rsp, 40
        mov [rbp - 8], rdi
        mov [rbp - 16], rsi
        mov [rbp - 24], rdx
        mov [rbp - 32], rcx
        xor eax, eax
        mov [rbp - 40], rax     ; size_t n_bytes

        call elf64_dump_header  ; size_t header_bytes = elf64_dump_header(...)
        cmp rax, 0              ; if (header_bytes == 0)
        je .ret_err
        mov rdx, [rbp - 24]
        sub rdx, rax
        mov [rbp - 24], rdx     ; n -= header_bytes
        mov rsi, [rbp - 16]
        mov rdx, [rbp - 40]
        add rsi, rax
        add rdx, rax
        mov [rbp - 16], rsi     ; buffer += header_bytes
        mov [rbp - 40], rdx     ; n_bytes += header_bytes

        mov rdi, [rbp - 8]
        mov rsi, [rbp - 16]
        mov rdx, [rbp - 24]
        mov rcx, [rbp - 32]
        call elf64_dump_shtab   ; size_t shtab_bytes = elf64_dump_shtab(...)
        cmp rax, 0              ; if (shtab_bytes == 0)
        je .ret_err
        mov rdx, [rbp - 24]
        sub rdx, rax
        mov [rbp - 24], rdx     ; n -= shtab_bytes
        mov rsi, [rbp - 16]
        mov rdx, [rbp - 40]
        add rsi, rax
        add rdx, rax
        mov [rbp - 16], rsi     ; buffer += shtab_bytes
        mov [rbp - 40], rdx     ; n_bytes += shtab_bytes


        mov rdi, [rbp - 8]
        mov rsi, [rbp - 16]
        mov rdx, [rbp - 24]
        call elf64_dump_text    ; size_t bintxt_bytes = elf64_dump_text(...)
        cmp rax, 0              ; if (bintxt_bytes == 0)
        je .ret_err
        mov rdx, [rbp - 24]
        sub rdx, rax
        mov [rbp - 24], rdx     ; n -= bintxt_bytes
        mov rsi, [rbp - 16]
        mov rdx, [rbp - 40]
        add rsi, rax
        add rdx, rax
        mov [rbp - 16], rsi     ; buffer += bintxt_bytes
        mov [rbp - 40], rdx     ; n_bytes += bintxt_bytes

        mov rdi, [rbp - 8]
        mov rsi, [rbp - 16]
        mov rdx, [rbp - 24]
        call elf64_dump_shstrtab; size_t shstrtab_bytes = elf64_dump_text(...)
        cmp rax, 0              ; if (shstrtab_bytes == 0)
        je .ret_err
        mov rdx, [rbp - 24]
        sub rdx, rax
        mov [rbp - 24], rdx     ; n -= shstrtab_bytes
        mov rsi, [rbp - 16]
        mov rdx, [rbp - 40]
        add rsi, rax
        add rdx, rax
        mov [rbp - 16], rsi     ; buffer += shstrtab_bytes
        mov [rbp - 40], rdx     ; n_bytes += shstrtab_bytes

        mov rdi, [rbp - 8]
        mov rsi, [rbp - 16]
        mov rdx, [rbp - 24]
        mov rcx, [rbp - 32]
        call elf64_dump_symtab  ; size_t symtab_bytes = elf64_dump_symtab(...)
        cmp rax, 0              ; if (symtab_bytes == 0)
        je .ret_err
        mov rdx, [rbp - 24]
        sub rdx, rax
        mov [rbp - 24], rdx     ; n -= symtab_bytes
        mov rsi, [rbp - 16]
        mov rdx, [rbp - 40]
        add rsi, rax
        add rdx, rax
        mov [rbp - 16], rsi     ; buffer += symtab_bytes
        mov [rbp - 40], rdx     ; n_bytes += symtab_bytes

        mov rdi, [rbp - 8]
        mov rsi, [rbp - 16]
        mov rdx, [rbp - 24]
        mov rcx, [rbp - 32]
        call elf64_dump_strtab  ; size_t strtab_bytes = elf64_dump_strtab(...)
        cmp rax, 0              ; if (strtab_bytes == 0)
        je .ret_err
        mov rdx, [rbp - 24]
        sub rdx, rax
        mov [rbp - 24], rdx     ; n -= strtab_bytes
        mov rsi, [rbp - 16]
        mov rdx, [rbp - 40]
        add rsi, rax
        add rdx, rax
        mov [rbp - 16], rsi     ; buffer += strtab_bytes
        mov [rbp - 40], rdx     ; n_bytes += strtab_bytes

        mov rax, [rbp - 40]
        mov rsp, rbp
        pop rbp
        retn

.ret_err:
        xor eax, eax
        mov rsp, rbp
        pop rbp
        retn

;;; rdi: `struct AsmCtx *ctx`
;;; rsi: `void *buffer`
;;; rdx: `size_t n`
elf64_dump_header:
        xor eax, eax
        sub rdx, 0x40
        jl .ret_err
        add eax, 0x40

        ;; Store 4 byte magic number
        mov ecx, 0x464c457f
        mov [rsi], ecx

        ;; 64 bit
        mov cl, 2
        mov [rsi + 4], cl
        ;; Little endian
        mov cl, 1
        mov [rsi + 5], cl
        ;; ELF version 1
        mov cl, 1
        mov [rsi + 6], cl
        ;; Type relocatable file
        mov cx, 1
        mov [rsi + 0x10], cx
        ;; AMD x86-64 ISA
        mov cx, 0x3e
        mov [rsi + 0x12], cx
        ;; Version 1
        mov ecx, 1
        mov [rsi + 0x14], ecx
        ;; Section header table right after the ELF header
        mov rcx, 0x40
        mov [rsi + 0x28], rcx
        ;; Size of the ELF header
        mov rcx, 0x40
        mov [rsi + 0x34], rcx
        ;; Size of a section header table entry
        mov rcx, 0x40
        mov [rsi + 0x3a], rcx
        ;; Number of sections
        mov cx, 5
        mov [rsi + 0x3c], cx
        ;; Shstrtab index in section header table
        mov cx, 2
        mov [rsi + 0x3e], cx

        retn

.ret_err:
        xor eax, eax
        retn

;;; rdi: `struct AsmCtx *ctx`
;;; rsi: `void *buffer`
;;; rdx: `size_t n`
elf64_dump_shstrtab:
        push rbp
        mov rbp, rsp

        sub rdx, 0x20
        jl .ret_err

        ;; Write "\0.text\0.shstrtab\0.symtab\0.strtab\0" to buffer
        mov rax, 0x2e00747865742e00
        mov [rsi], rax
        mov rax, 0x6261747274736873
        mov [rsi + 8], rax
        mov rax, 0x6261746d79732e00
        mov [rsi + 16], rax
        mov rax, 0x6261747274732e00
        mov [rsi + 24], rax
        mov al, 0
        mov [rsi + 33], al

        mov rax, 0x30
        jmp .ret

.ret_err:
        xor eax, eax
.ret:
        mov rsp, rbp
        pop rbp
        retn

;;; rdi: `struct AsmCtx *ctx`
;;; rsi: `void *buffer`
;;; rdx: `size_t n`
;;; rcx: `char *filename`
elf64_dump_shtab:
        push rbp
        mov rbp, rsp
        sub rsp, 32
        mov [rbp - 8], rdi
        mov [rbp - 16], rsi
        mov [rbp - 24], rdx
        mov [rbp - 32], rcx

        sub rdx, 0x140          ; 5 section headers, 0x40 * 5
        jl .ret_err

        ;; Add 64 byte empty section header as first entry in the section header
        ;; table.
        add rsi, 0x40
        mov [rbp - 16], rsi

        ;;
        ;; Add section header for .text section
        ;;

        ;; Name offset in .shstrtab
        mov eax, 1              ; One for first named section
        mov [rsi], eax

        ;; Section type PROGBITS
        mov eax, 1
        mov [rsi + 0x4], eax

        ;; Flags SHF_ALLOC and SHF_EXECINSTR
        mov rax, 0x6
        mov [rsi + 0x8], rax

        ;; Offset of the section
        mov rax, 0x180          ; 0x40 bytes header + 5 * 0x40 bytes per section
                                ; header table entry
        mov [rsi + 0x18], rax

        ;; Size of the section in bytes
        mov rax, [rdi + 24]     ; rax = ctx->bintxt_size
        mov [rsi + 0x20], rax

        ;; 16 byte alignment
        mov rax, 16
        mov [rsi + 0x30], rax

        add rsi, 0x40
        mov [rbp - 16], rsi

        ;;
        ;; Add section header for .shstrtab section
        ;;

        ;; Name offset in .shstrtab
        mov eax, 7              ; len("\0.text\0")
        mov [rsi], eax

        ;; Section type STRTAB
        mov eax, 3
        mov [rsi + 0x4], eax

        ;; Offset of the section
        mov rsi, [rbp - 16]
        mov rdi, [rsi - 0x20]   ; Store size of previous section in rax
        call algn16             ; Align to 16
        mov rsi, [rbp - 16]
        mov rcx, [rsi - 0x28]   ; Store offset of previous section in rcx
        add rax, rcx
        mov [rsi + 0x18], rax

        ;; Size of the section in bytes
        call elf64_clcshstrtabsz
        mov rsi, [rbp - 16]
        mov [rsi + 0x20], rax

        ;; 1 byte alignment
        mov rax, 1
        mov [rsi + 0x30], rax

        add rsi, 0x40
        mov [rbp - 16], rsi

        ;;
        ;; Add section header for .symtab section
        ;;

        ;; Name offset in .shstrtab
        mov eax, 17             ; len("\0.text\0.shstrtab\0")
        mov [rsi], eax

        ;; Section type SYMTAB
        mov eax, 2
        mov [rsi + 0x4], eax

        ;; Offset of the section
        mov rsi, [rbp - 16]
        mov rdi, [rsi - 0x20]   ; Store size of previos section in rax
        call algn16             ; Align to 16
        mov rsi, [rbp - 16]
        mov rcx, [rsi - 0x28]   ; Store offset of previos section in rcx
        add rax, rcx
        mov [rsi + 0x18], rax

        ;; Size of the section in bytes
        mov rdi, [rbp - 8]
        call elf64_clcsymtabsz
        mov rsi, [rbp - 16]
        mov [rsi + 0x20], rax

        ;; Link index of strtab section header entry
        mov rax, 4
        mov [rsi + 0x28], rax

        ;; Number of non global symbol table entries
        mov rdi, [rbp - 8]
        mov rsi, [rdi + 40]     ; size_t n = ctx->max_symtab_entries
        mov rdi, [rdi + 32]     ; struct SymTabNtr *symtab = ctx->symtab
        call symtablen
        push rax

        mov rdi, [rbp - 8]
        call symtabnglbls
        pop rcx
        sub rcx, rax            ; rcx = symtablen(...) - symtabnglbls(...)
        add rcx, 3              ; Null entry, file entry, text section entry

        mov rsi, [rbp - 16]
        mov [rsi + 0x2c], rcx

        ;; 8 byte alignment
        mov rax, 8
        mov [rsi + 0x30], rax

        ;; Entrysize
        mov rax, 0x18           ; sizeof(Elf64_Sym)
        mov [rsi + 0x38], rax

        add rsi, 0x40
        mov [rbp - 16], rsi

        ;;
        ;; Add section header for .strtab section
        ;;

        ;; Name offset in .shstrtab
        mov eax, 25             ; len("\0.text\0.shstrtab\0.symtab\0")
        mov [rsi], eax

        ;; Section type STRTAB
        mov eax, 3
        mov [rsi + 0x4], eax

        ;; Offset of the section
        mov rsi, [rbp - 16]
        mov rdi, [rsi - 0x20]   ; Store size of previos section in rax
        call algn16             ; Align to 16
        mov rsi, [rbp - 16]
        mov rcx, [rsi - 0x28]   ; Store offset of previos section in rcx
        add rax, rcx
        mov [rsi + 0x18], rax

        ;; Size of the section in bytes
        mov rdi, [rbp - 8]
        mov rsi, [rbp - 32]
        call elf64_clcstrtabsz
        mov rsi, [rbp - 16]
        mov [rsi + 0x20], rax

        ;; 1 byte alignment
        mov rax, 1
        mov [rsi + 0x30], rax

        mov rax, 0x140
        jmp .ret

.ret_err:
        xor eax, eax
.ret:
        mov rsp, rbp
        pop rbp
        retn

;;; rdi: `struct AsmCtx *ctx`
;;; rsi: `void *buffer`
;;; rdx: `size_t n`
;;; rcx: `char *filename`
elf64_dump_strtab:
        push rbp
        mov rbp, rsp
        sub rsp, 64

        mov [rbp - 8], rdi
        mov [rbp - 16], rsi
        mov [rbp - 24], rdx
        mov [rbp - 32], rcx

        mov rsi, rcx
        call elf64_clcstrtabsz

        mov rdi, rax
        call algn16
        mov [rbp - 40], rax

        mov rdx, [rbp - 24]
        sub rdx, rax
        jl .ret_err

        ;; Copy the filename
        mov rdi, [rbp - 32]     ; void *str = filename
        call len
        mov rdi, [rbp - 32]     ; void *src = filename
        mov rsi, [rbp - 16]
        inc rsi                 ; void *dst = buffer + 1
        mov rdx, rax            ; size_t n = len(filename)

        mov r8, rsi
        add r8, rax
        mov [rbp - 16], r8      ; buffer = buffer + len(filename) + 1

        call cpy

        mov rdi, [rbp - 8]
        mov rsi, [rdi + 40]     ; rsi = ctx->max_symtab_entries
        mov rdi, [rdi + 32]
        mov [rbp - 48], rdi     ; struct SymTabNtr *symtab = ctx->symtab
        call symtablen
        mov [rbp - 56], rax     ; size_t symtab_len

.loop:
        mov rcx, [rbp - 56]
        cmp rcx, 0
        je .loop_end

        mov rdi, [rbp - 48]     ; void *str = symtab[0].label
        call len

        mov rdi, [rbp - 48]     ; void *src = symtab[0].label
        mov rsi, [rbp - 16]     ; void *dst = buffer

        mov r8, rsi
        add r8, rax
        mov [rbp - 16], r8      ; buffer = buffer + len(filename)

        call cpy

        mov rcx, [rbp - 56]
        dec rcx
        mov [rbp - 56], rcx     ; symtab_len--
        mov rdi, [rbp - 48]
        add rdi, 256
        mov [rbp - 48], rdi     ; symtab += sizeof(struct SymTabNtr)

        jmp .loop

.loop_end:
        mov rax, [rbp - 40]
        jmp .ret

.ret_err:
        xor eax, eax
.ret:
        mov rsp, rbp
        pop rbp
        retn

;;; rdi: `struct AsmCtx *ctx`
;;; rsi: `void *buffer`
;;; rdx: `size_t n`
;;; rcx: `char *filename`
elf64_dump_symtab:
        push rbp
        mov rbp, rsp
        sub rsp, 64

        mov [rbp - 8], rdi
        mov [rbp - 16], rsi
        mov [rbp - 24], rdx
        mov [rbp - 32], rcx

        call elf64_clcsymtabsz
        mov rdi, rax
        call algn16
        mov [rbp - 40], rax     ; size_t symtab_size = rax
        mov rdx, [rbp - 24]

        sub rdx, rax
        jl .ret_err

        mov rdi, [rbp - 32]
        call len
        inc rax
        mov [rbp - 48], rax     ; size_t strtab_offset = rax

        mov rsi, [rbp - 16]
        add rsi, 0x18           ; Create first, empty symtab entry

        ;;
        ;; File entry
        ;;

        ;; .strtab index of file name
        mov eax, 1
        mov [rsi], eax

        ;; Symbol type
        mov al, 4               ; STT_FILE
        mov [rsi + 4], al

        ;; Associated section index
        mov ax, 0xfff1          ; SHN_ABS
        mov [rsi + 6], ax

        add rsi, 0x18
        mov [rbp - 16], rsi

        ;;
        ;; .text section entry
        ;;

        ;; Symbol type
        mov al, 3               ; STT_SECTION
        mov [rsi + 4], al

        ;; Associated section index
        mov ax, 1               ; .text section
        mov [rsi + 6], ax

        add rsi, 0x18
        mov [rbp - 16], rsi

        mov rdi, [rbp - 8]
        mov rsi, [rdi + 40]     ; rsi = ctx->max_symtab_entries
        mov rdi, [rdi + 32]     ; rdi = ctx->symtab
        mov [rbp - 56], rdi     ; struct SymTabNtr *symtab = ctx->symtab
        call symtablen
        mov [rbp - 64], rax     ; size_t symtab_len

.loop_locals:
        mov rcx, [rbp - 64]
        cmp rcx, 0
        je .loop_locals_end
        ;; Is global?
        mov rdi, [rbp - 8]      ; struct AsmCtx *ctx = ctx
        mov rsi, [rbp - 56]     ; char *label = symtab[i].label
        call isglbl
        cmp rax, 1              ; if (isglbl(ctx, symtab[i].label) != 1)
        jne .loop_locals_store

        mov rdi, [rbp - 56]     ; rdi = symtab[i].label
        call len

        mov rcx, [rbp - 48]     ; rcx = strtab_offset
        add rcx, rax
        mov [rbp - 48], rcx     ; size_t strtab_offset += len(label)

        jmp .loop_locals_skip

.loop_locals_store:
        mov rdi, [rbp - 56]     ; rdi = symtab[i].label
        call len

        ;; .strtab index of symbol name
        mov rsi, [rbp - 16]     ; rsi = buffer
        mov rcx, [rbp - 48]     ; rcx = strtab_offset
        mov [rsi], ecx          ; ((Elf64_Sym)buffer).st_name = strtab_offset

        add rcx, rax
        mov [rbp - 48], rcx     ; size_t strtab_offset += len(label)

        ;; Associated section index
        mov ax, 1               ; .text section
        mov [rsi + 6], ax       ; ((Elf64_Sym)buffer).st_shndx = 9

        ;; Address
        mov rdi, [rbp - 56]
        mov eax, [rdi + 252]
        mov [rsi + 8], rax      ; ((Elf64_Sym)buffer).st_value = symtab[i].offset

        mov rsi, [rbp - 16]     ; rsi = buffer
        add rsi, 0x18
        mov [rbp - 16], rsi     ; buffer += sizeof(Elf64_Sym)
.loop_locals_skip:
        mov rdi, [rbp - 56]     ; rdi = symtab
        add rdi, 256
        mov [rbp - 56], rdi     ; symtab += sizeof(struct SymTabNtr)
        mov rcx, [rbp - 64]
        dec rcx
        mov [rbp - 64], rcx     ; symtab_len--

        jmp .loop_locals

.loop_locals_end:
        mov rdi, [rbp - 32]
        call len
        inc rax
        mov [rbp - 48], rax     ; size_t strtab_offset = rax

        mov rdi, [rbp - 8]
        mov rsi, [rdi + 40]     ; rsi = ctx->max_symtab_entries
        mov rdi, [rdi + 32]     ; rdi = ctx->symtab
        mov [rbp - 56], rdi     ; struct SymTabNtr *symtab = ctx->symtab
        call symtablen
        mov [rbp - 64], rax     ; size_t symtab_len

.loop_globals:
        mov rcx, [rbp - 64]
        cmp rcx, 0
        je .loop_globals_end
        ;; Is global?
        mov rdi, [rbp - 8]      ; struct AsmCtx *ctx = ctx
        mov rsi, [rbp - 56]     ; char *label = symtab[i].label
        call isglbl
        cmp rax, 1              ; if (isglbl(ctx, symtab[i].label) == 1)
        je .loop_globals_store

        mov rdi, [rbp - 56]     ; rdi = symtab[i].label
        call len

        mov rcx, [rbp - 48]     ; rcx = strtab_offset
        add rcx, rax
        mov [rbp - 48], rcx     ; size_t strtab_offset += len(label)

        jmp .loop_globals_skip

.loop_globals_store:
        mov rdi, [rbp - 56]     ; rdi = symtab[i].label
        call len

        ;; .strtab index of symbol name
        mov rsi, [rbp - 16]     ; rsi = buffer
        mov rcx, [rbp - 48]     ; rcx = strtab_offset
        mov [rsi], ecx          ; ((Elf64_Sym)buffer).st_name = strtab_offset

        add rcx, rax
        mov [rbp - 48], rcx     ; size_t strtab_offset += len(label)

        ;; Binding
        mov al, 0x10
        mov [rsi + 4], al       ; ((Elf64_Sym)buffer).st_info = STB_GLOBAL << 4

        ;; Associated section index
        mov ax, 1               ; .text section
        mov [rsi + 6], ax       ; ((Elf64_Sym)buffer).st_shndx = 9

        ;; Address
        mov rdi, [rbp - 56]
        mov eax, [rdi + 252]
        mov [rsi + 8], rax      ; ((Elf64_Sym)buffer).st_value = symtab[i].offset

        mov rsi, [rbp - 16]     ; rsi = buffer
        add rsi, 0x18
        mov [rbp - 16], rsi     ; buffer += sizeof(Elf64_Sym)
.loop_globals_skip:
        mov rdi, [rbp - 56]     ; rdi = symtab
        add rdi, 256
        mov [rbp - 56], rdi     ; symtab += sizeof(struct SymTabNtr)
        mov rcx, [rbp - 64]
        dec rcx
        mov [rbp - 64], rcx     ; symtab_len--

        jmp .loop_globals

.loop_globals_end:
        mov rax, [rbp - 40]
        jmp .ret

.ret_err:
        xor eax, eax
.ret:
        mov rsp, rbp
        pop rbp
        retn

;;; rdi: `struct AsmCtx *ctx`
;;; rsi: `void *buffer`
;;; rdx: `size_t n`
elf64_dump_text:
        push rbp
        mov rbp, rsp
        sub rsp, 32
        mov [rbp - 8], rdi
        mov [rbp - 16], rsi
        mov [rbp - 24], rdx

        mov rdi, [rdi + 24]     ; rdi = ctx->bintxt_size
        call algn16
        mov [rbp - 32], rax
        sub rdx, rax
        jl .ret_err

        mov rdi, [rbp - 8]
        mov rdx, [rdi + 24]
        mov rdi, [rdi + 8]      ; rdi = ctx->bintxt
        call cpy

        mov rax, [rbp - 32]
        jmp .ret

.ret_err:
        xor eax, eax
.ret:
        mov rsp, rbp
        pop rbp
        retn

;;; rdi: `struct AsmCtx *ctx`
;;; rsi: `struct AsmOp *op`
;;; rdx: `uint8_t op8`
genop2aximm32:
        push rbp
        mov rbp, rsp
        sub rsp, 32

        mov [rbp - 8], rdi
        mov [rbp - 16], rsi
        mov [rbp - 17], dl

        mov al, 2
        mov [rsi], al           ; op->encoding = ENCODING_I
        mov al, 1
        mov [rsi + 5], al       ; op->n_opcodes = 1

        mov rdi, [rbp - 8]
        mov rdi, [rdi]          ; char *assembly = ctx->assembly
        mov rsi, 2              ; uint8_t n_ops = 2
        call ckopsize
        mov rsi, [rbp - 16]
        mov [rsi + 1], al       ; op->op_size = al
        mov [rsi + 9], al       ; op->imm_size = al

        mov rdi, [rbp - 8]      ; char **assembly = &ctx->assembly
        call preg

        mov rdi, [rbp - 8]      ; char **assembly = &ctx->assembly
        call skp2nxtop

        mov rdi, [rbp - 8]      ; char **assembly = &ctx->assembly
        call pint

        mov rsi, [rbp - 16]
        mov dl, [rsi + 1]
        cmp dl, 8               ; if (op->op_size == 8)
        je .imm8
        cmp dl, 16              ; if (op->op_size == 16)
        je .imm16
        cmp dl, 32              ; if (op->op_size == 32)
        je .imm32
        jmp .imm64

.imm8:
        mov [rsi + 16], al      ; op->imm.imm8 = pint(...)
        mov al, [rbp - 17]
        mov [rsi + 6], al       ; op->opcodes[0] = op8
        jmp .end

.imm16:
        mov [rsi + 16], ax      ; op->imm.imm16 = pint(...)
        mov al, [rbp - 17]
        inc al
        mov [rsi + 6], al       ; op->opcodes[0] = op8 + 1
        mov al, 0x08
        mov [rsi + 11], al      ; op->prefix = PREFIX_OP_SIZE_OVERRIDE
        jmp .end

.imm32:
        mov [rsi + 16], eax     ; op->imm.imm32 = pint(...)
        mov al, [rbp - 17]
        inc al
        mov [rsi + 6], al       ; op->opcodes[0] = op8 + 1
        jmp .end

.imm64:
        mov [rsi + 16], eax      ; op->imm.imm32 = pint(...)
        mov al, [rbp - 17]
        inc al
        mov [rsi + 6], al       ; op->opcodes[0] = op8 + 1
        mov al, 32
        mov [rsi + 9], al       ; op->imm_size = 32

.end:
        mov rsp, rbp
        pop rbp
        retn

;;; rdi: `struct AsmCtx *ctx`
;;; rsi: `struct AsmOp *op`
;;; rdx: `uint8_t op8`
;;; rcx: `uint8_t op_simm8`
;;; r8: `uint8_t modrm_reg`
genop2rimm32:
        push rbp
        mov rbp, rsp
        sub rsp, 32

        mov [rbp - 8], rdi
        mov [rbp - 16], rsi
        mov [rbp - 17], dl
        mov [rbp - 18], cl

        mov al, 4
        mov [rsi], al           ; op->encoding = ENCODING_MI
        mov al, 1
        mov [rsi + 5], al       ; op->n_opcodes = 1
        mov [rsi + 2], r8b      ; op->src_reg = modrm_reg

        mov rdi, [rbp - 8]
        mov rdi, [rdi]          ; char *assembly = ctx->assembly
        mov rsi, 2              ; uint8_t n_ops = 2
        call ckopsize
        mov rsi, [rbp - 16]
        mov [rsi + 1], al       ; op->op_size = al
        mov [rsi + 9], al       ; op->imm_size = al

        mov rdi, [rbp - 8]      ; char **assembly = &ctx->assembly
        call preg

        mov rsi, [rbp - 16]
        mov [rsi + 3], al       ; op->dst_reg = preg(&ctx->assembly)
        mov al, 0b11
        mov [rsi + 4], al       ; op->modrm_mod = MOD_DIRECT

        mov rdi, [rbp - 8]      ; char **assembly = &ctx->assembly
        call skp2nxtop

        mov rdi, [rbp - 8]      ; char **assembly = &ctx->assembly
        call pint

        mov rsi, [rbp - 16]
        mov dl, [rsi + 1]
        cmp dl, 8               ; if (op->op_size == 8)
        je .op8
        cmp dl, 16              ; if (op->op_size == 16)
        je .op16
        cmp dl, 32              ; if (op->op_size == 32)
        je .op32
        jmp .op64

.op8:
        mov [rsi + 16], al      ; op->imm.imm8 = pint(...)
        mov al, [rbp - 17]
        mov [rsi + 6], al       ; op->opcodes[0] = op8
        jmp .end

.op16:
        mov cl, 0x08
        mov [rsi + 11], cl      ; op->prefix = PREFIX_OP_SIZE_OVERRIDE
        cmp ax, 128             ; if (pint(...) < 128)
        jb .op_sign_extended
        mov [rsi + 16], ax      ; op->imm.imm16 = pint(...)
        mov al, [rbp - 17]
        inc al
        mov [rsi + 6], al       ; op->opcodes[0] = op8 + 1
        jmp .end

.op32:
        cmp eax, 128            ; if (pint(...) < 128)
        jb .op_sign_extended
        mov [rsi + 16], eax     ; op->imm.imm32 = pint(...)
        mov al, [rbp - 17]
        inc al
        mov [rsi + 6], al       ; op->opcodes[0] = op8 + 1
        jmp .end

.op64:
        cmp eax, 128            ; if (pint(...) < 128)
        jb .op_sign_extended
        mov [rsi + 16], eax     ; op->imm.imm32 = pint(...)
        mov al, [rbp - 17]
        inc al
        mov [rsi + 6], al       ; op->opcodes[0] = op8 + 1
        mov al, 32
        mov [rsi + 9], al       ; op->imm_size = 32
        jmp .end

.op_sign_extended:
        mov [rsi + 16], al      ; op->imm.imm8 = pint(...)
        mov al, 8
        mov [rsi + 9], al       ; op->imm_size = 8
        mov al, [rbp - 18]
        mov [rsi + 6], al       ; op->opcodes[0] = op_rsimm8

.end:
        mov rsp, rbp
        pop rbp
        retn

;;; rdi: `struct AsmCtx *ctx`
;;; rsi: `struct AsmOp *op`
;;; rdx: `uint8_t op8`
genop2rmr:
        push rbp
        mov rbp, rsp
        sub rsp, 32

        mov [rbp - 8], rdi
        mov [rbp - 16], rsi
        mov [rbp - 17], dl

        mov al, 5
        mov [rsi], al           ; op->encoding = ENCODING_MR
        mov al, 1
        mov [rsi + 5], al       ; op->n_opcodes = 1

        mov rdi, [rbp - 8]
        mov rdi, [rdi]          ; char *assembly = ctx->assembly
        mov rsi, 2              ; uint8_t n_ops = 2
        call ckopsize
        mov rsi, [rbp - 16]
        mov [rsi + 1], al       ; op->op_size = al

        mov rdi, [rbp - 8]
        mov rdi, [rdi]          ; char *assembly = ctx->assembly
        call isreg
        cmp rax, 1              ; if (isreg(ctx->assembly)
        je .first_op_reg

.first_op_drct:
        mov rdi, [rbp - 8]      ; struct AsmCtx *ctx = ctx
        mov rsi, [rbp - 16]     ; struct AsmOp *op = op
        lea rdx, [rsi + 12]     ; uint32_t *disp = &(op->disp.disp32)
        lea rsi, [rsi + 3]      ; uint8_t *reg = &(op->dst_reg)
        call prgndrct

        mov rdi, [rbp - 16]     ; struct AsmOp *op = op
        call strdspmodrmmod
        jmp .second_op

.first_op_reg:
        mov rdi, [rbp - 8]      ; char **assembly = &ctx->assembly
        call preg

        mov rsi, [rbp - 16]
        mov [rsi + 3], al       ; op->dst_reg = preg(&ctx->assembly)
        mov al, 0b11
        mov [rsi + 4], al       ; op->modrm_mod = MOD_DIRECT

.second_op:
        mov rdi, [rbp - 8]      ; char **assembly = &ctx->assembly
        call skp2nxtop

        mov rdi, [rbp - 8]      ; char **assembly = &ctx->assembly
        call preg

        mov rsi, [rbp - 16]
        mov [rsi + 2], al       ; op->src_reg = preg(&ctx->assembly)

        mov al, [rsi + 1]
        cmp al, 8               ; if (op->op_size == 8)
        je .op8
        cmp al, 16              ; if (op->op_size == 16)
        je .op16
        cmp al, 32              ; if (op->op_size == 32)
        je .op32
        jmp .op64

.op8:
        mov al, [rbp - 17]
        mov [rsi + 6], al        ; op->opcodes[0] = op8
        jmp .end

.op16:
        mov al, [rbp - 17]
        inc al
        mov [rsi + 6], al        ; op->opcodes[0] = op8 + 1
        mov al, 0x08
        mov [rsi + 11], al      ; op->prefix = PREFIX_OP_SIZE_OVERRIDE
        jmp .end

.op32:
.op64:
        mov al, [rbp - 17]
        inc al
        mov [rsi + 6], al        ; op->opcodes[0] = op8 + 1

.end:
        mov rsp, rbp
        pop rbp
        retn

;;; rdi: `struct AsmCtx *ctx`
;;; rsi: `struct AsmOp *op`
;;; rdx: `uint8_t op8`
genop2rrm:
        push rbp
        mov rbp, rsp
        sub rsp, 32

        mov [rbp - 8], rdi
        mov [rbp - 16], rsi
        mov [rbp - 17], dl

        mov al, 8
        mov [rsi], al           ; op->encoding = ENCODING_RM
        mov al, 1
        mov [rsi + 5], al       ; op->n_opcodes = 1

        mov rdi, [rbp - 8]
        mov rdi, [rdi]          ; char *assembly = ctx->assembly
        mov rsi, 2              ; uint8_t n_ops = 2
        call ckopsize
        mov rsi, [rbp - 16]
        mov [rsi + 1], al       ; op->op_size = al

        mov rdi, [rbp - 8]      ; char **assembly = &ctx->assembly
        call preg

        mov rsi, [rbp - 16]
        mov [rsi + 3], al       ; op->dst_reg = preg(&ctx->assembly)

        mov rdi, [rbp - 8]      ; char **assembly = &ctx->assembly
        call skp2nxtop

        mov rdi, [rbp - 8]
        mov rdi, [rdi]          ; char *assembly = ctx->assembly
        call isreg
        cmp rax, 1              ; if (isreg(ctx->assembly)
        je .second_op_reg

.second_op_drct:
        mov rdi, [rbp - 8]      ; struct AsmCtx *ctx = ctx
        mov rsi, [rbp - 16]     ; struct AsmOp *op = op
        lea rdx, [rsi + 12]     ; uint32_t *disp = &(op->disp.disp32)
        lea rsi, [rsi + 2]      ; uint8_t *reg = &(op->src_reg)
        call prgndrct

        mov rdi, [rbp - 16]     ; struct AsmOp *op = op
        call strdspmodrmmod

        mov rsi, [rbp - 16]
        jmp .encode

.second_op_reg:
        mov rdi, [rbp - 8]      ; char **assembly = &ctx->assembly
        call preg

        mov rsi, [rbp - 16]
        mov [rsi + 2], al       ; op->src_reg = preg(&ctx->assembly)
        mov al, 0b11
        mov [rsi + 4], al       ; op->modrm_mod = MOD_DIRECT

.encode:
        mov al, [rsi + 1]
        cmp al, 8               ; if (op->op_size == 8)
        je .op8
        cmp al, 16              ; if (op->op_size == 16)
        je .op16
        cmp al, 32              ; if (op->op_size == 32)
        je .op32
        jmp .op64

.op8:
        mov al, [rbp - 17]
        mov [rsi + 6], al        ; op->opcodes[0] = op8
        jmp .end

.op16:
        mov al, [rbp - 17]
        inc al
        mov [rsi + 6], al        ; op->opcodes[0] = op8 + 1
        mov al, 0x08
        mov [rsi + 11], al      ; op->prefix = PREFIX_OP_SIZE_OVERRIDE
        jmp .end

.op32:
.op64:
        mov al, [rbp - 17]
        inc al
        mov [rsi + 6], al        ; op->opcodes[0] = op8 + 1

.end:
        mov rsp, rbp
        pop rbp
        retn

;;; rdi: `struct AsmCtx *ctx`
;;; rsi: `char *label`
isglbl:
        mov rcx, [rdi + 72]     ; rcx = ctx->max_globals
        mov rdi, [rdi + 64]     ; rdi = ctx->globals
        mov r8, rdi
        mov r9, rsi

.loop:
        cmp rcx, 0
        je .ret_false
        mov al, 0
        cmp [rsi], al
        je .ret_true

        mov al, [rsi]
        cmp [rdi], al
        jne .next_entry
        inc rdi
        inc rsi
        jmp .loop

.next_entry:
        add r8, 64
        mov rdi, r8
        mov rsi, r9
        dec rcx
        jmp .loop

.ret_false:
        xor eax, eax
        retn

.ret_true:
        mov rax, 1
        retn

;;; rdi: `char *assembly`
isint:
        push rdi
        call isopdlm
        pop rdi
        cmp rax, 1
        je .ret_false

        mov al, 0x30            ; Ascii zero ('0')
        cmp [rdi], al
        jne .check_decimal
        mov al, 0x62            ; Ascii lowercase letter b
        cmp [rdi + 1], al
        je .check_binary
        mov al, 0x78            ; Ascii lowercase letter x
        cmp [rdi + 1], al
        je .check_hexadecimal
        jmp .check_decimal

.check_binary:
        add rdi, 2
        push rdi
        call isopdlm
        pop rdi
        cmp rax, 1
        je .ret_false

.check_binary_loop:
        mov al, 0x30            ; Ascii zero ('0')
        mov ah, 0x31            ; Ascii one ('1')
        cmp [rdi], al
        jb .check_token_delim
        cmp [rdi], ah
        ja .check_token_delim
        inc rdi
        jmp .check_binary_loop

.check_hexadecimal:
        add rdi, 2
        push rdi
        call isopdlm
        pop rdi
        cmp rax, 1
        je .ret_false

        mov al, 0x30            ; Ascii zero ('0')
        mov ah, 0x39            ; Ascii nine ('9')
        mov cl, 0x61            ; Ascii lowercase letter a
        mov ch, 0x66            ; Ascii lowercase letter f
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
        mov al, 0x30            ; Ascii zero ('0')
        mov ah, 0x39            ; Ascii nine ('9')
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
        mov al, 0x9             ; Ascii tabulator ('\t')
        cmp [rdi], al
        je .ret_true
        mov al, 0xa             ; Ascii newline ('\n')
        cmp [rdi], al
        je .ret_true
        mov al, 0x20            ; Ascii space
        cmp [rdi], al
        je .ret_true
        mov al, 0x2c            ; Ascii comma
        cmp [rdi], al
        je .ret_true
        mov al, 0x3b            ; Ascii semicolon
        cmp [rdi], al
        je .ret_true
        mov al, 0x5d            ; Ascii closing square bracket
        cmp [rdi], al
        je .ret_true
        mov al, 0               ; Null terminator
        cmp [rdi], al
        je .ret_true

.ret_false:
        mov rax, 0
        jmp .end
.ret_true:
        mov rax, 1

.end:
        retn

;;; rdi: `char *assembly`
isrgndrct:
        mov al, 0x5b
        cmp [rdi], al           ; if (assembly[0] != '[')
        jne .ret_false
        mov eax, 1
        retn

.ret_false:
        mov eax, 0
        retn

;;; rdi: `char *assembly`
isreg:
        push rbp
        mov rbp, rsp
        sub rsp, 16
        mov [rbp - 8], rdi
        mov [rbp - 16], rdi

        lea rdi, [rbp - 8]
        call pr8
        cmp al, 0xff
        jne .ret_true

        mov rdi, [rbp - 16]
        mov [rbp - 8], rdi
        lea rdi, [rbp - 8]
        call pr16
        cmp al, 0xff
        jne .ret_true

        mov rdi, [rbp - 16]
        mov [rbp - 8], rdi
        lea rdi, [rbp - 8]
        call pr32
        cmp al, 0xff
        jne .ret_true

        mov rdi, [rbp - 16]
        mov [rbp - 8], rdi
        lea rdi, [rbp - 8]
        call pr64
        cmp al, 0xff
        jne .ret_true

        mov eax, 0
        mov rsp, rbp
        pop rbp
        retn

.ret_true:
        mov eax, 1
        mov rsp, rbp
        pop rbp
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

;;; rdi: `char **assembly`
pint:
        push rbp
        mov rbp, rsp
        sub rsp, 8
        mov [rbp - 8], rdi
        xor eax, eax
        xor r8d, r8d

        mov rsi, [rdi]
        mov cl, 0x30            ; Ascii zero ('0')
        cmp [rsi], cl
        jne .parse_decimal
        mov cl, 0x62            ; Ascii lowercase letter b
        cmp [rsi + 1], cl
        je .parse_binary
        mov cl, 0x78            ; Ascii lowercase letter x
        cmp [rsi + 1], cl
        je .parse_hexadecimal
        jmp .parse_decimal

.parse_binary:
        add rsi, 2
        mov cl, 0x30            ; Ascii zero ('0')
        mov ch, 0x31            ; Ascii one ('1')
.parse_binary_loop:
        cmp [rsi], cl
        jb .ret
        cmp [rsi], ch
        ja .ret

        shl rax, 1
        mov r8b, [rsi]
        sub r8b, 0x30
        or al, r8b

        inc rsi
        jmp .parse_binary_loop

.parse_hexadecimal:
        add rsi, 2
        mov cl, 0x30            ; Ascii zero ('0')
        mov ch, 0x39            ; Ascii nine ('9')
        mov dl, 0x61            ; Ascii lowercase letter a
        mov dh, 0x66            ; Ascii lowercase letter f
.parse_hex_loop:
        cmp [rsi], cl
        jb .ret
        cmp [rsi], dh
        ja .ret
        cmp [rsi], ch
        jbe .parse_hex_digit
        cmp [rsi], dl
        jae .parse_hex_letter
        jmp .ret

.parse_hex_digit:
        shl rax, 4
        mov r8b, [rsi]
        sub r8b, 0x30
        or al, r8b
        jmp .parse_hex_next

.parse_hex_letter:
        shl rax, 4
        mov r8b, [rsi]
        sub r8b, 0x57           ; 0x61 - 10 or ten below ord('a')
        or al, r8b
        jmp .parse_hex_next

.parse_hex_next:
        inc rsi
        jmp .parse_hex_loop

.parse_decimal:
        mov cl, 0x30            ; Ascii zero ('0')
        mov ch, 0x39            ; Ascii nine ('9')
        mov r8d, 10
        xor r9d, r9d
.parse_decimal_loop:
        cmp [rsi], cl
        jb .ret
        cmp [rsi], ch
        ja .ret
        mul r8                  ; rax = 10 * eax
        mov r9b, [rsi]
        sub r9b, 0x30
        add rax, r9
        inc rsi
        jmp .parse_decimal_loop

.ret:
        mov [rdi], rsi
        mov rsp, rbp
        pop rbp
        retn

;;; rdi: `char **assembly`
pr8:
        push rbp
        mov rbp, rsp
        sub rsp, 24

        xor ecx, ecx
        mov [rbp - 8], rdi
        mov [rbp - 16], rcx
        mov rdi, [rdi]

        mov al, 0x61            ; Ascii lowercase letter a
        cmp [rdi], al
        mov cl, 0b0000
        je .a_b_c_d
        mov al, 0x62            ; Ascii lowercase letter b
        mov cl, 0b0011
        cmp [rdi], al
        je .a_b_c_d
        mov al, 0x63            ; Ascii lowercase letter c
        mov cl, 0b0001
        cmp [rdi], al
        je .a_b_c_d
        mov al, 0x64            ; Ascii lowercase letter d
        mov cl, 0b0010
        cmp [rdi], al
        je .a_b_c_d
        mov al, 0x72            ; Ascii lowercase letter r
        cmp [rdi], al
        je .r
        jmp .ret_false

.a_b_c_d:
        mov al, 0x6c            ; Ascii lowercase letter l
        inc rdi
        cmp [rdi], al
        je .check_token_delim
        mov al, 0x68            ; Ascii lowercase letter h
        or cl, 0b0100
        cmp [rdi], al
        je .check_token_delim
        jmp .ret_false

.r:
        inc rdi
        mov al, 0x38            ; Ascii number 8
        mov cl, 0b1000
        cmp [rdi], al
        je .r8_r9_r1x
        mov al, 0x39            ; Ascii number 9
        mov cl, 0b1001
        cmp [rdi], al
        je .r8_r9_r1x
        mov al, 0x31            ; Ascii number 1
        cmp [rdi], al
        je .r1
        jmp .ret_false

.r1:
        inc rdi
        mov al, 0x30            ; Ascii number 0
        cmp [rdi], al
        mov cl, 0b1010
        je .r8_r9_r1x
        mov al, 0x31            ; Ascii number 1
        mov cl, 0b1011
        cmp [rdi], al
        je .r8_r9_r1x
        mov al, 0x32            ; Ascii number 2
        mov cl, 0b1100
        cmp [rdi], al
        je .r8_r9_r1x
        mov al, 0x33            ; Ascii number 3
        mov cl, 0b1101
        cmp [rdi], al
        je .r8_r9_r1x
        mov al, 0x34            ; Ascii number 4
        mov cl, 0b1110
        cmp [rdi], al
        je .r8_r9_r1x
        mov al, 0x35            ; Ascii number 5
        mov cl, 0b1111
        cmp [rdi], al
        je .r8_r9_r1x
        jmp .ret_false

.r8_r9_r1x:
        inc rdi
        mov al, 0x62            ; Ascii lowercase letter b
        cmp [rdi], al
        je .check_token_delim
        jmp .ret_false

.check_token_delim:
        inc rdi
        mov [rbp - 16], rcx
        mov [rbp - 24], rdi
        call isopdlm
        cmp rax, 1
        jne .ret_false

        mov rdi, [rbp - 8]
        mov rsi, [rbp - 24]
        mov [rdi], rsi
        mov rax, [rbp - 16]
        mov rsp, rbp
        pop rbp
        retn

.ret_false:
        xor rax, rax
        mov al, 0xff
        mov rsp, rbp
        pop rbp
        retn

;;; rdi: `char **assembly`
pr16:
        push rbp
        mov rbp, rsp
        sub rsp, 24

        xor ecx, ecx
        mov [rbp - 8], rdi
        mov [rbp - 16], rcx
        mov rdi, [rdi]

        mov al, 0x61            ; Ascii lowercase letter a
        cmp [rdi], al
        je .a
        mov al, 0x62            ; Ascii lowercase letter b
        cmp [rdi], al
        je .b
        mov al, 0x63            ; Ascii lowercase letter c
        cmp [rdi], al
        je .c
        mov al, 0x64            ; Ascii lowercase letter d
        cmp [rdi], al
        je .d
        jmp .ret_false

.a:
        mov al, 0x78            ; Ascii lowercase letter x
        mov cl, 0b0000
        inc rdi
        cmp [rdi], al
        je .check_token_delim
        jmp .ret_false

.b:
        mov al, 0x78            ; Ascii lowercase letter x
        mov cl, 0b0011
        inc rdi
        cmp [rdi], al
        je .check_token_delim
        jmp .ret_false

.c:
        mov al, 0x78            ; Ascii lowercase letter x
        mov cl, 0b0001
        inc rdi
        cmp [rdi], al
        je .check_token_delim
        jmp .ret_false

.d:
        mov al, 0x78            ; Ascii lowercase letter x
        mov cl, 0b0010
        inc rdi
        cmp [rdi], al
        je .check_token_delim
        jmp .ret_false

.check_token_delim:
        inc rdi
        mov [rbp - 16], rcx
        mov [rbp - 24], rdi
        call isopdlm
        cmp rax, 1
        jne .ret_false

        mov rdi, [rbp - 8]
        mov rsi, [rbp - 24]
        mov [rdi], rsi
        mov rax, [rbp - 16]
        mov rsp, rbp
        pop rbp
        retn

.ret_false:
        xor rax, rax
        mov al, 0xff
        mov rsp, rbp
        pop rbp
        retn

;;; rdi: `char **assembly`
pr32:
        push rbp
        mov rbp, rsp
        sub rsp, 24

        xor ecx, ecx
        mov [rbp - 8], rdi
        mov [rbp - 16], rcx
        mov rdi, [rdi]

        mov al, 0x65            ; Ascii lowercase letter e
        cmp [rdi], al
        je .e
        mov al, 0x72            ; Ascii lowercase letter r
        cmp [rdi], al
        je .r
        jmp .ret_false

.e:
        inc rdi
        mov al, 0x61            ; Ascii lowercase letter a
        cmp [rdi], al
        je .ea
        mov al, 0x62            ; Ascii lowercase letter b
        cmp [rdi], al
        je .eb
        mov al, 0x63            ; Ascii lowercase letter c
        cmp [rdi], al
        je .ec
        mov al, 0x64            ; Ascii lowercase letter d
        cmp [rdi], al
        je .ed
        mov al, 0x73            ; Ascii lowercase letter s
        cmp [rdi], al
        je .es
        jmp .ret_false

.ea:
        inc rdi
        mov al, 0x78            ; Ascii lowercase letter x
        mov cl, 0b0000
        cmp [rdi], al
        je .check_token_delim
        jmp .ret_false

.eb:
        inc rdi
        mov al, 0x78            ; Ascii lowercase letter x
        mov cl, 0b0011
        cmp [rdi], al
        je .check_token_delim
        jmp .ret_false

.ec:
        inc rdi
        mov al, 0x78            ; Ascii lowercase letter x
        mov cl, 0b0001
        cmp [rdi], al
        je .check_token_delim
        jmp .ret_false

.ed:
        inc rdi
        mov al, 0x69            ; Ascii lowercase letter i
        mov cl, 0b0111
        cmp [rdi], al
        je .check_token_delim
        mov al, 0x78            ; Ascii lowercase letter x
        mov cl, 0b0010
        cmp [rdi], al
        je .check_token_delim
        jmp .ret_false

.es:
        inc rdi
        mov al, 0x69            ; Ascii lowercase letter i
        mov cl, 0b0110
        cmp [rdi], al
        je .check_token_delim
        jmp .ret_false

.r:
        inc rdi
        mov al, 0x38            ; Ascii number 8
        mov cl, 0b1000
        cmp [rdi], al
        je .r8_r9_r1x
        mov al, 0x39            ; Ascii number 9
        mov cl, 0b1001
        cmp [rdi], al
        je .r8_r9_r1x
        mov al, 0x31            ; Ascii number 1
        cmp [rdi], al
        je .r1
        jmp .ret_false

.r1:
        inc rdi
        mov al, 0x30            ; Ascii number 0
        cmp [rdi], al
        mov cl, 0b1010
        je .r8_r9_r1x
        mov al, 0x31            ; Ascii number 1
        mov cl, 0b1011
        cmp [rdi], al
        je .r8_r9_r1x
        mov al, 0x32            ; Ascii number 2
        mov cl, 0b1100
        cmp [rdi], al
        je .r8_r9_r1x
        mov al, 0x33            ; Ascii number 3
        mov cl, 0b1101
        cmp [rdi], al
        je .r8_r9_r1x
        mov al, 0x34            ; Ascii number 4
        mov cl, 0b1110
        cmp [rdi], al
        je .r8_r9_r1x
        mov al, 0x35            ; Ascii number 5
        mov cl, 0b1111
        cmp [rdi], al
        je .r8_r9_r1x
        jmp .ret_false

.r8_r9_r1x:
        inc rdi
        mov al, 0x64            ; Ascii lowercase letter d
        cmp [rdi], al
        je .check_token_delim
        jmp .ret_false

.check_token_delim:
        inc rdi
        mov [rbp - 16], rcx
        mov [rbp - 24], rdi
        call isopdlm
        cmp rax, 1
        jne .ret_false

        mov rdi, [rbp - 8]
        mov rsi, [rbp - 24]
        mov [rdi], rsi
        mov rax, [rbp - 16]
        mov rsp, rbp
        pop rbp
        retn

.ret_false:
        xor rax, rax
        mov al, 0xff
        mov rsp, rbp
        pop rbp
        retn

;;; rdi: `char **assembly`
pr64:
        push rbp
        mov rbp, rsp
        sub rsp, 24

        xor ecx, ecx
        mov [rbp - 8], rdi
        mov [rbp - 16], rcx
        mov rdi, [rdi]
        mov al, 0x72            ; Ascii lowercase letter r
        cmp [rdi], al
        je .r
        jmp .ret_false

.r:
        inc rdi
        mov al, 0x61            ; Ascii lowercase letter a
        cmp [rdi], al
        je .ra
        mov al, 0x62            ; Ascii lowercase letter b
        cmp [rdi], al
        je .rb
        mov al, 0x63            ; Ascii lowercase letter c
        cmp [rdi], al
        je .rc
        mov al, 0x64            ; Ascii lowercase letter d
        cmp [rdi], al
        je .rd
        mov al, 0x73            ; Ascii lowercase letter s
        cmp [rdi], al
        je .rs
        mov al, 0x38            ; Ascii number 8
        mov cl, 0b1000
        cmp [rdi], al
        je .check_token_delim
        mov al, 0x39            ; Ascii number 9
        mov cl, 0b1001
        cmp [rdi], al
        je .check_token_delim
        mov al, 0x31            ; Ascii number 1
        cmp [rdi], al
        je .r1
        jmp .ret_false

.r1:
        inc rdi
        mov al, 0x30            ; Ascii number 0
        mov cl, 0b1010
        cmp [rdi], al
        je .check_token_delim
        mov al, 0x31            ; Ascii number 1
        mov cl, 0b1011
        cmp [rdi], al
        je .check_token_delim
        mov al, 0x32            ; Ascii number 2
        mov cl, 0b1100
        cmp [rdi], al
        je .check_token_delim
        mov al, 0x33            ; Ascii number 3
        mov cl, 0b1101
        cmp [rdi], al
        je .check_token_delim
        mov al, 0x34            ; Ascii number 4
        mov cl, 0b1110
        cmp [rdi], al
        je .check_token_delim
        mov al, 0x35            ; Ascii number 5
        mov cl, 0b1111
        cmp [rdi], al
        je .check_token_delim
        jmp .ret_false

.ra:
        inc rdi
        mov al, 0x78            ; Ascii lowercase letter x
        mov cl, 0
        cmp [rdi], al
        je .check_token_delim
        jmp .ret_false

.rb:
        inc rdi
        mov al, 0x70            ; Ascii lowercase letter p
        cmp [rdi], al
        mov cl, 0b0101
        je .check_token_delim
        mov al, 0x78            ; Ascii lowercase letter x
        mov cl, 0b0011
        cmp [rdi], al
        je .check_token_delim
        jmp .ret_false

.rc:
        inc rdi
        mov al, 0x78            ; Ascii lowercase letter x
        mov cl, 0b0001
        cmp [rdi], al
        je .check_token_delim
        jmp .ret_false

.rd:
        inc rdi
        mov al, 0x69            ; Ascii lowercase letter i
        mov cl, 0b0111
        cmp [rdi], al
        je .check_token_delim
        mov al, 0x78            ; Ascii lowercase letter x
        mov cl, 0b0010
        cmp [rdi], al
        je .check_token_delim
        jmp .ret_false

.rs:
        inc rdi
        mov al, 0x69            ; Ascii lowercase letter i
        mov cl, 0b0110
        cmp [rdi], al
        je .check_token_delim
        mov al, 0x70            ; Ascii lowercase letter p
        mov cl, 0b0100
        cmp [rdi], al
        je .check_token_delim
        jmp .ret_false

.check_token_delim:
        inc rdi
        mov [rbp - 16], rcx
        mov [rbp - 24], rdi
        call isopdlm
        cmp rax, 1
        jne .ret_false

        mov rdi, [rbp - 8]
        mov rsi, [rbp - 24]
        mov [rdi], rsi
        mov rax, [rbp - 16]
        mov rsp, rbp
        pop rbp
        retn

.ret_false:
        xor rax, rax
        mov al, 0xff
        mov rsp, rbp
        pop rbp
        retn

;;; rdi: `char **assembly`
preg:
        push rbp
        mov rbp, rsp
        sub rsp, 8
        mov [rbp - 8], rdi

        call pr8
        cmp al, 0xff
        jne .ret

        mov rdi, [rbp - 8]
        call pr16
        cmp al, 0xff
        jne .ret

        mov rdi, [rbp - 8]
        call pr32
        cmp al, 0xff
        jne .ret

        mov rdi, [rbp - 8]
        call pr64

.ret:
        mov rsp, rbp
        pop rbp
        retn

;;; rdi: `char const **assembly`
;;; rsi: `uint8_t *reg`
;;; rdx: `uint32_t *disp`
prgndrct:
        push rbp
        mov rbp, rsp
        sub rsp, 24
        mov [rbp - 8], rdi
        mov [rbp - 16], rsi
        mov [rbp - 24], rdx
        mov rsi, [rdi]
        xor eax, eax
        mov [rdx], eax          ; *disp = 0

        inc rsi                 ; Skip initial '[' character
        mov [rdi], rsi
        call skp2lbinst

        mov rdi, [rbp - 8]
        call preg               ; Fetch register
        mov rsi, [rbp - 16]
        mov [rsi], al           ; *reg = al

        mov rdi, [rbp - 8]
        call skp2lbinst         ; Skip to next token

        mov rdi, [rbp - 8]
        mov rsi, [rdi]
        mov al, 0x5d            ; Ascii closing square bracket (']')
        cmp [rsi], al
        je .end

        mov al, 0x2b            ; Ascii plus ('+')
        cmp [rsi], al
        je .store_plus

.store_minus:
        inc rsi
        mov [rdi], rsi
        call skp2lbinst         ; Skip to next token

        mov rdi, [rbp - 8]
        call pint

        xor ecx, ecx
        sub ecx, eax            ; ecx = -eax
        mov rdx, [rbp - 24]
        mov [rdx], ecx          ; *disp = ecx
        jmp .end

.store_plus:
        inc rsi
        mov [rdi], rsi
        call skp2lbinst         ; Skip to next token

        mov rdi, [rbp - 8]
        call pint

        mov rdx, [rbp - 24]
        mov [rdx], eax          ; *disp = acx
        jmp .end

.end:
        mov rdi, [rbp - 8]
        call skp2lbinst         ; Skip to next token
        mov rdi, [rbp - 8]
        mov rsi, [rdi]
        inc rsi
        mov [rdi], rsi          ; (*assembly)++ Skip the closing square bracket
        mov rsp, rbp
        pop rbp
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
        mov [rbp - 32], rax     ; size_t bytes_written = 0
        mov [rbp - 40], rax     ; uint64_t overflow = 0

.loop:
        mov rdi, [rbp - 8]
        mov rdi, [rdi]
        call isopdlm
        cmp rax, 0x1            ; if (isopdlm(*assembly))
        je .end

        mov rdi, [rbp - 8]
        mov rdi, [rdi]
        mov cl, 0x3a            ; Ascii colon
        cmp [rdi], cl           ; if ((*assembly)[0] == ':')
        je .end

        mov rax, [rbp - 32]
        inc rax
        mov [rbp - 32], rax     ; bytes_written++
        mov rdx, [rbp - 24]
        cmp rax, rdx            ; if (bytes_written > n)
        ja .no_copy

        mov cl, [rdi]
        mov rsi, [rbp - 16]
        mov [rsi], cl           ; label[0] = (*assembly)[0]
.loop_end:
        inc rdi                 ; (*assembly)++
        inc rsi                 ; label++
        mov r9, [rbp - 8]
        mov [r9], rdi
        mov [rbp - 16], rsi
        jmp .loop
.no_copy:
        mov r9, 1
        mov [rbp - 40], r9      ; overflow = 1
        jmp .loop_end

.end:
        mov rsi, [rbp - 8]
        mov rdi, [rsi]
        inc rdi
        mov [rsi], rdi          ; (*assembly)++

        mov r9, [rbp - 40]
        cmp r9, 1               ; if (overflow == 1)
        je .no_terminator
        mov rax, [rbp - 32]
        inc rax                 ; bytes_written++
        mov rdx, [rbp - 24]
        cmp rax, rdx            ; if (bytes_written > n)
        ja .no_terminator

        mov rsi, [rbp - 16]
        inc rsi                 ; label++
        mov cl, 0
        mov [rsi], cl           ; label[0] = '\0'
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
        mov [rbp - 12], eax     ; uint32_t flags
        mov [rbp - 16], eax     ; uint32_t rel_target
        mov [rbp - 20], eax     ; uint32_t offset
        mov [rbp - 24], eax     ; uint32_t target_offset
        mov rsi, [rdi + 48]
        mov [rbp - 32], rsi     ; struct SymTabNtr *reftab
        mov rsi, [rdi + 56]
        mov [rbp - 40], rsi     ; struct size_t max_reftab_entries

.loop_reftab:
        mov rsi, [rbp - 32]
        mov rcx, [rbp - 40]
        cmp rcx, 0
        je .end
        mov dl, 0
        cmp [rsi], dl           ; if (reftab[0].label == '\0')
        je .end

        mov eax, [rsi + 244]
        mov [rbp - 12], eax     ; flags = reftab[0].flags
        mov eax, [rsi + 248]
        mov [rbp - 16], eax     ; rel_target = reftab[0].rel_target
        mov eax, [rsi + 252]
        mov [rbp - 20], eax     ; offset = reftab[0].offset

        mov rdi, [rbp - 32]     ; char *label = &reftab[0].label
        mov r8, [rbp - 8]
        mov rsi, [r8 + 32]      ; struct SymTabNtr *symtab = ctx->symtab
        mov rdx, [r8 + 40]      ; size_t n = ctx->max_symtab_entries
        lea rcx, [rbp - 24]     ; uint32_t *offset = &target_offset
        mov r8, 0               ; uint32_t *flags = 0
        mov r9, 0               ; uint32_t *rel_target = 0
        call rslvref

        cmp eax, 1
        je .err_invalid_ref

        mov eax, [rbp - 12]
        cmp eax, 0x1            ; if (flags == FLAG_RELATIVE)
        je .resolve_relative

.resolve_absolute:
        mov rax, 0
        mov eax, [rbp - 20]
        mov rdi, [rbp - 8]
        mov rsi, [rdi + 8]
        add rsi, rax
        mov eax, [rbp - 24]
        mov [rsi], eax          ; memcpy(ctx->bintxt + offset, target_offset, 4)
        jmp .loop_reftab_end

.resolve_relative:
        mov rax, 0
        mov eax, [rbp - 20]
        mov ecx, [rbp - 24]
        mov edx, [rbp - 16]
        sub ecx, edx            ; target_offset -= rel_target
        mov rdi, [rbp - 8]
        mov rsi, [rdi + 8]
        add rsi, rax
        mov [rsi], ecx          ; memcpy(ctx->bintxt + offset, target_offset, 4)

.loop_reftab_end:
        mov rsi, [rbp - 32]
        add rsi, 256
        mov [rbp - 32], rsi     ; reftab++
        mov rcx, [rbp - 40]
        dec rcx
        mov [rbp - 40], rcx     ; max_reftab_entries--
        jmp .loop_reftab

.end:
        xor eax, eax            ; Return `ERR_NONE`
        mov rsp, rbp
        pop rbp
        retn

.err_invalid_ref:
        mov rdi, [rbp - 8]
        lea rdi, [rdi + 80]
        mov rsi, [rbp - 32]

.copy_label_loop:
        mov cx, 240
        mov al, [rsi]
        movsb
        dec cx
        jz .ret_err_invalid_ref
        cmp al, 0
        jne .copy_label_loop

.ret_err_invalid_ref:
        mov eax, 3            ; Return `ERR_UNKNOWN_REFERENCE`
        mov rsp, rbp
        pop rbp
        retn

;;; rdi: `char const **assembly`
skp2nxtop:
        push rbp
        mov rbp, rsp
        sub rsp, 8
        mov [rbp - 8], rdi

        call skp2lbinst         ; Skip to the comma between the operands
        mov rdi, [rbp - 8]
        mov rsi, [rdi]
        inc rsi
        mov [rdi], rsi          ; (*assembly)++

        call skp2lbinst         ; Skip to the next operand

        mov rsp, rbp
        pop rbp
        retn

;;; rdi: `struct AsmOp *op`
strdspmodrmmod:
        mov eax, [rdi + 12]     ; eax = op->disp.disp32
        cmp eax, 0
        je .direct              ; if (op->disp.disp32 == 0)
        cmp eax, 127
        jg .disp32              ; if (op->disp.disp32 > 127)
        cmp eax, -128
        jl .disp32              ; if (op->disp.disp32 < -128)

.disp8:
        mov al, 0b01
        mov [rdi + 4], al       ; op->modrm_mod = MOD_INDIRECT_8
        jmp .end

.disp32:
        mov al, 0b10
        mov [rdi + 4], al       ; op->modrm_mod = MOD_INDIRECT_32
        jmp .end

.direct:
        mov al, 0b00
        mov [rdi + 4], al       ; op->modrm_mod = MOD_INDIRECT
.end:
        retn

;;; rdi: `struct AsmCtx *ctx`
strglbl:
        push rbp
        mov rbp, rsp
        sub rsp, 8
        mov [rbp - 8], rdi
        call skp2lbinst         ; Skip to next token

        mov rsi, [rdi + 64]     ; rsi = ctx->globals
        mov rcx, [rdi + 72]     ; rcx = ctx->max_globals
        mov rax, 0
        mov rdx, 64
        cmp rcx, rax
        je .ret_err

.find_free_entry_loop:
        cmp [rsi], al
        je .store_label
        add rsi, rdx
        dec rcx
        jz .ret_err
        jmp .find_free_entry_loop

.store_label:
        ;; Variables are already set
        ;; rdi: `char const **assembly`
        ;; rsi: `char *label`
        ;; rdx: `size_t n = 64`
        call readnlbl
        jmp .ret

.ret_err:
        mov rax, -1

.ret:
        mov rsp, rbp
        pop rbp
        retn

;;; rdi: `struct AsmCtx *ctx`
strlbl:
        push rbp
        mov rbp, rsp
        sub rsp, 8
        mov [rbp - 8], rdi

        mov rsi, [rdi]          ; Stores ctx->assembly in ri
        mov al, 0x2e            ; Ascii dot ('.')
        cmp [rsi], al
        jne .store_top_label

.store_sub_label:
        mov rdi, [rbp - 8]      ; Stores ctx in rdi
        lea rsi, [rdi + 80]     ; void *dst = ctx->label
        lea rdi, [rdi + 320]    ; void *src = ctx->_label
        mov rdx, 240            ; size_t n = 240
        call cpy

        mov rdi, [rbp - 8]
        lea rdi, [rdi + 320]    ; void *str = ctx->_label
        call len

        mov rdi, [rbp - 8]      ; char **assembly = &ctx->assembly
        lea rsi, [rdi + 80]     ; char *label = ctx->label
        add rsi, rax            ; sublabel += len(ctx->_label)
        dec rsi
        mov rdx, 240            ; size_t n = 240
        sub rdx, rax            ; n -= len(ctx->_label)
        call readnlbl

        jmp .end

.store_top_label:
        lea rdi, [rdi + 80]
        mov rsi, 480
        call clr

        mov rdi, [rbp - 8]      ; char **assembly = &ctx->assembly
        lea rsi, [rdi + 80]     ; char *label = ctx->label
        mov rdx, 240            ; size_t n = 240
        call readnlbl

        mov rdi, [rbp - 8]      ; Stores ctx in rdi
        lea rsi, [rdi + 320]    ; void *dst = ctx->_label
        lea rdi, [rdi + 80]     ; void *src = ctx->label
        mov rdx, 240            ; size_t n = 240
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
        mov al, 0x20            ; Ascii whitespace
        mov ah, 0x9             ; Ascii tab
        mov cl, 0xa             ; Ascii newline
        mov ch, 0x0             ; Ascii null terminator
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
        mov al, 0xa             ; Ascii newline
        mov ah, 0x0             ; Ascii null terminator
.skip_till_next_line_loop:
        cmp [rsi], al
        je .skip_whitespace
        cmp [rsi], ah
        je .end
        inc rsi
        jmp .skip_till_next_line_loop

.analyze:
        mov al, 0x3b            ; Ascii semicolon
        cmp [rsi], al
        je .skip_till_next_line

.end:
        mov [rdi], rsi
        pop rbp
        retn

;;; rdi: `struct SymTabNtr *symtab`
;;; rsi: `size_t n`
symtablen:
        xor rax, rax
        xor rcx, rcx
        mov rdx, 256

.loop:
        cmp rsi, rax
        je .end
        cmp [rdi], cl
        je .end
        inc rax
        add rdi, rdx
        jmp .loop
.end:
        retn

;;; rdi: `struct AsmCtx *ctx`
symtabnglbls:
        push rbp
        mov rbp, rsp
        sub rsp, 32
        mov [rbp - 8], rdi      ; struct AsmCtx *ctx = ctx
        mov rsi, [rdi + 32]
        mov [rbp - 16], rsi     ; struct SymTabNtr *symtab = ctx->symtab
        xor eax, eax
        mov [rbp - 24], rax     ; size_t i = 0
        mov [rbp - 32], rax     ; size_t n = 0

.loop:
        mov rcx, [rdi + 40]     ; rcx = ctx->max_symtab_entries
        mov rax, [rbp - 24]     ; rax = i
        cmp rcx, rax            ; if (ctx->max_symtab_entries == i)
        je .end
        xor eax, eax
        mov rsi, [rbp - 16]
        cmp [rsi], al
        je .end
        call isglbl             ; if (isglbl(ctx, symtab[i].label))
        cmp rax,  1
        jne .loop_next
        mov rax, [rbp - 32]
        inc rax
        mov [rbp - 32], rax     ; n += 1
.loop_next:
        mov rdi, [rbp - 8]
        mov rsi, [rbp - 16]
        add rsi, 256
        mov [rbp - 16], rsi     ; symtab += sizeof(struct SymTabNtr)
        mov rax, [rbp - 24]
        inc rax
        mov [rbp - 24], rax     ; i += 1
        jmp .loop

.end:
        mov rax, [rbp - 32]     ; return n
        mov rsp, rbp
        pop rbp
        retn
