/*
 *  Copyright (c) 2026 by Karsten Lehmann <mail@kalehmann.de>
 *
 *  This file is part of 0x864.
 *
 *  0x864 is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  0x864 is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  long with 0x864. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _0x864_H
#define _0x864_H

#include <stddef.h>
#include <stdint.h>

// Possible values for the `encoding` field of the AsmOp structure.
#define ENCODING_ZO 0x00
#define ENCODING_D 0x01
#define ENCODING_I 0x02
#define ENCODING_M 0x03
#define ENCODING_MI 0x04
#define ENCODING_MR 0x05
#define ENCODING_O 0x06
#define ENCODING_OI 0x07
#define ENCODING_RM 0x08

// Possible values for the `flags` field of the SymTabNtr structure.
#define FLAG_RELATIVE 0x01

// Possible values for the `d_label` field of the AsmOp structure.
#define D_LABEL_NONE 0x00
#define D_LABEL_ABSOLUTE 0x01
#define D_LABEL_RELATIVE 0x02

// Possible values for the `modrm_mod` field of the AsmOp structure.
#define MOD_INDIRECT 0b00
#define MOD_INDIRECT_8 0b01
#define MOD_INDIRECT_32 0b10
#define MOD_DIRECT 0b11

// Possible encodings of operand types for ckoptps
#define OP_TYPE_REG 0b00
#define OP_TYPE_RGNDRCT 0b01
#define OP_TYPE_IMM 0b10
#define OP_TYPE_LBL 0b11

// Possible values for the `prefix` field of the AsmOp structure.
#define PREFIX_LOCK 0x01
#define PREFIX_REPNE_REPNZ 0x02
#define PREFIX_REPE_REPZ 0x04
#define PREFIX_OP_SIZE_OVERRIDE 0x08

enum AsmErr {
        ERR_NONE = 0,
        ERR_UNKNOWN_INSTRUCTION = 1,
        ERR_INVALID_OPERANDS = 2,
        ERR_UNKNOWN_REFERENCE = 3,
        ERR_TOO_MANY_GLOBALS = 4,
        ERR_TOO_MANY_LABELS = 5,
        ERR_TOO_MANY_REFERENCES = 6,
};

struct SymTabNtr {
        char label[240];
        uint32_t __reserved;
        uint32_t flags;
        uint32_t rel_target;
        uint32_t offset;
};

struct AsmCtx {
        char const *assembly;
        uint8_t *bintxt;
        size_t max_bintxt_size;
        size_t bintxt_size;
        struct SymTabNtr *symtab;
        size_t max_symtab_entries;
        struct SymTabNtr *reftab;
        size_t max_reftab_entries;
        char (*globals)[64];
        size_t max_globals;
        char label[240];
        char _label[240];
};

union Disp{
        int8_t disp8;
        int32_t disp32;
};

union Immediate{
        int8_t imm8;
        int16_t imm16;
        int32_t imm32;
        int64_t imm64;
};

struct AsmOp {
        uint8_t encoding;
        /**
         * Size of the operation. 8, 16, 32 or 64 bit.
         */
        uint8_t op_size;
        /**
         * Source register of the operation encoded as 4 bit value.
         */
        uint8_t src_reg;
        /**
         * Destination register of the operation encoded as 4 bit value.
         */
        uint8_t dst_reg;
        /**
         * The two mod bits of the ModRM byte.
         */
        uint8_t modrm_mod;
        /**
         * The number of opcodes stored in the `opcodes` array.
         * - 00 -> register indirect
         * - 01 -> register indirect + `disp.disp8`
         * - 10 -> register indirect + `disp.disp32`
         * - 11 -> register direct
         */
        uint8_t n_opcodes;
        /**
         * The number of opcodes of the operation.
         */
        uint8_t opcodes[3];
        /**
         * The size of the immediate value in bits.
         * This value specifies whether `imm.imm8`, `imm.imm16`, `imm.imm32`
         * or `imm.imm64` shall be used.
         */
        uint8_t imm_size;
        /**
         * For encoding `ENCODING_D`, this specifies if the value from the `imm`
         * field should be used as targed or the address referenced by the label
         * currently stored in the context.
         */
        uint8_t d_label;
        /**
         * Additional prefixes for the instruction
         */
        uint8_t prefix;
        /**
         * The displacement of an register indirect access. The usage of
         * `disp.disp8` or `disp.disp32` is determined by the `modrm_mod`
         * field.
         */
        union Disp disp;
        /**
         * The intermediate value to encode. See the `imm_size` field.
         */
        union Immediate imm;
};

