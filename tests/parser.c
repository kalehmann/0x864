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
#include <string.h>
#include "0x864.h"
#include "parser.h"

void test_struct_AsmCtx_is_packed(void)
{
        // The assembly code makes assumptions about the offsets in the
        // structure. Verify, that the struct is packed.
        TEST_CHECK(sizeof(struct AsmCtx) == 64);
}

void test_assemble(void)
{
        // Test that calling `assemble` with an output size of 0 writes nothing
        // to the output buffer.
        struct AsmCtx *ctx = make_asmctx("\tnop\n", 0, 0, 0);
        TEST_ASSERT(ctx != NULL);
        assemble(ctx);
        TEST_CHECK(ctx->bintxt_size == 0);
        free_asmctx(ctx);

        // Test that the `assemble` function accepts an empty input.
        ctx = make_asmctx("", 0, 0, 0);
        TEST_ASSERT(ctx != NULL);
        assemble(ctx);
        TEST_CHECK(ctx->bintxt_size == 0);
}

void test_as_nop(void)
{
        char const *assembly = "\n"
                "        push rbp";
        char const *assembly2 = assembly;

        // Test that nothing happens if 0 bytes should be written into the output
        struct AsmCtx *ctx = make_asmctx(assembly, 0, 0, 0);
        TEST_ASSERT(ctx != NULL);
        as_nop(ctx);
        TEST_CHECK(assembly == assembly2);
        TEST_CHECK(ctx->bintxt_size == 0);
        free_asmctx(ctx);

        // Test assembling `nop` to 0x90
        ctx = make_asmctx(assembly, 16, 0, 0);
        TEST_ASSERT(ctx != NULL);
        as_nop(ctx);
        TEST_CHECK(assembly == assembly2);
        TEST_CHECK(ctx->bintxt_size == 1);
        TEST_CHECK(ctx->bintxt[0] == 0x90);
        free_asmctx(ctx);
}

void test_as_retn(void)
{
        char const *assembly = "\n"
                "        push rbp";
        char const *assembly2 = assembly;

        // Test that nothing happens if 0 bytes should be written into the output
        struct AsmCtx *ctx = make_asmctx(assembly, 0, 0, 0);
        TEST_ASSERT(ctx != NULL);
        as_retn(ctx);
        TEST_CHECK(assembly == assembly2);
        TEST_CHECK(ctx->bintxt_size == 0);
        free_asmctx(ctx);

        // Test assembling `retn` to 0xc3
        ctx = make_asmctx(assembly, 16, 0, 0);
        TEST_ASSERT(ctx != NULL);
        as_retn(ctx);
        TEST_CHECK(assembly == assembly2);
        TEST_CHECK(ctx->bintxt_size == 1);
        TEST_CHECK(ctx->bintxt[0] == 0xc3);
        as_retn(ctx);
        TEST_CHECK(ctx->bintxt_size == 2);
        TEST_CHECK(ctx->bintxt[1] == 0xc3);
        TEST_CHECK(ctx->bintxt[2] == 0);
        free_asmctx(ctx);
}


void test_cklb(void)
{
        TEST_CHECK(cklb("") == 0);
        TEST_CHECK(cklb("\n") == 0);
        TEST_CHECK(cklb(";;; Comment\n") == 0);
        TEST_CHECK(cklb("        ;; Comment\n") == 0);
        TEST_CHECK(cklb(";;; test:\n") == 0);
        TEST_CHECK(cklb("test: \n") == 1);
}

void test_readnlbl(void)
{
        int ret = 0;
        char label[256] = { 0 };
        char const *assembly = "label:\n"
                "        push rbp";
        char const *assembly_bak = assembly;
        const char * const token = strstr(assembly, "\n");

        // Test that the pointer to the assembly is still advanced after the
        // label, but no output is written, if `n` is zero.
        ret = readnlbl(&assembly, label, 0);
        TEST_CHECK(ret == -1);
        TEST_CHECK(assembly == token);
        TEST_CHECK(strncmp(label, "\0", 1) == 0);

        assembly = assembly_bak;
        memset(label, 0, 256);
        // Test that the pointer to the assembly is still advanced after the
        // label and some output is written, if `n` is shorter than the label.
        ret = readnlbl(&assembly, label, 3);
        TEST_CHECK(ret == -1);
        TEST_CHECK(assembly == token);
        TEST_CHECK(strncmp(label, "lab", 3) == 0);

        assembly = assembly_bak;
        memset(label, 0, 256);
        // Test that the pointer to the assembly is advanced after the label
        // and the label name is saved, if `n` is exceeds the labels size.
        ret = readnlbl(&assembly, label, 256);
        TEST_CHECK(ret == 6);
        TEST_CHECK(assembly == token);
        TEST_CHECK(strncmp(label, "label", 6) == 0);
}

