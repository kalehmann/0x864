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

#include <acutest.h>
#include <stddef.h>
#include "assembler.h"
#include "elf64.h"
#include "parser.h"
#include "utils.h"

TEST_LIST = {
        { "struct_AsmCtx_is_packed", test_struct_AsmCtx_is_packed },
        { "struct_AsmOp_is_packed", test_struct_AsmOp_is_packed },
        { "struct_SymTabNtr_is_packed", test_struct_SymTabNtr_is_packed },
        { "algn16", test_algn16 },
        { "assemble", test_assemble },
        { "assemble_op", test_assemble_op },
        { "as_call", test_as_call },
        { "as_int", test_as_int },
        { "as_nop", test_as_nop },
        { "as_retn", test_as_retn },
        { "cklb", test_cklb },
        { "ckln", test_ckln },
        { "ckopsize", test_ckopsize },
        { "ckoptps", test_ckoptps },
        { "clr", test_clr },
        { "cpy", test_cpy },
        { "elf64_clcshstrtabsz", test_elf64_clcshstrtabsz },
        { "elf64_clcstrtabsz", test_elf64_clcstrtabsz },
        { "elf64_clcsymtabsz", test_elf64_clcsymtabsz },
        { "elf64_clctextsz", test_elf64_clctextsz },
        { "elf64_dump_header", test_elf64_dump_header },
        { "isglbl", test_isglbl },
        { "isint", test_isint },
        { "isopdlm", test_isopdlm },
        { "isreg", test_isreg },
        { "isrgndrct", test_isrgndrct },
        { "len", test_len },
        { "pint", test_pint },
        { "pr8", test_pr8 },
        { "pr16", test_pr16 },
        { "pr32", test_pr32 },
        { "pr64", test_pr64 },
        { "preg", test_preg },
        { "prgndrct", test_prgndrct },
        { "readnlbl", test_readnlbl },
        { "rslvref", test_rslvref },
        { "scndpss", test_scndpss },
        { "skp2lbinst", test_skp2lbinst },
        { "skp2nxtop", test_skp2nxtop },
        { "storing_globals", test_storing_globals },
        { "strdspmodrmmod", test_strdspmodrmmod },
        { "strglbl", test_strglbl },
        { "strlbl", test_strlbl },
        { "strsymtabntr", test_strsymtabntr },
        { "symbol_table_generation", test_symbol_table_generation },
        { "symtablen", test_symtablen },
        { "symtabnglbls", test_symtabnglbls },
        { NULL, NULL }
};