/**
 * Allocates a AsmCtx structure including it's members.
 *
 * @param assembly is a pointer to the string containing the assembly code
 * @param max_bintxt_size is the size of the buffer allocated for binary output
 * @param max_symbtab_entries is the number of entries for the allocated symbol
 *                            table. This parameter should be larger than the
 *                            number of labels in the assembly could.
 * @param max_reftab_entries is the number of entries for the allocated reference
 *                           table. This parameter should be largern than the
 *                           number of jump and call instructions in the assembly
 *                           code, that use labels as target.
 * @param max_globals is the number of entries for the globals table. This
 *                    parameter should be largern than the number of global
 *                    statements in the assembly code.
 *
 * @returns a pointer to the newly allocated AsmCtx structure or NULL on failure
 */
struct AsmCtx *make_asmctx(char const *assembly, size_t max_bintxt_size,
                           size_t max_symtab_entries, size_t max_reftab_entries,
                           size_t max_globals);

/**
 * Cleans up a AsmCtx structure including all it's members allocated by
 * `max_asmctx`.
 *
 * @param ctx is the pointer to the AsmCtx structure.
 */
void free_asmctx(struct AsmCtx *ctx);

/**
 * Aligns an offset to a multiple of 16.
 *
 * @param off is the offset to align
 *
 * @returns the aligned offset
 */
extern size_t algn16(size_t off);

/**
 * Assembles the given assembly.
 *
 * Fills the `bintxt` field and sets the `bintxt_size` field of the AsmCtx
 * structure. The `assembly` field of the AsmCtx structure is advanced to the
 * end.
 *
 * @param ctx is the pointer to the AsmCtx structure.
 *
 * @returns `ERR_NONE` on success or any error, that occured
 */
extern enum AsmErr assemble(struct AsmCtx *);

/**
 * Encodes an instruction described by the AsmOp structure.
 *
 * @param ctx is the pointer to the AsmCtx structure.
 * @param op is the pointer to the AsmOp structure.
 *
 * @returns `ERR_NONE` on success or any error, that occured
 */
extern enum AsmErr assemble_op(struct AsmCtx *ctx, struct AsmOp *op);

/**
 * Assembles a single instruction.
 *
 * @param ctx is the pointer to the AsmCtx structure.
 *
 * @returns `ERR_NONE` on success or any error, that occured
 */
extern enum AsmErr as_snglinst(struct AsmCtx *ctx);

/**
 * Assembles the add instruction.
 *
 * @param ctx is the pointer to the AsmCtx structure.
 * @param op is a pointer to an empty AsmOp structure, that should be filled
 *           with data about the encoded instruction.
 *
 * @returns `ERR_NONE` on success or any error, that occured
 */
extern enum AsmErr as_add(struct AsmCtx *ctx, struct AsmOp *op);

/**
 * Assembles the and instruction.
 *
 * @param ctx is the pointer to the AsmCtx structure.
 * @param op is a pointer to an empty AsmOp structure, that should be filled
 *           with data about the encoded instruction.
 *
 * @returns `ERR_NONE` on success or any error, that occured
 */
extern enum AsmErr as_and(struct AsmCtx *ctx, struct AsmOp *op);

/**
 * Assembles the call instruction.
 *
 * @param ctx is the pointer to the AsmCtx structure.
 * @param op is a pointer to an empty AsmOp structure, that should be filled
 *           with data about the encoded instruction.
 *
 * @returns `ERR_NONE` on success or any error, that occured
 */
extern enum AsmErr as_call(struct AsmCtx *ctx, struct AsmOp *op);

/**
 * Assembles the cmp instruction.
 *
 * @param ctx is the pointer to the AsmCtx structure.
 * @param op is a pointer to an empty AsmOp structure, that should be filled
 *           with data about the encoded instruction.
 *
 * @returns `ERR_NONE` on success or any error, that occured
 */
extern enum AsmErr as_cmp(struct AsmCtx *ctx, struct AsmOp *op);

/**
 * Assembles the dec instruction.
 *
 * @param ctx is the pointer to the AsmCtx structure.
 * @param op is a pointer to an empty AsmOp structure, that should be filled
 *           with data about the encoded instruction.
 *
 * @returns `ERR_NONE` on success or any error, that occured
 */
extern enum AsmErr as_dec(struct AsmCtx *ctx, struct AsmOp *op);

/**
 * Assembles the div instruction.
 *
 * @param ctx is the pointer to the AsmCtx structure.
 * @param op is a pointer to an empty AsmOp structure, that should be filled
 *           with data about the encoded instruction.
 *
 * @returns `ERR_NONE` on success or any error, that occured
 */
extern enum AsmErr as_div(struct AsmCtx *ctx, struct AsmOp *op);

/**
 * Assembles a generic jump instruction.
 *
 * Suitable as generic as generic implementation for the `call`, `jmp` or `j*`
 * (jcc) instructions. The instruction operand encoding is always D and the
 * offset rel32 with no optimizations for shorter jump offsets.
 *
 * @param ctx is a pointer to the AsmCtx structure
 * @param op is a pointer to the AsmOp structure
 * @param opcodes are the opcodes for the instruction
 * @param n_opcodes is the number of opcodes (one or two) in the `opcodes`
 *                  parameters
 *
 * @returns `ERR_NONE` on success or any error, that occured.
 */
