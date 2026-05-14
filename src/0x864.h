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

/**
 * @file src/0x864.h
 * @brief Interfaces to the selfh0sted x86 4ssembler.
 *
 * This file contains the constants, structures and function declarations to
 * interface C-code with 0x864.
 *
 * Relevant function for implementing a harness around the Assembler are
 * - @ref make_asmctx to allocate the context structure for the assembly process.
 * - @ref assemble to encode the human readable assembly program as binary.
 * - @ref elf64_dump to produce a relocatable elf64 file from the binary code.
 * - @ref free_asmctx to deallocate the context structure.
 *
 * The rest is only exposed for testing and documentation of the internal
 * structures.
 */

#ifndef _0x864_H
#define _0x864_H

#include <stddef.h>
#include <stdint.h>

/**
 * Possible error values for functions exposed by 0x864
 */
enum AsmErr : uint32_t {
        /**
         * Returned on success.
         */
        ERR_NONE = 0,
        /**
         * Returned in case an instruction not understood by 0x864 (yet) is
         * encountered.
         */
        ERR_UNKNOWN_INSTRUCTION = 1,
        /**
         * Returned in case 0x864 is unable to encode the given operands for the
         * instruction.
         */
        ERR_INVALID_OPERANDS = 2,
        /**
         * Returned in case of a call or jump to a target, that is not in the
         * symbol table.
         */
        ERR_UNKNOWN_REFERENCE = 3,
        /**
         * Returned in case the @ref AsmCtx.globals table does not have enough
         * space to hold all globals declared in the code.
         */
        ERR_TOO_MANY_GLOBALS = 4,
        /**
         * Returned in case the @ref AsmCtx.symtab table does not have enough
         * space to hold all labels declared in the code.
         */
        ERR_TOO_MANY_LABELS = 5,
        /**
         * Returned in case the @ref AsmCtx.reftab table does not have enough
         * space to hold all references to labels from calls or jumps in the
         * assembly code.
         */
        ERR_TOO_MANY_REFERENCES = 6,
        /**
         * Returned in case the @ref AsmCtx.bintxt buffer is smaller than the
         * assembled binary.
         */
        ERR_BINTXT_BUFFER_TOO_SMALL = 7,
};

/**
 * Possible values for the @ref AsmOp.d_label field.
 */
enum AsmOp_d_label : uint8_t {
        /**
         * Use immediate as operand for call or jump.
         */
        D_LABEL_NONE = 0x00,
        /**
         * Use absolute address @ref AsmCtx.label as target for call or jump.
         */
        D_LABEL_ABSOLUTE = 0x01,
        /**
         * Use address @ref AsmCtx.label relative to current position in binary
         * as target for call or jump.
         */
        D_LABEL_RELATIVE = 0x02,
};

/**
 * Possible values for the @ref AsmOp.encoding field.
 */
enum AsmOp_encoding : uint8_t {
        /**
         * No operands.
         */
        ENCODING_ZO = 0x00,
        /**
         * Offset for call or jump as single operand.
         */
        ENCODING_D = 0x01,
        /**
         * An immediate as single operand.
         */
        ENCODING_I = 0x02,
        /**
         * Register stored in the ModRM.rm bits as single operand.
         */
        ENCODING_M = 0x03,
        /**
         * Destination stored in the ModRM.rm bits followed by an immediate.
         */
        ENCODING_MI = 0x04,
        /**
         * Destination stored in the ModRM.rm bits, source stored in the
         * ModRM.reg bits.
         */
        ENCODING_MR = 0x05,
        /**
         * Destination encoded in the opcode.
         */
        ENCODING_O = 0x06,
        /**
         * Destination encoded in the opcode followed by an immediate as source.
         */
        ENCODING_OI = 0x07,
        /**
         * Destination stored in the ModRM.reg bits, source stored in the
         * ModRM.rm bits.
         */
        ENCODING_RM = 0x08,
};

/**
 * Possible values for the @ref AsmOp.modrm_mod field.
 */
enum AsmOp_modrm_mod : uint8_t {
        /**
         * Use value from memory address stored in register as target.
         */
        MOD_INDIRECT = 0b00,
        /**
         * Use value from memory address stored in register plus the following
         * byte as 8-bit signed displacement as target.
         */
        MOD_INDIRECT_8 = 0b01,
        /**
         * Use value from memory address stored in register plus the following
         * four byte as 32-bit signed displacement as target.
         */
        MOD_INDIRECT_32 = 0b10,
        /**
         * Use value stored in register as target.
         */
        MOD_DIRECT = 0b11,
};

