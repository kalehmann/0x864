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

struct SymTabNtr {
	char label[252];
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
 * Assembles a single instruction.
 * @param assembly is a pointer to the string with the assembly code.
 *		   This pointer gets advanced to the character following the
 *		   last operand of the instruction.
 * @param output is a pointer to the buffer where the binary output will be
 *		 written.
 * @param n is the maximum number of bytes to write into the output buffer.
 * @param o is a pointer to the location in which the actual number of bytes
 *	    written into the output buffer will be stored.
 */
extern void as_snglinst(struct AsmCtx *);

/**
 * @param assembly is a pointer to the string with the assembly code.
 *		   As the `nop` instruction takes no operands, this pointer will
 *		   not be advanced.
 * @param output is a pointer to the buffer where the binary output will be
 *		 written.
 * @param n is the maximum number of bytes to write into the output buffer.
 * @param o is a pointer to the location in which the actual number of bytes
 *	    written into the output buffer will be stored.
 */
extern void as_nop(struct AsmCtx *);

/**
 * @param assembly is a pointer to the string with the assembly code.
 *		   As the `retn` instruction takes no operands, this pointer will
 *		   not be advanced.
 * @param output is a pointer to the buffer where the binary output will be
 *		 written.
 * @param n is the maximum number of bytes to write into the output buffer.
 * @param o is a pointer to the location in which the actual number of bytes
 *	    written into the output buffer will be stored.
 */
extern void as_retn(struct AsmCtx *);

extern int cklb(char const *assembly);

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
extern int rslvref(char *label, uint8_t (*symtab)[256], size_t n,
		   uint32_t *offset);

extern void skp2lbinst(char const **assembly);

/**
 * Stores a symbol in the next free entry of the symbol table.
 *
 * @param symtab is a pointer to the symbol table
 * @param n is the number of entries in the symbol table
 * @param label is the label of the new entry to insert
 * @param offset is the offset to store for the label
 *
 * @returns 0 on success or 1 in case there is no free entry in the symbol table
 */
extern int strsymtabntr(uint8_t (*symtab)[256], size_t n, char *label,
			uint32_t offset);

#endif /* _0x864_H */