extern enum AsmErr as_genjmp(struct AsmCtx *ctx, struct AsmOp *op,
                             uint16_t opcodes, uint8_t n_opcodes);

/**
 * Assembles a generic 1 operand instruction with an R/M destination.
 *
 * Suitable as generic as generic implementation for example the `dec`, `div`,
 * `inc` or `mul` instructions.
 *
 * This function can be used as a generic implementation of a instruction under
 * the following assumptions:
 *
 * - the operation size matches the source register size
 * - the instruction operand encoding is M
 * - the 8-bit operation uses a single opcode
 * - the 16-bit operation uses the opcode of the 8-bit operation plus one and
 *   the operand size override prefix (0x66)
 * - the 32-bit operation uses the opcode of the 8-bit operation plus one
 * - the 64-bit operation uses the opcode of the 8-bit operation plus one and
 *   the REX.W bit
 * - there may be additional data in the ModRM.reg field
 *
 * @param ctx is a pointer to the AsmCtx structure
 * @param op is a pointer to the AsmOp structure
 * @param op8_rm8 is the opcode for the 8-bit operation
 * @param modrm_reg is additional data to encode in the ModRM.reg
 *
 * @returns `ERR_NONE` on success or any error, that occured.
 */
extern enum AsmErr as_genop1rm(struct AsmCtx *ctx, struct AsmOp *op,
                               uint8_t op_rm8, uint8_t modrm_reg);

/**
 * Assembles a generic 2 operand instruction with special handling for `ax`.
 *
 * Suitable as generic as generic implementation for example the `add`, `and`,
 * `cmp`, `or`, `sub` or `xor` instructions.
 *
 * This function can be used as a generic implementation of a instruction under
 * the following assumptions:
 *
 * - the operation size matches the source register size
 * - the 64 bit operation uses a 32 bit immediate
 * - the instruction operand encoding is I for `ax` destination and an immediate
 *   as source
 * - the instruction operand encoding is MI for any other register as destination
 *   and an immediate as source
 * - the instruction operand encoding is MR for R/M as destination and a register
 *   as source
 * - the instruction operand encoding is RM for a register as destination and R\M
 *   as source
 * - the 8-bit operation uses a single opcode
 * - the 16-bit operation uses the opcode of the 8-bit operation plus one and
 *   the operand size override prefix (0x66)
 * - the 32-bit operation uses the opcode of the 8-bit operation plus one
 * - the 64-bit operation uses the opcode of the 8-bit operation plus one and
 *   the REX.W bit
 * - if the operation has a register as destination, an immediate as source and
 *   that immediate fits into a `int8_t`, the operation has an opcode to
 *   sign-extend the immediate to the register size. The 16-bit operation then
 *   utilizes the operand size override prefix and the 64-bit operation sets the
 *   REX.W bit while the opcode stays the same
 * - the MI encoding may have additional data in the ModRM.reg field
 *
 * @param ctx is a pointer to the AsmCtx structure
 * @param op is a pointer to the AsmOp structure
 * @param op8_al_imm8 is the opcode for the 8-bit operation with `al` as
 *                    destination and an immediate as source.
 * @param op8_rimm8 is the opcode for the 8-bit operation with any register other
 *                  than `al` as destination and an immediate as source.
 * @param op8_rsimm8 is the opcode for the operation with a register of any size
 *                   and a sign-extended 8-bit immediate as source.
 * @param op8_rmr8 is the opcode for the 8-bit operation with a R\M as source
 *                 and a register as destination.
 * @param op8_rrm8 is the opcode for the 8-bit operation with a register as
 *                 source and R/M as destination.
 * @param modrm_reg is additional data to encode in the ModRM.reg field for the I
 *                  instruction operand encoding.
 *
 * @returns `ERR_NONE` on success or any error, that occured.
 */
extern enum AsmErr as_genop2ax32(struct AsmCtx *ctx, struct AsmOp *op,
                                 uint8_t op_al_imm8, uint8_t op_rimm8,
                                 uint8_t op_rsimm8, uint8_t op_rmr8,
                                 uint8_t op_rrm8, uint8_t modrm_reg);

/**
 * Assembles the inc instruction.
 *
 * @param ctx is the pointer to the AsmCtx structure.
 * @param op is a pointer to an empty AsmOp structure, that should be filled
 *           with data about the encoded instruction.
 *
 * @returns `ERR_NONE` on success or any error, that occured
 */
extern enum AsmErr as_inc(struct AsmCtx *ctx, struct AsmOp *op);

/**
 * Assembles the int instruction.
 *
 * @param ctx is the pointer to the AsmCtx structure.
 * @param op is a pointer to an empty AsmOp structure, that should be filled
 *           with data about the encoded instruction.
 *
 * @returns `ERR_NONE` on success or any error, that occured
 */