/**
 * Possible flags for the @ref AsmOp.prefix field.
 */
enum AsmOp_prefix : uint8_t {
        /**
         * Add the LOCK prefix (`0xf0`) before the opcode.
         */
        PREFIX_LOCK = 0x01,
        /**
         * Add the REPNE/REPNZ prefix (`0xf2`) before the opcode.
         * Note, that this is not valid in combination with
         * @ref AsmOp_prefix.PREFIX_REPE_REPZ.
         */
        PREFIX_REPNE_REPNZ = 0x02,
        /**
         * Add the REP/REPE/REPZ prefix (`0xf3`) before the opcode.
         * Note, that this is not valid in combination with
         * @ref AsmOp_prefix.PREFIX_REPNE_REPNZ.
         */
        PREFIX_REPE_REPZ = 0x04,
        /**
         * Add the Operand-size override prefix (`0x66`) before the opcode.
         */
        PREFIX_OP_SIZE_OVERRIDE = 0x08,
};

/**
 * Possible operand types for the @ref ckoptps function.
 */
enum OpType : uint8_t {
        /**
         * Operand is a register.
         */
        OP_TYPE_REG = 0b00,
        /**
         * Operand is a register indirect memory access, e.g. `[rax]` or
         * `[rbp - 8]`.
         */
        OP_TYPE_RGNDRCT = 0b01,
        /**
         * Operand is an immediate.
         */
        OP_TYPE_IMM = 0b10,
        /**
         * Operand is a label.
         */
        OP_TYPE_LBL = 0b11,
};

/**
 * Possible flags for the @ref SymTabNtr.flags field.
 */
enum SymTabNtr_flags : uint32_t {
        /**
         * No special handling.
         */
        FLAG_NONE = 0x00,
        /**
         * When resolving references, resolve the relative offset to the stored
         * label from the current position in the binary.
         */
        FLAG_RELATIVE = 0x01,
};

/**
 * Union type for displacement values in register indirect memory access.
 */
union Disp{
        /**
         * Signed 8-bit displacement for register-indirect memory access.
         */
        int8_t disp8;
        /**
         * Signed 32-bit displacement for register-indirect memory access.
         */
        int32_t disp32;
};

/**
 * Union type for immediate values.
 */
union Immediate{
        /**
         * 8-bit signed immediate value.
         */
        int8_t imm8;
        /**
         * 16-bit signed immediate value.
         */
        int16_t imm16;
        /**
         * 32-bit signed immediate value.
         */
        int32_t imm32;
        /**
         * 64-bit signed immediate value.
         */
        int64_t imm64;
};

/**
 * Context for the assembly of a script into a binary.
 *
 * This structure holds common information related to the assembly process.
 */
struct AsmCtx {
        /**
         * Pointer to the program in human readable assembly language.
         */
        char const *assembly;
        /**
         * Buffer for the binary / assembled program.
         */
        uint8_t *bintxt;
        /**
         * Size of the @ref AsmCtx.bintxt buffer.
         */
        size_t max_bintxt_size;
        /**
         * Actual size of the binary program in the @ref AsmCtx.bintxt buffer.
         */
        size_t bintxt_size;
        /**
         * Table with all symbols (labels) in the assembly code.
         */
        struct SymTabNtr *symtab;
        /**
         * Number of entries allocated for @ref AsmCtx.symtab.
         */
        size_t max_symtab_entries;
        /**
         * Table with all references to symbols, their location and information
         * how they should be resolved.
         */
        struct SymTabNtr *reftab;
        /**
         * Number of entries allocated for @ref AsmCtx.reftab.
         */
        size_t max_reftab_entries;
        /**
         * Contains each global symbol in the assembly code.
         */
        char (*globals)[64];
        /**
         * Number of entries allocated for @ref AsmCtx.globals.
         */
        size_t max_globals;
        /**
         * Last parsed label. Maybe filled with a label related to an error in
         * case a function return a result other than @ref AsmErr.ERR_NONE.
         */
        char label[240];
        /**
         * Last parsed top-level label (not starting with a dot).
         */
        char _label[240];
};

/**
 * This structures describes an operation, so that it can be encoded.
 */
