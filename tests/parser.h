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

#ifndef PARSER_H
#define PARSER_H

void test_struct_AsmCtx_is_packed(void);
void test_struct_SymTabNtr_is_packed(void);
void test_assemble(void);
void test_as_call(void);
void test_as_nop(void);
void test_as_retn(void);
void test_cklb(void);
void test_isint(void);
void test_isopdlm(void);
void test_isreg(void);
void test_isrgndrct(void);
void test_pint(void);
void test_pr8(void);
void test_pr16(void);
void test_pr32(void);
void test_pr64(void);
void test_preg(void);
void test_readnlbl(void);
void test_rslvref(void);
void test_scndpss(void);
void test_skp2lbinst(void);
void test_strlbl(void);
void test_strsymtabntr(void);
void test_symbol_table_generation(void);

#endif /* PARSER_H */