extern enum AsmErr as_int(struct AsmCtx *ctx, struct AsmOp *op);

/**
 * Assembles the ja instruction.
 *
 * @param ctx is the pointer to the AsmCtx structure.
 * @param op is a pointer to an empty AsmOp structure, that should be filled
 *           with data about the encoded instruction.
 *
 * @returns `ERR_NONE` on success or any error, that occured
 */
extern enum AsmErr as_ja(struct AsmCtx *ctx, struct AsmOp *op);

/**
 * Assembles the jb instruction.
 *
 * @param ctx is the pointer to the AsmCtx structure.
 * @param op is a pointer to an empty AsmOp structure, that should be filled
 *           with data about the encoded instruction.
 *
 * @returns `ERR_NONE` on success or any error, that occured
 */
extern enum AsmErr as_jb(struct AsmCtx *ctx, struct AsmOp *op);

/**
 * Assembles the je instruction.
 *
 * @param ctx is the pointer to the AsmCtx structure.
 * @param op is a pointer to an empty AsmOp structure, that should be filled
 *           with data about the encoded instruction.
 *
 * @returns `ERR_NONE` on success or any error, that occured
 */
extern enum AsmErr as_je(struct AsmCtx *ctx, struct AsmOp *op);

/**
 * Assembles the jmp instruction.
 *
 * @param ctx is the pointer to the AsmCtx structure.
 * @param op is a pointer to an empty AsmOp structure, that should be filled
 *           with data about the encoded instruction.
 *
 * @returns `ERR_NONE` on success or any error, that occured
 */
extern enum AsmErr as_jmp(struct AsmCtx *ctx, struct AsmOp *op);

/**
 * Assembles the jne instruction.
 *
 * @param ctx is the pointer to the AsmCtx structure.
 * @param op is a pointer to an empty AsmOp structure, that should be filled
 *           with data about the encoded instruction.
 *
 * @returns `ERR_NONE` on success or any error, that occured
 */
extern enum AsmErr as_jne(struct AsmCtx *ctx, struct AsmOp *op);

/**
 * Assembles the lea instruction.
 *
 * @param ctx is the pointer to the AsmCtx structure.
 * @param op is a pointer to an empty AsmOp structure, that should be filled
 *           with data about the encoded instruction.
 *
 * @returns `ERR_NONE` on success or any error, that occured
 */
extern enum AsmErr as_lea(struct AsmCtx *ctx, struct AsmOp *op);

/**
 * Assembles the mov instruction.
 *
 * @param ctx is the pointer to the AsmCtx structure.
 * @param op is a pointer to an empty AsmOp structure, that should be filled
 *           with data about the encoded instruction.
 *
 * @returns `ERR_NONE` on success or any error, that occured
 */
extern enum AsmErr as_mov(struct AsmCtx *ctx, struct AsmOp *op);

/**
 * Assembles the movsb instruction.
 *
 * @param ctx is the pointer to the AsmCtx structure.
 * @param op is a pointer to an empty AsmOp structure, that should be filled
 *           with data about the encoded instruction.
 *
 * @returns `ERR_NONE` on success or any error, that occured
 */
extern enum AsmErr as_movsb(struct AsmCtx *ctx, struct AsmOp *op);

/**
 * Assembles the movsd instruction.
 *
 * @param ctx is the pointer to the AsmCtx structure.
 * @param op is a pointer to an empty AsmOp structure, that should be filled
 *           with data about the encoded instruction.
 *
 * @returns `ERR_NONE` on success or any error, that occured
 */
extern enum AsmErr as_movsd(struct AsmCtx *ctx, struct AsmOp *op);

/**
 * Assembles the movsq instruction.
 *
 * @param ctx is the pointer to the AsmCtx structure.
 * @param op is a pointer to an empty AsmOp structure, that should be filled
 *           with data about the encoded instruction.
 *
 * @returns `ERR_NONE` on success or any error, that occured
 */
extern enum AsmErr as_movsq(struct AsmCtx *ctx, struct AsmOp *op);

/**
 * Assembles the movsw instruction.
 *
 * @param ctx is the pointer to the AsmCtx structure.
 * @param op is a pointer to an empty AsmOp structure, that should be filled
 *           with data about the encoded instruction.
 *
 * @returns `ERR_NONE` on success or any error, that occured
 */
extern enum AsmErr as_movsw(struct AsmCtx *ctx, struct AsmOp *op);
/**
 * Assembles the mul instruction.
 *
 * @param ctx is the pointer to the AsmCtx structure.
 * @param op is a pointer to an empty AsmOp structure, that should be filled
 *           with data about the encoded instruction.
 *
 * @returns `ERR_NONE` on success or any error, that occured
 */
extern enum AsmErr as_mul(struct AsmCtx *ctx, struct AsmOp *op);