struct AsmOp {
        /**
         * Specifies the instruction operand encoding.
         */
        enum AsmOp_encoding encoding;
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
         * - 00 -> register indirect
         * - 01 -> register indirect + `disp.disp8`
         * - 10 -> register indirect + `disp.disp32`
         * - 11 -> register direct
         */
        enum AsmOp_modrm_mod modrm_mod;
        /**
         * The number of opcodes stored in the `opcodes` array.
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
         * For encoding @ref AsmOp_encoding.ENCODING_D, this specifies if the
         * value from the @ref AsmOp.imm field should be used as targed or the
         * address referenced by the label currently stored in the context.
         */
        enum AsmOp_d_label d_label;
        /**
         * Additional prefixes for the instruction
         */
        enum AsmOp_prefix prefix;
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
 * Entry in a symbol table like @ref AsmCtx.symtab or @ref AsmCtx.reftab.
 */
struct SymTabNtr {
        /**
         * Name of the symbol
         */
        char label[240];
        /**
         * May be used in future revisions.
         */
        uint32_t __reserved;
        /**
         * When used in a reference table like @ref AsmCtx.reftab, this field
         * contains additional information how the reference will be resolved.
         */
        enum SymTabNtr_flags flags;
        /**
         * When used in a reference table like @ref AsmCtx.reftab and
         * @ref SymTabNtr.flags contains the @ref SymTabNtr_flags.FLAG_RELATIVE,
         * the reference is resolved relative to the address stored in this
         * field.
         */
        uint32_t rel_target;
        /**
         * When used in a symbol table, this field contains the offset of the
         * symbol in the binary.
         *
         * Otherwise, when used in a reference table like @ref AsmCtx.reftab this
         * field contains the offset in the binary where the address of the
         * resolved reference shall be inserted.
         */
        uint32_t offset;
};

/**
 * Allocates a AsmCtx structure including it's members.
 *
 * @param assembly is a pointer to the string containing the assembly code
 * @param max_bintxt_size is the size of the buffer allocated for binary output
 * @param max_symtab_entries is the number of entries for the allocated symbol
 *                           table. This parameter should be larger than the
 *                           number of labels in the assembly could.
 * @param max_reftab_entries is the number of entries for the allocated reference
 *                           table. This parameter should be larger than the
 *                           number of jump and call instructions in the assembly
 *                           code, that use labels as target.
 * @param max_globals is the number of entries for the globals table. This
 *                    parameter should be larger than the number of global
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
 * @code
 * assert(algn16(16) == 16);
 * assert(algn16(17) == 32);
 * @endcode
 *
 * @param off is the offset to align
 *
 * @returns the offset aligned to the next multiple of 16.
 */
extern size_t algn16(size_t off);

/**
 * Assembles the given assembly.
 *
 * Fills the @ref AsmCtx.bintxt field and sets the @ref AsmCtx.bintxt_size field.
 * The pointer @ref AsmCtx.assembly will be advanced to the end of the human
 * readable assembly program in the process.
 *
 * @param ctx is the pointer to the @ref AsmCtx structure.
 *
 * @returns @ref AsmErr.ERR_NONE on success or any error, that occurred
 */
extern enum AsmErr assemble(struct AsmCtx *ctx);

/**
 * Encodes an instruction described by the AsmOp structure.
 *
 * @param ctx is the pointer to the AsmCtx structure.
 * @param op is the pointer to the AsmOp structure.
 *
 * @returns @ref AsmErr.ERR_NONE on success or any error, that occurred
 */
extern enum AsmErr assemble_op(struct AsmCtx *ctx, struct AsmOp *op);

/**
 * Assembles a single instruction.
 *
 * @param ctx is the pointer to the AsmCtx structure.
 *
 * @returns @ref AsmErr.ERR_NONE on success or any error, that occurred
 */
extern enum AsmErr as_snglinst(struct AsmCtx *ctx);

/**
 * Assembles the add instruction.
 *
 * @param ctx is the pointer to the @ref AsmCtx structure.
 * @param op is a pointer to an empty @ref AsmOp structure, that should be
 *           filled with data about the encoded instruction.
 *
 * @returns @ref AsmErr.ERR_NONE on success or any error, that occurred
 */
extern enum AsmErr as_add(struct AsmCtx *ctx, struct AsmOp *op);

/**
 * Assembles the and instruction.
 *
 * @param ctx is the pointer to the @ref AsmCtx structure.
 * @param op is a pointer to an empty @ref AsmOp structure, that should be
 *           filled with data about the encoded instruction.
 *
 * @returns @ref AsmErr.ERR_NONE on success or any error, that occurred
 */
extern enum AsmErr as_and(struct AsmCtx *ctx, struct AsmOp *op);

/**
 * Assembles the call instruction.
 *
 * @param ctx is the pointer to the @ref AsmCtx structure.
 * @param op is a pointer to an empty @ref AsmOp structure, that should be
 *           filled with data about the encoded instruction.
 *
 * @returns @ref AsmErr.ERR_NONE on success or any error, that occurred
 */
extern enum AsmErr as_call(struct AsmCtx *ctx, struct AsmOp *op);

/**
 * Assembles the cmp instruction.
 *
 * @param ctx is the pointer to the @ref AsmCtx structure.
 * @param op is a pointer to an empty @ref AsmOp structure, that should be
 *           filled with data about the encoded instruction.
 *
 * @returns @ref AsmErr.ERR_NONE on success or any error, that occurred
 */
extern enum AsmErr as_cmp(struct AsmCtx *ctx, struct AsmOp *op);

/**
 * Assembles the dec instruction.
 *
 * @param ctx is the pointer to the @ref AsmCtx structure.
 * @param op is a pointer to an empty @ref AsmOp structure, that should be
 *           filled with data about the encoded instruction.
 *
 * @returns @ref AsmErr.ERR_NONE on success or any error, that occurred
 */
extern enum AsmErr as_dec(struct AsmCtx *ctx, struct AsmOp *op);

/**
 * Assembles the div instruction.
 *
 * @param ctx is the pointer to the @ref AsmCtx structure.
 * @param op is a pointer to an empty @ref AsmOp structure, that should be
 *           filled with data about the encoded instruction.
 *
 * @returns @ref AsmErr.ERR_NONE on success or any error, that occurred
 */
extern enum AsmErr as_div(struct AsmCtx *ctx, struct AsmOp *op);

/**
 * Assembles a generic jump instruction.
 *
 * Suitable as generic as generic implementation for the `call`, `jmp` or `j*`
 * (jcc) instructions. The instruction operand encoding is always D and the
 * offset rel32 with no optimizations for shorter jump offsets.
 *
 * @param ctx is a pointer to the @ref AsmCtx structure.
 * @param op is a pointer to the @ref AsmOp structure.
 * @param opcodes are the opcodes for the instruction.
 * @param n_opcodes is the number of opcodes (one or two) in the `opcodes`
 *                  parameters.
 *
 * @returns @ref AsmErr.ERR_NONE on success or any error, that occurred.
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
 * @param ctx is a pointer to the @ref AsmCtx structure.
 * @param op is a pointer to the @ref AsmOp structure.
 * @param op_rm8 is the opcode for the 8-bit operation.
 * @param modrm_reg is additional data to encode in the ModRM.reg
 *
 * @returns @ref AsmErr.ERR_NONE on success or any error, that occurred.
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
 * - the instruction operand encoding is RM for a register as destination and R/M
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
 * @param ctx is a pointer to the @ref AsmCtx structure.
 * @param op is a pointer to the @ref AsmOp structure.
 * @param op_al_imm8 is the opcode for the 8-bit operation with `al` as
 *                   destination and an immediate as source.
 * @param op_rimm8 is the opcode for the 8-bit operation with any register other
 *                 than `al` as destination and an immediate as source.
 * @param op_rsimm8 is the opcode for the operation with a register of any size
 *                  and a sign-extended 8-bit immediate as source.
 * @param op_rmr8 is the opcode for the 8-bit operation with a R/M as source
 *                and a register as destination.
 * @param op_rrm8 is the opcode for the 8-bit operation with a register as
 *                source and R/M as destination.
 * @param modrm_reg is additional data to encode in the ModRM.reg field for the I
 *                  instruction operand encoding.
 *
 * @returns @ref AsmErr.ERR_NONE on success or any error, that occurred.
 */
extern enum AsmErr as_genop2ax32(struct AsmCtx *ctx, struct AsmOp *op,
                                 uint8_t op_al_imm8, uint8_t op_rimm8,
                                 uint8_t op_rsimm8, uint8_t op_rmr8,
                                 uint8_t op_rrm8, uint8_t modrm_reg);

/**
 * Assembles the inc instruction.
 *
 * @param ctx is the pointer to the @ref AsmCtx structure.
 * @param op is a pointer to an empty @ref AsmOp structure, that should be
 *           filled with data about the encoded instruction.
 *
 * @returns @ref AsmErr.ERR_NONE on success or any error, that occurred
 */
extern enum AsmErr as_inc(struct AsmCtx *ctx, struct AsmOp *op);

/**
 * Assembles the int instruction.
 *
 * @param ctx is the pointer to the @ref AsmCtx structure.
 * @param op is a pointer to an empty @ref AsmOp structure, that should be
 *           filled with data about the encoded instruction.
 *
 * @returns @ref AsmErr.ERR_NONE on success or any error, that occurred
 */
extern enum AsmErr as_int(struct AsmCtx *ctx, struct AsmOp *op);

/**
 * Assembles the ja instruction.
 *
 * @param ctx is the pointer to the @ref AsmCtx structure.
 * @param op is a pointer to an empty @ref AsmOp structure, that should be
 *           filled with data about the encoded instruction.
 *
 * @returns @ref AsmErr.ERR_NONE on success or any error, that occurred
 */
extern enum AsmErr as_ja(struct AsmCtx *ctx, struct AsmOp *op);

/**
 * Assembles the jae instruction.
 *
 * @param ctx is the pointer to the @ref AsmCtx structure.
 * @param op is a pointer to an empty @ref AsmOp structure, that should be
 *           filled with data about the encoded instruction.
 *
 * @returns @ref AsmErr.ERR_NONE on success or any error, that occurred
 */
extern enum AsmErr as_jae(struct AsmCtx *ctx, struct AsmOp *op);

/**
 * Assembles the jb instruction.
 *
 * @param ctx is the pointer to the @ref AsmCtx structure.
 * @param op is a pointer to an empty @ref AsmOp structure, that should be
 *           filled with data about the encoded instruction.
 *
 * @returns @ref AsmErr.ERR_NONE on success or any error, that occurred
 */
extern enum AsmErr as_jb(struct AsmCtx *ctx, struct AsmOp *op);

/**
 * Assembles the jbe instruction.
 *
 * @param ctx is the pointer to the @ref AsmCtx structure.
 * @param op is a pointer to an empty @ref AsmOp structure, that should be
 *           filled with data about the encoded instruction.
 *
 * @returns @ref AsmErr.ERR_NONE on success or any error, that occurred
 */
extern enum AsmErr as_jbe(struct AsmCtx *ctx, struct AsmOp *op);

/**
 * Assembles the je instruction.
 *
 * @param ctx is the pointer to the @ref AsmCtx structure.
 * @param op is a pointer to an empty @ref AsmOp structure, that should be
 *           filled with data about the encoded instruction.
 *
 * @returns @ref AsmErr.ERR_NONE on success or any error, that occurred
 */
extern enum AsmErr as_je(struct AsmCtx *ctx, struct AsmOp *op);

/**
 * Assembles the jg instruction.
 *
 * @param ctx is the pointer to the @ref AsmCtx structure.
 * @param op is a pointer to an empty @ref AsmOp structure, that should be
 *           filled with data about the encoded instruction.
 *
 * @returns @ref AsmErr.ERR_NONE on success or any error, that occurred
 */
extern enum AsmErr as_jg(struct AsmCtx *ctx, struct AsmOp *op);

/**
 * Assembles the jl instruction.
 *
 * @param ctx is the pointer to the @ref AsmCtx structure.
 * @param op is a pointer to an empty @ref AsmOp structure, that should be
 *           filled with data about the encoded instruction.
 *
 * @returns @ref AsmErr.ERR_NONE on success or any error, that occurred
 */
extern enum AsmErr as_jl(struct AsmCtx *ctx, struct AsmOp *op);

/**
 * Assembles the jmp instruction.
 *
 * @param ctx is the pointer to the @ref AsmCtx structure.
 * @param op is a pointer to an empty @ref AsmOp structure, that should be
 *           filled with data about the encoded instruction.
 *
 * @returns @ref AsmErr.ERR_NONE on success or any error, that occurred
 */
extern enum AsmErr as_jmp(struct AsmCtx *ctx, struct AsmOp *op);

/**
 * Assembles the jne instruction.
 *
 * @param ctx is the pointer to the @ref AsmCtx structure.
 * @param op is a pointer to an empty @ref AsmOp structure, that should be
 *           filled with data about the encoded instruction.
 *
 * @returns @ref AsmErr.ERR_NONE on success or any error, that occurred
 */
extern enum AsmErr as_jne(struct AsmCtx *ctx, struct AsmOp *op);

/**
 * Assembles the lea instruction.
 *
 * @param ctx is the pointer to the @ref AsmCtx structure.
 * @param op is a pointer to an empty @ref AsmOp structure, that should be
 *           filled with data about the encoded instruction.
 *
 * @returns @ref AsmErr.ERR_NONE on success or any error, that occurred
 */
extern enum AsmErr as_lea(struct AsmCtx *ctx, struct AsmOp *op);

/**
 * Assembles the mov instruction.
 *
 * @param ctx is the pointer to the @ref AsmCtx structure.
 * @param op is a pointer to an empty @ref AsmOp structure, that should be
 *           filled with data about the encoded instruction.
 *
 * @returns @ref AsmErr.ERR_NONE on success or any error, that occurred
 */
extern enum AsmErr as_mov(struct AsmCtx *ctx, struct AsmOp *op);

/**
 * Assembles the movsb instruction.
 *
 * @param ctx is the pointer to the @ref AsmCtx structure.
 * @param op is a pointer to an empty @ref AsmOp structure, that should be
 *           filled with data about the encoded instruction.
 *
 * @returns @ref AsmErr.ERR_NONE on success or any error, that occurred
 */
extern enum AsmErr as_movsb(struct AsmCtx *ctx, struct AsmOp *op);

/**
 * Assembles the movsd instruction.
 *
 * @param ctx is the pointer to the @ref AsmCtx structure.
 * @param op is a pointer to an empty @ref AsmOp structure, that should be
 *           filled with data about the encoded instruction.
 *
 * @returns @ref AsmErr.ERR_NONE on success or any error, that occurred
 */
extern enum AsmErr as_movsd(struct AsmCtx *ctx, struct AsmOp *op);

/**
 * Assembles the movsq instruction.
 *
 * @param ctx is the pointer to the @ref AsmCtx structure.
 * @param op is a pointer to an empty @ref AsmOp structure, that should be
 *           filled with data about the encoded instruction.
 *
 * @returns @ref AsmErr.ERR_NONE on success or any error, that occurred
 */
extern enum AsmErr as_movsq(struct AsmCtx *ctx, struct AsmOp *op);

/**
 * Assembles the movsw instruction.
 *
 * @param ctx is the pointer to the @ref AsmCtx structure.
 * @param op is a pointer to an empty @ref AsmOp structure, that should be
 *           filled with data about the encoded instruction.
 *
 * @returns @ref AsmErr.ERR_NONE on success or any error, that occurred
 */
extern enum AsmErr as_movsw(struct AsmCtx *ctx, struct AsmOp *op);
/**
 * Assembles the mul instruction.
 *
 * @param ctx is the pointer to the @ref AsmCtx structure.
 * @param op is a pointer to an empty @ref AsmOp structure, that should be
 *           filled with data about the encoded instruction.
 *
 * @returns @ref AsmErr.ERR_NONE on success or any error, that occurred
 */
extern enum AsmErr as_mul(struct AsmCtx *ctx, struct AsmOp *op);

/**
 * Assembles the neg instruction.
 *
 * @param ctx is the pointer to the @ref AsmCtx structure.
 * @param op is a pointer to an empty @ref AsmOp structure, that should be
 *           filled with data about the encoded instruction.
 *
 * @returns @ref AsmErr.ERR_NONE on success or any error, that occurred
 */
extern enum AsmErr as_neg(struct AsmCtx *ctx, struct AsmOp *op);

/**
 * Assembles the nop instruction.
 *
 * @param ctx is the pointer to the @ref AsmCtx structure.
 * @param op is a pointer to an empty @ref AsmOp structure, that should be
 *           filled with data about the encoded instruction.
 *
 * @returns @ref AsmErr.ERR_NONE on success or any error, that occurred
 */
extern enum AsmErr as_nop(struct AsmCtx *ctx, struct AsmOp *op);

/**
 * Assembles the or instruction.
 *
 * @param ctx is the pointer to the @ref AsmCtx structure.
 * @param op is a pointer to an empty @ref AsmOp structure, that should be
 *           filled with data about the encoded instruction.
 *
 * @returns @ref AsmErr.ERR_NONE on success or any error, that occurred
 */
extern enum AsmErr as_or(struct AsmCtx *ctx, struct AsmOp *op);

/**
 * Assembles the pop instruction.
 *
 * @param ctx is the pointer to the @ref AsmCtx structure.
 * @param op is a pointer to an empty @ref AsmOp structure, that should be
 *           filled with data about the encoded instruction.
 *
 * @returns @ref AsmErr.ERR_NONE on success or any error, that occurred
 */
extern enum AsmErr as_pop(struct AsmCtx *ctx, struct AsmOp *op);

/**
 * Assembles the push instruction.
 *
 * @param ctx is the pointer to the @ref AsmCtx structure.
 * @param op is a pointer to an empty @ref AsmOp structure, that should be
 *           filled with data about the encoded instruction.
 *
 * @returns @ref AsmErr.ERR_NONE on success or any error, that occurred
 */
extern enum AsmErr as_push(struct AsmCtx *ctx, struct AsmOp *op);

/**
 * Assembles the retn instruction.
 *
 * @param ctx is the pointer to the @ref AsmCtx structure.
 * @param op is a pointer to an empty @ref AsmOp structure, that should be
 *           filled with data about the encoded instruction.
 *
 * @returns @ref AsmErr.ERR_NONE on success or any error, that occurred
 */
extern enum AsmErr as_retn(struct AsmCtx *ctx, struct AsmOp *op);

/**
 * Assembles the shl instruction.
 *
 * @param ctx is the pointer to the @ref AsmCtx structure.
 * @param op is a pointer to an empty @ref AsmOp structure, that should be
 *           filled with data about the encoded instruction.
 *
 * @returns @ref AsmErr.ERR_NONE on success or any error, that occurred
 */
extern enum AsmErr as_shl(struct AsmCtx *ctx, struct AsmOp *op);

/**
 * Assembles the shr instruction.
 *
 * @param ctx is the pointer to the @ref AsmCtx structure.
 * @param op is a pointer to an empty @ref AsmOp structure, that should be
 *           filled with data about the encoded instruction.
 *
 * @returns @ref AsmErr.ERR_NONE on success or any error, that occurred
 */
extern enum AsmErr as_shr(struct AsmCtx *ctx, struct AsmOp *op);

/**
 * Assembles the sub instruction.
 *
 * @param ctx is the pointer to the @ref AsmCtx structure.
 * @param op is a pointer to an empty @ref AsmOp structure, that should be
 *           filled with data about the encoded instruction.
 *
 * @returns @ref AsmErr.ERR_NONE on success or any error, that occurred
 */
extern enum AsmErr as_sub(struct AsmCtx *ctx, struct AsmOp *op);

/**
 * Assembles the syscall instruction.
 *
 * @param ctx is the pointer to the @ref AsmCtx structure.
 * @param op is a pointer to an empty @ref AsmOp structure, that should be
 *           filled with data about the encoded instruction.
 *
 * @returns @ref AsmErr.ERR_NONE on success or any error, that occurred
 */
extern enum AsmErr as_syscall(struct AsmCtx *ctx, struct AsmOp *op);

/**
 * Assembles the xchg instruction.
 *
 * @param ctx is the pointer to the @ref AsmCtx structure.
 * @param op is a pointer to an empty @ref AsmOp structure, that should be
 *           filled with data about the encoded instruction.
 *
 * @returns @ref AsmErr.ERR_NONE on success or any error, that occurred
 */
extern enum AsmErr as_xchg(struct AsmCtx *ctx, struct AsmOp *op);

/**
 * Assembles the xor instruction.
 *
 * @param ctx is the pointer to the @ref AsmCtx structure.
 * @param op is a pointer to an empty @ref AsmOp structure, that should be
 *           filled with data about the encoded instruction.
 *
 * @returns @ref AsmErr.ERR_NONE on success or any error, that occurred
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
 * Get the current line in the assembly text.
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
 * @param dest is a pointer to the destination buffer
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
 * @param ctx is a pointer to the @ref AsmCtx structure.
 * @param filename is the name of the source file, that will be embedded in the
 *                 strtab
 *
 * @returns the size of the `.shstrtab` section in bytes
 */
extern size_t elf64_clcstrtabsz(struct AsmCtx *ctx, char *filename);

/**
 * Calculates the size of a `.symtab` section in an ELF file.
 *
 * @param ctx is a pointer to the @ref AsmCtx structure.
 *
 * @returns the size of the `.symtab` section in bytes
 */
extern size_t elf64_clcsymtabsz(struct AsmCtx *ctx);

/**
 * Calculates the size of a `.text` section in an ELF file.
 *
 * @param ctx is a pointer to the @ref AsmCtx structure.
 *
 * @returns the size of the `.text` section in bytes
 */
extern size_t elf64_clctextsz(struct AsmCtx *ctx);

/**
 * Stores the assembled program as ELF data into a buffer.
 *
 * @param ctx is a pointer to the @ref AsmCtx structure.
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
 * @param ctx is a pointer to the @ref AsmCtx structure.
 * @param buffer is a zero allocated buffer, where the ELF data will be written
 *               to.
 * @param n is the size of the buffer in bytes
 *
 * @returns the number of bytes written to the buffer or zero on failure
 */
extern size_t elf64_dump_header(struct AsmCtx *ctx, void *buffer, size_t n);

/**
 * Fills @ref AsmOp for an immediate to ax instruction with 32 bit limit.
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
 * @param ctx is a pointer to the @ref AsmCtx structure.
 * @param op is a pointer to the @ref AsmOp structure.
 * @param op8 is the opcode for the 8-bit operation.
 */
extern void genop2aximm32(struct AsmCtx *ctx, struct AsmOp *op, uint8_t op8);

/**
 * Fills @ref AsmOp for a immediate to R/M instruction with 32 bit limit.
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
 * @param ctx is a pointer to the @ref AsmCtx structure.
 * @param op is a pointer to the @ref AsmOp structure.
 * @param op8 is the opcode for the 8-bit operation.
 * @param op_simm8 is the opcode for the operation with a sign-extended 8-bit
 *                 immediate.
 * @param modrm_reg is additional data to encode in the ModRM.reg field.
 */
extern void genop2rimm32(struct AsmCtx *ctx, struct AsmOp *op, uint8_t op8,
                         uint8_t op_simm8, uint8_t modrm_reg);

/**
 * Fills @ref AsmOp for a R source and R/M destination instruction.
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
 * @param ctx is a pointer to the @ref AsmCtx structure.
 * @param op is a pointer to the @ref AsmOp structure.
 * @param op8 is the opcode for the 8-bit operation.
 */
extern void genop2rmr(struct AsmCtx *ctx, struct AsmOp *op, uint8_t op8);

/**
 * Fills the AsmOp structure for a R/M source and R destination instruction.
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
 * @param ctx is a pointer to the @ref AsmCtx structure.
 * @param op is a pointer to the @ref AsmOp structure.
 * @param op8 is the opcode for the 8-bit operation.
 */
extern void genop2rrm(struct AsmCtx *ctx, struct AsmOp *op, uint8_t op8);

/**
 * Checks whether the given label is declared as global.
 *
 * @param ctx is a pointer to the @ref AsmCtx structure
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
 * Checks if the next character is a delimiter for operands.
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
 * @returns the signed 64 bit integer
 */
extern int64_t pint(char **assembly);

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
 * @param flags is a pointer to the location where the flags of the entry in the
 *              symbol table will be stored on success.
 *              See @ref SymTabNtr_flags for further information.
 * @param rel_target is a pointer to the location where the
 *                   @ref SymTabNtr.rel_target will be stored on success.
 *
 * @returns 0 on success or 1 in case the label is not found in the symbol table
 */
extern int rslvref(char *label, struct SymTabNtr *symtab, size_t n,
                   uint32_t *offset, enum SymTabNtr_flags *flags,
                   uint32_t *rel_target);

/**
 * Performs the second pass of the output generation and resolves all references.
 *
 * @param ctx is the pointer to the @ref AsmCtx structure.
 *
 * @returns @ref AsmErr.ERR_NONE on success or @ref AsmErr.ERR_UNKNOWN_REFERENCE
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
 * Sets @ref AsmOp.modrm_mod based on @ref AsmOp.disp.
 *
 * @param op is the pointer to the @ref AsmOp structure.
 */
extern void strdspmodrmmod(struct AsmOp *op);

/**
 * Stores the next token as global symbol.
 *
 * @param ctx is the pointer to the @ref AsmCtx structure.
 *
 * @returns the number of bytes stored or `-1` on failure.
 */
extern int strglbl(struct AsmCtx *ctx);

/**
 * Stores next token of the assembly text as label in @ref AsmCtx.label.
 *
 * If the label starts with a dot, it will be prepended with the previous label
 * that did not start with a dot.
 *
 * @param ctx is the pointer to the @ref AsmCtx structure.
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
 *              shall be resolved.
 * @param rel_target is used when @ref SymTabNtr_flags.FLAG_RELATIVE is set
 *                   in `flags`.
 *
 * @returns 0 on success or 1 in case there is no free entry in the symbol table
 */
extern int strsymtabntr(struct SymTabNtr *symtab, size_t n, char *label,
                        uint32_t offset, enum SymTabNtr_flags flags,
                        uint32_t rel_target);

/**
 * @returns the number of entries in the symbol table.
 */
extern size_t symtablen(struct SymTabNtr *symtab, size_t n);

/**
 * @returns the number of global entries in the symbol table.
 */
extern size_t symtabnglbls(struct AsmCtx *ctx);

#endif /* _0x864_H */
