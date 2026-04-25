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
#include <elf.h>
#include <string.h>
#include "0x864.h"
#include "elf64.h"

void test_elf64_clcshstrtabsz(void)
{
        // 33 bytes for strlen("\0.text\0.shstrtab\0.symtab\0.strtab\0")
        TEST_CHECK(elf64_clcshstrtabsz() == 33);
}

void test_elf64_clcstrtabsz(void)
{
        struct AsmCtx *ctx = make_asmctx(NULL, 0, 16, 0, 0);
        TEST_ASSERT(ctx != NULL);

        TEST_CHECK(strsymtabntr(ctx->symtab, 16, "demo_01_call", 0, 0, 0) == 0);
        TEST_CHECK(strsymtabntr(ctx->symtab, 16, "foo", 0, 0, 0) == 0);
        TEST_CHECK(strsymtabntr(ctx->symtab, 16, "bar", 0, 0, 0) == 0);

        TEST_CHECK(elf64_clcstrtabsz(ctx, "tests/demos/01-call.s") == 0x2c);

        free_asmctx(ctx);
}

void test_elf64_clcsymtabsz(void)
{
        struct AsmCtx *ctx = make_asmctx(NULL, 0, 16, 0, 0);
        TEST_ASSERT(ctx != NULL);

        TEST_CHECK(strsymtabntr(ctx->symtab, 16, "demo_01_call", 0, 0, 0) == 0);
        TEST_CHECK(strsymtabntr(ctx->symtab, 16, "foo", 0, 0, 0) == 0);
        TEST_CHECK(strsymtabntr(ctx->symtab, 16, "bar", 0, 0, 0) == 0);

        TEST_CHECK(elf64_clcsymtabsz(ctx) == 0x90);

        free_asmctx(ctx);
}

void test_elf64_clctextsz(void)
{
        struct AsmCtx *ctx = make_asmctx(NULL, 16, 0, 0, 0);
        TEST_ASSERT(ctx != NULL);
        ctx->bintxt_size = 0x0e;

        TEST_CHECK(elf64_clctextsz(ctx) == 0x0e);

        free_asmctx(ctx);
}

void test_elf64_dump_header(void)
{
        void *buffer = NULL;
        struct AsmCtx *ctx = make_asmctx(NULL, 0, 0, 0, 0);
        TEST_ASSERT(ctx != NULL);

        TEST_CHECK(elf64_dump_header(ctx, NULL, 0) == 0);

        buffer = calloc(1, 0x80);
        TEST_ASSERT(buffer != NULL);

        TEST_CHECK(elf64_dump_header(ctx, buffer, 0x80) == 0x40);
        Elf64_Ehdr *header = buffer;

        TEST_CHECK(header->e_ident[EI_MAG0] == ELFMAG0);
        TEST_CHECK(header->e_ident[EI_MAG1] == ELFMAG1);
        TEST_CHECK(header->e_ident[EI_MAG2] == ELFMAG2);
        TEST_CHECK(header->e_ident[EI_MAG3] == ELFMAG3);
        TEST_CHECK(header->e_ident[EI_CLASS] == ELFCLASS64);
        TEST_CHECK(header->e_ident[EI_DATA] == ELFDATA2LSB);
        TEST_CHECK(header->e_ident[EI_VERSION] == EV_CURRENT);
        TEST_CHECK(header->e_ident[EI_OSABI] == ELFOSABI_NONE);
        TEST_CHECK(header->e_ident[EI_ABIVERSION] == 0);

        TEST_CHECK(header->e_type == ET_REL);
        TEST_CHECK(header->e_machine == EM_X86_64);
        TEST_CHECK(header->e_version == EV_CURRENT);
        TEST_CHECK(header->e_phoff == 0);
        TEST_CHECK(header->e_shoff == sizeof(Elf64_Ehdr));
        TEST_CHECK(header->e_ehsize == sizeof(Elf64_Ehdr));
        TEST_CHECK(header->e_phentsize == 0);
        TEST_CHECK(header->e_phnum == 0);
        TEST_CHECK(header->e_shentsize == sizeof(Elf64_Shdr));
        // Null-section, .text, .shstrtab, .symtab, .strtab
        TEST_CHECK(header->e_shnum == 5);
        // .shstrtab is the third section with index 2
        TEST_CHECK(header->e_shstrndx == 2);

        free_asmctx(ctx);
}

void test_elf64_dump_symtab(void)
{
}