/**
 * Assembles the nop instruction.
 *
 * @param ctx is the pointer to the AsmCtx structure.
 * @param op is a pointer to an empty AsmOp structure, that should be filled
 *           with data about the encoded instruction.
 *
 * @returns `ERR_NONE` on success or any error, that occured
 */
extern enum AsmErr as_nop(struct AsmCtx *ctx, struct AsmOp *op);

/**
 * Assembles the or instruction.
 *
 * @param ctx is the pointer to the AsmCtx structure.
 * @param op is a pointer to an empty AsmOp structure, that should be filled
 *           with data about the encoded instruction.
 *
 * @returns `ERR_NONE` on success or any error, that occured
 */
extern enum AsmErr as_or(struct AsmCtx *ctx, struct AsmOp *op);

/**
 * Assembles the pop instruction.
 *
 * @param ctx is the pointer to the AsmCtx structure.
 * @param op is a pointer to an empty AsmOp structure, that should be filled
 *           with data about the encoded instruction.
 *
 * @returns `ERR_NONE` on success or any error, that occured
 */
extern enum AsmErr as_pop(struct AsmCtx *ctx, struct AsmOp *op);

/**
 * Assembles the push instruction.
 *
 * @param ctx is the pointer to the AsmCtx structure.
 * @param op is a pointer to an empty AsmOp structure, that should be filled
 *           with data about the encoded instruction.
 *
 * @returns `ERR_NONE` on success or any error, that occured
 */
extern enum AsmErr as_push(struct AsmCtx *ctx, struct AsmOp *op);

/**
 * Assembles the retn instruction.
 *
 * @param ctx is the pointer to the AsmCtx structure.
 * @param op is a pointer to an empty AsmOp structure, that should be filled
 *           with data about the encoded instruction.
 *
 * @returns `ERR_NONE` on success or any error, that occured
 */
extern enum AsmErr as_retn(struct AsmCtx *ctx, struct AsmOp *op);

/**
 * Assembles the shl instruction.
 *
 * @param ctx is the pointer to the AsmCtx structure.
 * @param op is a pointer to an empty AsmOp structure, that should be filled
 *           with data about the encoded instruction.
 *
 * @returns `ERR_NONE` on success or any error, that occured
 */
extern enum AsmErr as_shl(struct AsmCtx *ctx, struct AsmOp *op);

/**
 * Assembles the shr instruction.
 *
 * @param ctx is the pointer to the AsmCtx structure.
 * @param op is a pointer to an empty AsmOp structure, that should be filled
 *           with data about the encoded instruction.
 *
 * @returns `ERR_NONE` on success or any error, that occured
 */
extern enum AsmErr as_shr(struct AsmCtx *ctx, struct AsmOp *op);

/**
 * Assembles the sub instruction.
 *
 * @param ctx is the pointer to the AsmCtx structure.
 * @param op is a pointer to an empty AsmOp structure, that should be filled
 *           with data about the encoded instruction.
 *
 * @returns `ERR_NONE` on success or any error, that occured
 */
extern enum AsmErr as_sub(struct AsmCtx *ctx, struct AsmOp *op);

/**
 * Assembles the syscall instruction.
 *
 * @param ctx is the pointer to the AsmCtx structure.
 * @param op is a pointer to an empty AsmOp structure, that should be filled
 *           with data about the encoded instruction.
 *
 * @returns `ERR_NONE` on success or any error, that occured
 */
extern enum AsmErr as_syscall(struct AsmCtx *ctx, struct AsmOp *op);

/**
 * Assembles the xchg instruction.
 *
 * @param ctx is the pointer to the AsmCtx structure.
 * @param op is a pointer to an empty AsmOp structure, that should be filled
 *           with data about the encoded instruction.
 *
 * @returns `ERR_NONE` on success or any error, that occured
 */
extern enum AsmErr as_xchg(struct AsmCtx *ctx, struct AsmOp *op);

/**
 * Assembles the xor instruction.
 *
 * @param ctx is the pointer to the AsmCtx structure.
 * @param op is a pointer to an empty AsmOp structure, that should be filled
 *           with data about the encoded instruction.
 *
 * @returns `ERR_NONE` on success or any error, that occured
 */
extern enum AsmErr as_xor(struct AsmCtx *ctx, struct AsmOp *op);

/**
 * Checks if the next token in the assembly text is a label.
 *
 * @param assembly is a pointer to the assembly string.
 *
 * @returns 1 if the next token is a label or 0 otherwise
 */
extern int cklb(char const *assembly);

/**
 * Get the current line in the assmebly text.
 *
 * @param assembly is the programs code in assembly language
 * @param cpos is a pointer to the current position of the parser in the
 *        assembly language text
 *
 * @returns the line in the assembly text, that the current position is on
 */
extern size_t ckln(char const *assembly, char const *cpos);

/**
 * Get the size of an operation from it's operands.
 *
 * @param assembly is a pointer to the assembly string.
 * @param n_ops is the number of operands
 *
 * @returns the size of the operation in bits (8, 16, 32 or 64)
 */
