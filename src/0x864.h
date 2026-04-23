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
#define ENCODING_OI 0x06
#define ENCODING_RM 0x07

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
	uint8_t __reserved;
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
 *			      table. This parameter should be larger than the
 *			      number of labels in the assembly could.
 * @param max_reftab_entries is the number of entries for the allocated reference
 *			     table. This parameter should be largern than the
 *			     number of jump and call instructions in the assembly
 *			     code, that use labels as target.
 *
 * @returns a pointer to the newly allocated AsmCtx structure or NULL on failure
 */
struct AsmCtx *make_asmctx(char const *assembly, size_t max_bintxt_size,
			   size_t max_symtab_entries, size_t max_reftab_entries);

/**
 * Cleans up a AsmCtx structure including all it's members allocated by
 * `max_asmctx`.
 *
 * @param ctx is the pointer to the AsmCtx structure.
 */
void free_asmctx(struct AsmCtx *ctx);

extern void assemble(struct AsmCtx *);

/**
 * Encodes an instruction described by the AsmOp structure.
 *
 * @param ctx is the pointer to the AsmCtx structure.
 * @param op is the pointer to the AsmOp structure.
 */
extern void assemble_op(struct AsmCtx *ctx, struct AsmOp *op);

/**
 * Assembles a single instruction.
 *
 * @param ctx is the pointer to the AsmCtx structure.
 */
extern void as_snglinst(struct AsmCtx *ctx);

/**
 * Assembles the call instruction.
 *
 * @param ctx is the pointer to the AsmCtx structure.
 * @param op is a pointer to an empty AsmOp structure, that should be filled
 *	     with data about the encoded instruction.
 */
extern void as_call(struct AsmCtx *ctx, struct AsmOp *op);

/**
 * Assembles the nop instruction.
 *
 * @param ctx is the pointer to the AsmCtx structure.
 * @param op is a pointer to an empty AsmOp structure, that should be filled
 *	     with data about the encoded instruction.
 */
extern void as_nop(struct AsmCtx *ctx, struct AsmOp *op);

/**
 * Assembles the retn instruction.
 *
 * @param ctx is the pointer to the AsmCtx structure.
 * @param op is a pointer to an empty AsmOp structure, that should be filled
 *	     with data about the encoded instruction.
 */
extern void as_retn(struct AsmCtx *ctx, struct AsmOp *op);

extern int cklb(char const *assembly);

/**
 * Get the size of an operation from it's operants.
 *
 * @param assembly is a pointer to the assembly string.
 * @param n_ops is the number of operants
 *
 * @returns the size of the operation in bits (8, 16, 32 or 64)
 */
extern uint8_t ckopsize(char const *assembly, uint8_t n_ops);

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
 *	    space, comma or semicolon
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
 *		   This pointer gets advanced to the character following the
 *		   integer.
 *
 * @returns the unsigned 32 bit integer
 */
extern uint32_t pint(char **assembly);

/**
 * Parses the next token as 8-bit register.
 *
 * @param assembly is a pointer to the string with the assembly code.
 *		   This pointer gets advanced to the character following the
 *		   register.
 *
 * @returns the 4-bit encoded register or `0xff` on failure.
 */
extern int pr8(char **assembly);

/**
 * Parses the next token as 16-bit register.
 *
 * @param assembly is a pointer to the string with the assembly code.
 *		   This pointer gets advanced to the character following the
 *		   register.
 *
 * @returns the 4-bit encoded register or `0xff` on failure.
 */
extern int pr16(char **assembly);

/**
 * Parses the next token as 32-bit register.
 *
 * @param assembly is a pointer to the string with the assembly code.
 *		   This pointer gets advanced to the character following the
 *		   register.
 *
 * @returns the 4-bit encoded register or `0xff` on failure.
 */
extern int pr32(char **assembly);

/**
 * Parses the next token as 64-bit register.
 *
 * @param assembly is a pointer to the string with the assembly code.
 *		   This pointer gets advanced to the character following the
 *		   register.
 *
 * @returns the 4-bit encoded register or `0xff` on failure.
 */
extern int pr64(char **assembly);

/**
 * Parses a register indirect addressing.
 *
 * @param assembly is a pointer to the string with the assembly code.
 *		   This pointer gets advanced to the character following the
 *		   closing square bracket.
 * @param reg
 * @param disp
 */
extern void prgndrct(char **assembly, uint8_t *reg, uint32_t *disp);

/**
 * Parses the next token as register.
 *
 * @param assembly is a pointer to the string with the assembly code.
 *		   This pointer gets advanced to the character following the
 *		   register.
 *
 * @returns the 4 bit encoded register or `0xff` on failure.
 */
extern uint8_t preg(char **assembly);

/**
 * Parses next token as a label and writes it to the `label` parameter.
 * Writes at most `n` bytes (including the null-terminator after the label).
 *
 * @param assembly is a pointer to the string with the assembly code.
 *		   This pointer gets advanced to the character following the
 *		   colon after the label.
 * @param label is a pointer to the string where the label will be written into.
 *		Should be at least `n` bytes in size.
 * @param n is the maximum number of bytes to write into `label`.
 *
 * @returns the number of bytes written or -1 if the label does not fit into `n`
 *	    bytes.
 */
extern int readnlbl(char const **assembly, char *label, size_t n);

/**
 * Loads the address of the given label from the symbol table.
 *
 * @param label is the label to search the offset for
 * @param symtab is a pointer to the symbol table
 * @param n is the number of entries in the symbol table
 * @param offset is a pointer to the location where the offset of the label will
 *		 be stored on success.
 *
 * @returns 0 on success or 1 in case the label is not found in the symbol table
 */
extern int rslvref(char *label, struct SymTabNtr *symtab, size_t n,
		   uint32_t *offset, uint32_t *flags, uint32_t *rel_target);

/**
 * Performs the second pass of the output generation and resolves all references.
 *
 * @param ctx is the pointer to the AsmCtx structure.
 */
extern void scndpss(struct AsmCtx *ctx);

extern void skp2lbinst(char const **assembly);

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
 *		references and holds additional information how the references
 *		shall be resolved. Available flags:
 *		- 0x01 RESOLVE_RELATIVE
 * @param rel_target is used when the RESOLVE_RELATIVE bit is set in the flags
 *
 * @returns 0 on success or 1 in case there is no free entry in the symbol table
 */
extern int strsymtabntr(struct SymTabNtr *symtab, size_t n, char *label,
			uint32_t offset, uint32_t flags, uint32_t rel_target);

#endif /* _0x864_H */