void test_rslvref(void)
{
        uint32_t offset1 = 0xDEADC0DE;
        uint32_t offset2 = 0xCAFEBABE;
        uint32_t offset3 = 0x0DEFACED;
        uint32_t offset = 0;

        void *symtab = calloc(4, 256);
        TEST_ASSERT(symtab != NULL);

        strcpy(symtab, "label1");
        memcpy(symtab + 251, &offset1, 4);
        strcpy(symtab + 256, "label2");
        memcpy(symtab + 507, &offset2, 4);
        strcpy(symtab + 512, "label3");
        memcpy(symtab + 763, &offset3, 4);

        // Test resolving an non existing reference gives an error
        TEST_CHECK(rslvref("label", symtab, 4, NULL) == 1);
        // Test resolving labels
        TEST_CHECK(rslvref("label1", symtab, 4, &offset) == 0);
        TEST_CHECK(offset = offset1);
        TEST_CHECK(rslvref("label2", symtab, 4, &offset) == 0);
        TEST_CHECK(offset = offset2);
        TEST_CHECK(rslvref("label3", symtab, 4, &offset) == 0);
        TEST_CHECK(offset = offset3);

        free(symtab);
}

void test_skp2lbinst(void)
{
        char const *assembly1 = ";;; Comment\n"
                "\n"
                "label:\n"
                "      ret";
        const char * const label1 = strstr(assembly1, "label:");
        char const *assembly2 = ".local_label:\n"
                "      ret";
        const char * const label2 = assembly2;

        skp2lbinst(&assembly1);
        TEST_CHECK(assembly1 == label1);

        skp2lbinst(&assembly2);
        TEST_CHECK(assembly2 == label2);
}

void test_strsymtabntr(void)
{
        uint32_t offset1 = 1234;
        uint32_t offset2 = 0xCAFEBABE;
        void *symtab = calloc(2, 256);

        TEST_ASSERT(symtab != NULL);

        // Test storing entry in empty symbol table
        TEST_CHECK(strsymtabntr(symtab, 2, "test", offset1) == 0);
        TEST_CHECK(strncmp(symtab, "test", 5) == 0);
        TEST_CHECK(memcmp(symtab + 251, &offset1, 4) == 0);
        TEST_CHECK(memcmp(symtab + 256, "\0\0\0\0\0", 5) == 0);
        TEST_CHECK(memcmp(symtab + 507, "\0\0\0\0", 4) == 0);

        // Test storing second entry in symbol table
        TEST_CHECK(strsymtabntr(symtab, 2, "label2", offset2) == 0);
        TEST_CHECK(strncmp(symtab, "test", 5) == 0);
        TEST_CHECK(memcmp(symtab + 251, &offset1, 4) == 0);
        TEST_CHECK(memcmp(symtab + 256, "label2", 7) == 0);
        TEST_CHECK(memcmp(symtab + 507, &offset2, 4) == 0);

        // Storing a third entry in a symbol table of size two returns an error
        // and leaves the first two entries unchanged.
        TEST_CHECK(strsymtabntr(symtab, 2, "foobar", 0xDEADC0DE) == 1);
        TEST_CHECK(strncmp(symtab, "test", 5) == 0);
        TEST_CHECK(memcmp(symtab + 251, &offset1, 4) == 0);
        TEST_CHECK(memcmp(symtab + 256, "label2", 7) == 0);
        TEST_CHECK(memcmp(symtab + 507, &offset2, 4) == 0);

        free(symtab);
}