extern uint8_t ckopsize(char const *assembly, uint8_t n_ops);

/**
 * Returns the types of the operands for an instruction
 *
 * @param assembly is a pointer to the assembly string.
 * @param n_ops is the number of operands
 *
 * @returns the encoded operand types. The i-th operand is can by obtained from
 *          the result by `(ckopts(...) >> ((n_ops - i) * 4)) & 0xf`
 */
extern uint16_t ckoptps(char const *assembly, uint8_t n_ops);

/**
 * Fills a buffer with zeros.
 *
 * @param buf is a pointer to the buffer
 * @param n is size of the buffer in bytes
 */
extern void clr(void *buf, size_t n);

/**
 * Copies bytes from one buffer into another.
 *
 * @param src is a pointer to the source buffer
 * @param dst is a pointer to the destination buffer
 * @param n is the number of bytes to copy.
 */
extern void cpy(void *src, void *dest, size_t n);

/**
 * Calculates the size of a `.shstrtab` section in an ELF file.
 *
 * This function considers only the sections `.text`, `.shstrtab`, `.symtab` and
 * `.strtab`
 *
 * @returns the size of the `.shstrtab` section in bytes
 */
extern size_t elf64_clcshstrtabsz(void);

/**
 * Calculates the size of a `.strtab` section in an ELF file.
 *
 * @param ctx is a pointer to the AsmCtx structure
 * @param filename is the name of the source file, that will be embedded in the
 *                 strtab
 *
 * @returns the size of the `.shstrtab` section in bytes
 */
extern size_t elf64_clcstrtabsz(struct AsmCtx *ctx, char *filename);

/**
 * Calculates the size of a `.symtab` section in an ELF file.
 *
 * @param ctx is a pointer to the AsmCtx structure
 *
 * @returns the size of the `.symtab` section in bytes
 */
extern size_t elf64_clcsymtabsz(struct AsmCtx *ctx);

/**
 * Calculates the size of a `.text` section in an ELF file.
 *
 * @param ctx is a pointer to the AsmCtx structure
 *
 * @returns the size of the `.text` section in bytes
 */
extern size_t elf64_clctextsz(struct AsmCtx *ctx);

/**
 * Stores the assembled program as ELF data into a buffer.
 *
 * @param ctx is a pointer to the AsmCtx structure
 * @param buffer is a zero allocated buffer, where the ELF data will be written
 *               to.
 * @param n is the size of the buffer in bytes
 * @param filename is the name of the source file, that will be embedded for
 *                 debugging inside the ELF file.
 *
 * @returns the number of bytes written to the buffer or zero on failure
 */
extern size_t elf64_dump(struct AsmCtx *ctx, void *buffer, size_t n,
                         char *filename);

/**
 * Stores the ELF-header of the assembled program into a buffer.
 *
 * @param ctx is a pointer to the AsmCtx structure
 * @param buffer is a zero allocated buffer, where the ELF data will be written
 *               to.
 * @param n is the size of the buffer in bytes
 *
 * @returns the number of bytes written to the buffer or zero on failure
 */
extern size_t elf64_dump_header(struct AsmCtx *ctx, void *buffer, size_t n);

/**
 * Fills the AsmOp structure for an immediate to ax instruction with 32 bit limit
 *
 * This function can be used as a generic implementation of the instruction under
 * the following assumptions:
 *
 * - the operation size matches the source register size
 * - the 64 bit operation uses a 32 bit immediate
 * - the instruction operand encoding is I
 * - the 8-bit operation uses a single opcode `op8`
 * - the 16-bit operation uses a single opcode `op8 + 1` and the operand size
 *   override prefix (0x66)
 * - the 32-bit operation uses a single opcode `op8 + 1`
 * - the 64-bit operation uses a single opcode `op8 + 1` and the REX.W bit
 *
 * @param ctx is a pointer to the AsmCtx structure
 * @param op is a pointer to the AsmOp structure
 * @param op8 is the opcode for the 8-bit operation.
 */
extern void genop2aximm32(struct AsmCtx *ctx, struct AsmOp *op, uint8_t op8);

/**
 * Fills the AsmOp structure for a immediate to R\M instruction with 32 bit limit
 *
 * This function can be used as a generic implementation of the instruction under
 * the following assumptions:
 *
 * - the operation size matches the source register size
 * - the 64 bit operation uses a 32 bit immediate
 * - the instruction operand encoding is MI
 * - the 8-bit operation uses a single opcode `op8`
 * - the 16-bit operation uses a single opcode `op8 + 1` and the operand size
 *   override prefix (0x66)
 * - the 32-bit operation uses a single opcode `op8 + 1`
 * - the 64-bit operation uses a single opcode `op8 + 1` and the REX.W bit
 * - if the operation has a register as destination, an immediate as source and
 *   that immediate fits into a `int8_t`, the operation has an opcode to
 *   sign-extend the immediate to the register size. The 16-bit operation then
 *   utilizes the operand size override prefix and the 64-bit operation sets the
 *   REX.W bit while the opcode stays the same
 *
 * @param ctx is a pointer to the AsmCtx structure
 * @param op is a pointer to the AsmOp structure
 * @param op8 is the opcode for the 8-bit operation.
 * @param op_simm8 is the opcode for the operation with a sign-extended 8-bit
 *                 immediate.
 * @param modrm_reg is additional data to encode in the ModRM.reg field.
 */
