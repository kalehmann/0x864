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

#include <stddef.h>

extern void assemble(char const *assembly, void *output, size_t n, size_t *o);

/**
 * Assembles a single instruction.
 * @param assembly is a pointer to the string with the assembly code.
 *                 This pointer gets advanced to the character following the
 *                 last operand of the instruction.
 * @param output is a pointer to the buffer where the binary output will be
 *               written.
 * @param n is the maximum number of bytes to write into the output buffer.
 * @param o is a pointer to the location in which the actual number of bytes
 *          written into the output buffer will be stored.
 */
extern void as_snglinst(char const **assembly, void *output, size_t n, size_t *o);

/**
 * @param assembly is a pointer to the string with the assembly code.
 *                 As the `nop` instruction takes no operands, this pointer will
 *                 not be advanced.
 * @param output is a pointer to the buffer where the binary output will be
 *               written.
 * @param n is the maximum number of bytes to write into the output buffer.
 * @param o is a pointer to the location in which the actual number of bytes
 *          written into the output buffer will be stored.
 */
extern void as_nop(char const **assembly, void *output, size_t n, size_t *o);

/**
 * @param assembly is a pointer to the string with the assembly code.
 *                 As the `retn` instruction takes no operands, this pointer will
 *                 not be advanced.
 * @param output is a pointer to the buffer where the binary output will be
 *               written.
 * @param n is the maximum number of bytes to write into the output buffer.
 * @param o is a pointer to the location in which the actual number of bytes
 *          written into the output buffer will be stored.
 */
extern void as_retn(char const **assembly, void *output, size_t n, size_t *o);

extern int cklb(char const *assembly);

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
extern int rslvref(char *label, void *symtab, size_t n, unsigned int *offset);

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
extern int strsymtabntr(void *symtab, size_t n, char *label, unsigned int offset);
