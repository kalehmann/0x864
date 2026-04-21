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
	TEST_CHECK(sizeof(struct AsmCtx) == 544);
}

void test_struct_SymTabNtr_is_packed(void)
{
	// The assembly code makes assumptions about the offsets in the
	// structure. Verify, that the struct is packed.
	TEST_CHECK(sizeof(struct SymTabNtr) == 256);
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

void test_as_call(void)
{
	char const *assembly = " foobar ; Comment here\n";
	char const *assembly2 = assembly + 8;

	// Test that nothing happens if 0 bytes should be written into the output
	// and the reftab has a size of zero.
	struct AsmCtx *ctx = make_asmctx(assembly, 0, 0, 0);
	TEST_ASSERT(ctx != NULL);
	as_call(ctx);
	TEST_CHECK(ctx->assembly == assembly2);
	TEST_CHECK(ctx->bintxt_size == 0);
	free_asmctx(ctx);

	// Test a near relative call (0xE8) is written to the output if the size
	// allows it and a reference to the target label stored in the reference
	// table.
	ctx = make_asmctx(assembly, 16, 0, 1);
	TEST_ASSERT(ctx != NULL);
	as_call(ctx);
	TEST_CHECK(ctx->assembly == assembly2);
	TEST_CHECK(ctx->bintxt_size == 5);
	TEST_CHECK(ctx->bintxt[0] == 0xe8);
	TEST_CHECK(ctx->bintxt[1] == 0);
	TEST_CHECK(ctx->bintxt[2] == 0);
	TEST_CHECK(ctx->bintxt[3] == 0);
	TEST_CHECK(ctx->bintxt[4] == 0);
	TEST_CHECK(strncmp(ctx->reftab[0].label, "foobar", 7) == 0);
	// Place resolved reference after first 0xe8 byte
	TEST_CHECK(ctx->reftab[0].offset == 1);
	// Resolve reference relative to `rel_target`
	TEST_CHECK(ctx->reftab[0].flags & FLAG_RELATIVE);
	TEST_CHECK(ctx->reftab[0].rel_target == 5);
	free_asmctx(ctx);
}

void test_as_nop(void)
{
	char const *assembly = "\n"
		"	 push rbp";

	// Test that nothing happens if 0 bytes should be written into the output
	struct AsmCtx *ctx = make_asmctx(assembly, 0, 0, 0);
	TEST_ASSERT(ctx != NULL);
	as_nop(ctx);
	TEST_CHECK(ctx->assembly == assembly);
	TEST_CHECK(ctx->bintxt_size == 0);
	free_asmctx(ctx);

	// Test assembling `nop` to 0x90
	ctx = make_asmctx(assembly, 16, 0, 0);
	TEST_ASSERT(ctx != NULL);
	as_nop(ctx);
	TEST_CHECK(assembly == assembly);
	TEST_CHECK(ctx->bintxt_size == 1);
	TEST_CHECK(ctx->bintxt[0] == 0x90);
	free_asmctx(ctx);
}

void test_as_retn(void)
{
	char const *assembly = "\n"
		"	 push rbp";

	// Test that nothing happens if 0 bytes should be written into the output
	struct AsmCtx *ctx = make_asmctx(assembly, 0, 0, 0);
	TEST_ASSERT(ctx != NULL);
	as_retn(ctx);
	TEST_CHECK(ctx->assembly == assembly);
	TEST_CHECK(ctx->bintxt_size == 0);
	free_asmctx(ctx);

	// Test assembling `retn` to 0xc3
	ctx = make_asmctx(assembly, 16, 0, 0);
	TEST_ASSERT(ctx != NULL);
	as_retn(ctx);
	TEST_CHECK(ctx->assembly == assembly);
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
	TEST_CHECK(cklb("	 ;; Comment\n") == 0);
	TEST_CHECK(cklb(";;; test:\n") == 0);
	TEST_CHECK(cklb("test: \n") == 1);
}

void test_isint(void)
{
	TEST_CHECK(isint("") == 0);
	TEST_CHECK(isint("1") == 1);
	TEST_CHECK(isint("321, ") == 1);
	TEST_CHECK(isint("3a1, ") == 0);
	TEST_CHECK(isint("9323 ; <- this is a decimal integer ") == 1);
	TEST_CHECK(isint("0x, ") == 0);
	TEST_CHECK(isint("0xff, ") == 1);
	TEST_CHECK(isint("0x1a, ") == 1);
	TEST_CHECK(isint("0x1g, ") == 0);
	TEST_CHECK(isint("0xdeadc0de\n") == 1);
}

void test_isopdlm(void)
{
	TEST_CHECK(isopdlm("; Comment here") == 1);
	TEST_CHECK(isopdlm("0x01") == 0);
	TEST_CHECK(isopdlm("") == 1);
	TEST_CHECK(isopdlm("\n") == 1);
	TEST_CHECK(isopdlm("\t") == 1);
	TEST_CHECK(isopdlm(",") == 1);
	TEST_CHECK(isopdlm(" ,") == 1);
}

void test_isr8(void)
{
	TEST_CHECK(isr8("al") == 1);
	TEST_CHECK(isr8("ah") == 1);
	TEST_CHECK(isr8("bl") == 1);
	TEST_CHECK(isr8("bh") == 1);
	TEST_CHECK(isr8("cl; Comment here") == 1);
	TEST_CHECK(isr8("ch  test") == 1);
	TEST_CHECK(isr8("dl\n") == 1);
	TEST_CHECK(isr8("dh\t") == 1);
	TEST_CHECK(isr8("ax\t") == 0);
	TEST_CHECK(isr8("rax") == 0);
	TEST_CHECK(isr8("ecx") == 0);
	TEST_CHECK(isr8("alt") == 0);
}

void test_isr16(void)
{
	TEST_CHECK(isr16("ax") == 1);
	TEST_CHECK(isr16("bx") == 1);
	TEST_CHECK(isr16("cx; Comment here") == 1);
	TEST_CHECK(isr16("dx  test") == 1);
	TEST_CHECK(isr16("rax") == 0);
	TEST_CHECK(isr16("ecx") == 0);
	TEST_CHECK(isr16("ex") == 0);
}

void test_isr32(void)
{
	TEST_CHECK(isr32("eax") == 1);
	TEST_CHECK(isr32("ebx") == 1);
	TEST_CHECK(isr32("ecx; Comment here") == 1);
	TEST_CHECK(isr32("edx  test") == 1);
	TEST_CHECK(isr32("edi") == 1);
	TEST_CHECK(isr32("esi") == 1);
	TEST_CHECK(isr32("r8d") == 1);
	TEST_CHECK(isr32("r9d") == 1);
	TEST_CHECK(isr32("r10d") == 1);
	TEST_CHECK(isr32("r12d") == 1);
	TEST_CHECK(isr32("r13d") == 1);
	TEST_CHECK(isr32("r14d") == 1);
	TEST_CHECK(isr32("r15d") == 1);
	TEST_CHECK(isr32("r8") == 0);
	TEST_CHECK(isr32("r9") == 0);
	TEST_CHECK(isr32("r10") == 0);
	TEST_CHECK(isr32("r12") == 0);
	TEST_CHECK(isr32("r13") == 0);
	TEST_CHECK(isr32("r14") == 0);
	TEST_CHECK(isr32("r15") == 0);
}

void test_isr64(void)
{
	TEST_CHECK(isr64("rax") == 1);
	TEST_CHECK(isr64("rbx") == 1);
	TEST_CHECK(isr64("rcx; Comment here") == 1);
	TEST_CHECK(isr64("rdx  test") == 1);
	TEST_CHECK(isr64("rsp") == 1);
	TEST_CHECK(isr64("rbp") == 1);
	TEST_CHECK(isr64("rdi") == 1);
	TEST_CHECK(isr64("rsi") == 1);
	TEST_CHECK(isr64("r8") == 1);
	TEST_CHECK(isr64("r9") == 1);
	TEST_CHECK(isr64("r10") == 1);
	TEST_CHECK(isr64("r12") == 1);
	TEST_CHECK(isr64("r13") == 1);
	TEST_CHECK(isr64("r14") == 1);
	TEST_CHECK(isr64("r15\t") == 1);
}

void test_isreg(void)
{
	TEST_CHECK(isreg("al") == 1);
	TEST_CHECK(isreg("ah") == 1);
	TEST_CHECK(isreg("ax,") == 1);
	TEST_CHECK(isreg("axb") == 0);
	TEST_CHECK(isreg("bl ") == 1);
	TEST_CHECK(isreg("ch\t") == 1);
	TEST_CHECK(isreg("esi") == 1);
	TEST_CHECK(isreg("esx") == 0);
	TEST_CHECK(isreg("es, ") == 0);
	TEST_CHECK(isreg("edx;Comment here\n") == 1);
	TEST_CHECK(isreg("r8\t\t\n") == 1);
	TEST_CHECK(isreg("r9 ") == 1);
	TEST_CHECK(isreg("r10 ") == 1);
	TEST_CHECK(isreg("r11 ") == 1);
	TEST_CHECK(isreg("r12 ") == 1);
	TEST_CHECK(isreg("r13 ") == 1);
	TEST_CHECK(isreg("r14 ") == 1);
	TEST_CHECK(isreg("r15 ") == 1);
	TEST_CHECK(isreg("r16 ") == 0);
	TEST_CHECK(isreg("r") == 0);
	TEST_CHECK(isreg("r7") == 0);
}

void test_readnlbl(void)
{
	int ret = 0;
	char label[256] = { 0 };
	char const *assembly = "label:\n"
		"	 push rbp";
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
	uint32_t flags = 0;
	uint32_t offset = 0;
	uint32_t rel_target = 0;

	struct SymTabNtr *symtab = calloc(4, sizeof(struct SymTabNtr));
	TEST_ASSERT(symtab != NULL);

	strcpy(symtab[0].label, "label1");
	symtab[0].flags = FLAG_RELATIVE;
	symtab[0].rel_target = 0xAABBCCDD;
	symtab[0].offset = offset1;
	strcpy(symtab[1].label, "label2");
	symtab[1].offset = offset2;
	strcpy(symtab[2].label, "label3");
	symtab[2].offset = offset3;

	// Test resolving an non existing reference gives an error
	TEST_CHECK(rslvref("label", symtab, 4, NULL, NULL, NULL) == 1);
	// Test resolving labels
	TEST_CHECK(rslvref("label1", symtab, 4, &offset, &flags, &rel_target) == 0);
	TEST_CHECK(flags == FLAG_RELATIVE);
	TEST_CHECK(offset == offset1);
	TEST_CHECK(rel_target == 0xAABBCCDD);
	TEST_CHECK(rslvref("label2", symtab, 4, &offset, NULL, NULL) == 0);
	TEST_CHECK(offset == offset2);
	TEST_CHECK(rslvref("label3", symtab, 4, &offset, NULL, NULL) == 0);
	TEST_CHECK(offset == offset3);

	free(symtab);
}

void test_scndpss(void)
{
	int *target = NULL;
	struct AsmCtx *ctx = make_asmctx(NULL, 32, 8, 8);
	TEST_ASSERT(ctx != NULL);

	ctx->bintxt_size = 32;
	// Fill symbol table with dummy entries
	strncpy(ctx->symtab[0].label, "test", 5);
	ctx->symtab[0].offset = 20;
	strncpy(ctx->symtab[1].label, "foobar", 7);
	ctx->symtab[1].offset = 13;

	// Fill reference table with dummy entries
	strncpy(ctx->reftab[0].label, "test", 5);
	ctx->reftab[0].offset = 4;
	ctx->reftab[0].flags = 0;
	ctx->reftab[0].rel_target = 0;
	strncpy(ctx->reftab[1].label, "foobar", 7);
	ctx->reftab[1].offset = 16;
	ctx->reftab[1].flags = FLAG_RELATIVE;
	ctx->reftab[1].rel_target = 20;

	// Act
	scndpss(ctx);

	// Test can replace an absolute reference
	target = ((void *)ctx->bintxt) + 4;
	TEST_CHECK(*target == 20);

	// Test can replace a relative reference
	target = ((void *)ctx->bintxt) + 16;
	TEST_CHECK(*target == -7);

	free_asmctx(ctx);
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

void test_strlbl(void)
{
	const char *assembly = "label1:\n"
		".sublabel1:\n"
		".sublabel2:\n"
		".end:\n"
		"label2:\n"
		"end:\n";

	struct AsmCtx *ctx = make_asmctx(assembly, 0, 0, 0);
	TEST_ASSERT(ctx != NULL);

	strlbl(ctx);
	TEST_CHECK(strncmp(ctx->label, "label1", 7) == 0);

	skp2lbinst(&ctx->assembly);
	strlbl(ctx);
	TEST_CHECK(strncmp(ctx->label, "label1.sublabel1", 17) == 0);

	skp2lbinst(&ctx->assembly);
	strlbl(ctx);
	TEST_CHECK(strncmp(ctx->label, "label1.sublabel2", 17) == 0);

	skp2lbinst(&ctx->assembly);
	strlbl(ctx);
	TEST_CHECK(strncmp(ctx->label, "label1.end", 11) == 0);

	skp2lbinst(&ctx->assembly);
	strlbl(ctx);
	TEST_CHECK(strncmp(ctx->label, "label2", 7) == 0);

	skp2lbinst(&ctx->assembly);
	strlbl(ctx);
	TEST_CHECK(strncmp(ctx->label, "end", 4) == 0);

	free_asmctx(ctx);
}

void test_strsymtabntr(void)
{
	struct SymTabNtr *symtab = calloc(3, sizeof(struct SymTabNtr));

	TEST_ASSERT(symtab != NULL);

	// Test storing entry in empty symbol table
	TEST_CHECK(strsymtabntr(symtab, 2, "test", 1234, 0, 0) == 0);
	TEST_CHECK(strncmp(symtab[0].label, "test", 5) == 0);
	TEST_CHECK(symtab[0].offset == 1234);
	TEST_CHECK(memcmp(symtab[1].label, "\0\0\0\0\0", 5) == 0);
	TEST_CHECK(memcmp(&symtab[1].offset, "\0\0\0\0", 4) == 0);

	// Test storing second entry in symbol table
	TEST_CHECK(strsymtabntr(symtab, 2, "label2", 0xCAFEBABE, 0, 0) == 0);
	TEST_CHECK(strncmp(symtab[0].label, "test", 5) == 0);
	TEST_CHECK(symtab[0].offset == 1234);
	TEST_CHECK(strncmp(symtab[1].label, "label2", 7) == 0);
	TEST_CHECK(symtab[1].offset == 0xCAFEBABE);

	// Storing a third entry in a symbol table of size two returns an error
	// and leaves the first two entries unchanged.
	TEST_CHECK(strsymtabntr(symtab, 2, "foobar", 0xDEADC0DE, 0, 0) == 1);
	TEST_CHECK(strncmp(symtab[0].label, "test", 5) == 0);
	TEST_CHECK(symtab[0].offset == 1234);
	TEST_CHECK(strncmp(symtab[1].label, "label2", 7) == 0);
	TEST_CHECK(symtab[1].offset == 0xCAFEBABE);

	// Test that a relative target and flags can be stored
	TEST_CHECK(strsymtabntr(symtab, 3, "relative", 0xABCD11EF,
				FLAG_RELATIVE, 1234) == 0);
	TEST_CHECK(strncmp(symtab[0].label, "test", 5) == 0);
	TEST_CHECK(symtab[0].offset == 1234);
	TEST_CHECK(strncmp(symtab[1].label, "label2", 7) == 0);
	TEST_CHECK(symtab[1].offset == 0xCAFEBABE);
	TEST_CHECK(strncmp(symtab[2].label, "relative", 9) == 0);
	TEST_CHECK(symtab[2].offset == 0xABCD11EF);
	TEST_CHECK(symtab[2].flags == FLAG_RELATIVE);
	TEST_CHECK(symtab[2].rel_target == 1234);

	free(symtab);
}

void test_symbol_table_generation(void)
{
	struct AsmCtx *ctx = make_asmctx(
		"label1:\n"
		"\tnop\n"
		".sublabel1:\n"
		"\tnop\n"
		".end:\n"
		"\tnop\n"
		"label3:\n"
		"\tnop\n",
		8, 4, 0);
	TEST_ASSERT(ctx != NULL);
	assemble(ctx);
	TEST_CHECK(ctx->bintxt_size == 4);

	TEST_CHECK(strncmp(ctx->symtab[0].label, "label1", 7) == 0);
	TEST_CHECK(ctx->symtab[0].offset == 0);
	TEST_CHECK(strncmp(ctx->symtab[1].label, "label1.sublabel1", 16) == 0);
	TEST_CHECK(ctx->symtab[1].offset == 1);
	TEST_CHECK(strncmp(ctx->symtab[2].label, "label1.end", 16) == 0);
	TEST_CHECK(ctx->symtab[2].offset == 2);
	TEST_CHECK(strncmp(ctx->symtab[3].label, "label3", 7) == 0);
	TEST_CHECK(ctx->symtab[3].offset == 3);

	free_asmctx(ctx);
}