extern void genop2rimm32(struct AsmCtx *ctx, struct AsmOp *op, uint8_t op8,
                         uint8_t op_simm8, uint8_t modrm_reg);

/**
 * Fills the AsmOp structure for a R source and R\M destination instruction.
 *
 * This function can be used as a generic implementation of the instruction under
 * the following assumptions:
 *
 * - the operation size matches the source register size
 * - the instruction operand encoding is MR
 * - the 8-bit operation uses a single opcode `op8`
 * - the 16-bit operation uses a single opcode `op8 + 1` and the operand size
 *   override prefix (0x66)
 * - the 32-bit operation uses a single opcode `op8 + 1`
 * - the 64-bit operation uses a single opcode `op8 + 1` and the REX.W bit
 *
 * @param ctx is a pointer to the AsmCtx structure
 * @param op is a pointer to the AsmOp structure
 * @param op8 is the opcode for the 8-bit operation.
 */
extern void genop2rmr(struct AsmCtx *ctx, struct AsmOp *op, uint8_t op8);

/**
 * Fills the AsmOp structure for a R\M source and R destination instruction.
 *
 * This function can be used as a generic implementation of the instruction under
 * the following assumptions:
 *
 * - the operation size matches the source register size
 * - the instruction operand encoding is RM
 * - the 8-bit operation uses a single opcode `op8`
 * - the 16-bit operation uses a single opcode `op8 + 1` and the operand size
 *   override prefix (0x66)
 * - the 32-bit operation uses a single opcode `op8 + 1`
 * - the 64-bit operation uses a single opcode `op8 + 1` and the REX.W bit
 *
 * @param ctx is a pointer to the AsmCtx structure
 * @param op is a pointer to the AsmOp structure
 * @param op8 is the opcode for the 8-bit operation.
 */
extern void genop2rrm(struct AsmCtx *ctx, struct AsmOp *op, uint8_t op8);

/**
 * Checks whether the given label is declared as global.
 *
 * @param ctx is a pointer to the AsmCtx structure
 * @param label is the label to check
 *
 * @returns one if the label is declared global or zero otherwise
 */
extern int isglbl(struct AsmCtx *ctx, char *label);

/**
 * Checks if the next token is an decimal or hexadecimal integer
 *
 * @param assembly is a pointer to the assembly string.
 *
 * @returns whether the next token is an integer
 */
extern int isint(char *assembly);

/**
 * Checks if the next character is a delimiter for operants.
 *
 * @param assembly is a pointer to the assembly string.
 *
 * @returns whether the next token is a null terminator, tabulator, newline,
 *          space, comma or semicolon
 */
extern int isopdlm(char *assembly);

/**
 * Checks if the next token is a register indirect addressing.
 *
 * @param assembly is a pointer to the assembly string.
 *
 * @returns whether the next token is a register indirect addressing.
 */
extern int isrgndrct(char *assembly);

/**
 * Checks if the next token is a register
 *
 * @param assembly is a pointer to the assembly string.
 *
 * @returns whether the next token is a register
 */
extern int isreg(char *assembly);

/**
 * Returns the length of a string (including the null terminator).
 *
 * @param str is the string
 * @returns the length of the string
 */
extern size_t len(char *str);

/**
 * Parses the next token as integer.
 *
 * @param assembly is a pointer to the string with the assembly code.
 *                 This pointer gets advanced to the character following the
 *                 integer.
 *
 * @returns the unsigned 64 bit integer
 */
extern uint64_t pint(char **assembly);

/**
 * Parses the next token as 8-bit register.
 *
 * @param assembly is a pointer to the string with the assembly code.
 *                 This pointer gets advanced to the character following the
 *                 register.
 *
 * @returns the 4-bit encoded register or `0xff` on failure.
 */
extern int pr8(char **assembly);

/**
 * Parses the next token as 16-bit register.
 *
 * @param assembly is a pointer to the string with the assembly code.
 *                 This pointer gets advanced to the character following the
 *                 register.
 *
 * @returns the 4-bit encoded register or `0xff` on failure.
 */
extern int pr16(char **assembly);

/**
 * Parses the next token as 32-bit register.
 *
 * @param assembly is a pointer to the string with the assembly code.
 *                 This pointer gets advanced to the character following the
 *                 register.
 *
 * @returns the 4-bit encoded register or `0xff` on failure.
 */
extern int pr32(char **assembly);

/**
 * Parses the next token as 64-bit register.
 *
 * @param assembly is a pointer to the string with the assembly code.
 *                 This pointer gets advanced to the character following the
 *                 register.
 *
 * @returns the 4-bit encoded register or `0xff` on failure.
 */
extern int pr64(char **assembly);

/**
 * Parses a register indirect addressing.
 *
 * @param assembly is a pointer to the string with the assembly code.
 *                 This pointer gets advanced to the character following the
 *                 closing square bracket.
 * @param reg is a pointer to the variable where the parsed register will be
 *            stored.
 * @param disp is a pointer to the variable where the parsed displacement will
 *             be stored. If the register indirect access does not specify a
 *             displacement, zero will be stored there.
 */
extern void prgndrct(char **assembly, uint8_t *reg, uint32_t *disp);

/**
 * Parses the next token as register.
 *
 * @param assembly is a pointer to the string with the assembly code.
 *                 This pointer gets advanced to the character following the
 *                 register.
 *
 * @returns the 4 bit encoded register or `0xff` on failure.
 */
extern uint8_t preg(char **assembly);

/**
 * Parses next token as a label and writes it to the `label` parameter.
 * Writes at most `n` bytes (including the null-terminator after the label).
 *
 * @param assembly is a pointer to the string with the assembly code.
 *                 This pointer gets advanced to the character following the
 *                 colon after the label.
 * @param label is a pointer to the string where the label will be written into.
 *              Should be at least `n` bytes in size.
 * @param n is the maximum number of bytes to write into `label`.
 *
 * @returns the number of bytes written or -1 if the label does not fit into `n`
 *          bytes.
 */
extern int readnlbl(char const **assembly, char *label, size_t n);

/**
 * Loads the address of the given label from the symbol table.
 *
 * @param label is the label to search the offset for
 * @param symtab is a pointer to the symbol table
 * @param n is the number of entries in the symbol table
 * @param offset is a pointer to the location where the offset of the label will
 *               be stored on success.
 *
 * @returns 0 on success or 1 in case the label is not found in the symbol table
 */
extern int rslvref(char *label, struct SymTabNtr *symtab, size_t n,
                   uint32_t *offset, uint32_t *flags, uint32_t *rel_target);

/**
 * Performs the second pass of the output generation and resolves all references.
 *
 * @param ctx is the pointer to the AsmCtx structure.
 *
 * @returns `ERR_NONE` on success or `ERR_UNKNOWN_REFERENCE`
 */
extern enum AsmErr scndpss(struct AsmCtx *ctx);

/**
 * Advances the assembly text to the next token.
 *
 * Skips comments, whitespace characters, ...
 *
 * @param assembly is a pointer to the string with the assembly code.
 *                 This pointer gets advanced to the first character of the next
 *                 token.
 */
extern void skp2lbinst(char const **assembly);

/**
 * Advances the assembly text to the next operand of a instruction
 *
 * @param assembly is a pointer to the string with the assembly code.
 *                 This pointer gets advanced to the first character of the next
 *                 operand.
 */
extern void skp2nxtop(char const **assembly);

/**
 * Stores the ModRM.mod bits in the AsmOp structure based on the displacement.
 *
 * @param op is the pointer to the AsmOp structure.
 */
extern void strdspmodrmmod(struct AsmOp *op);

/**
 * Stores the next token as global.
 *
 * @param ctx is the pointer to the AsmCtx structure.
 *
 * @returns the number of bytes stored or -1 on failure.
 */
extern int strglbl(struct AsmCtx *ctx);

/**
 * Stores next token of the assembly text as label in `ctx->label`.
 *
 * If the label starts with a dot, it will be prepended with the previous label
 * that did not start with a dot.
 *
 * @param ctx is the pointer to the AsmCtx structure.
 */
extern void strlbl(struct AsmCtx *ctx);

/**
 * Stores a symbol in the next free entry of the symbol table.
 *
 * @param symtab is a pointer to the symbol table
 * @param n is the number of entries in the symbol table
 * @param label is the label of the new entry to insert
 * @param offset is the offset to store for the label
 * @param flags is only relevant when the symbol table is used to resolve
 *              references and holds additional information how the references
 *              shall be resolved. Available flags:
 *              - 0x01 RESOLVE_RELATIVE
 * @param rel_target is used when the RESOLVE_RELATIVE bit is set in the flags
 *
 * @returns 0 on success or 1 in case there is no free entry in the symbol table
 */
extern int strsymtabntr(struct SymTabNtr *symtab, size_t n, char *label,
                        uint32_t offset, uint32_t flags, uint32_t rel_target);

/**
 * @returns the number of entries in the symbol table
 */
extern size_t symtablen(struct SymTabNtr *symtab, size_t n);

/**
 * @returns the number of global entries in the symbol table
 */
extern size_t symtabnglbls(struct AsmCtx *ctx);

#endif /* _0x864_H */
