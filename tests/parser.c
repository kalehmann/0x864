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
        TEST_CHECK(sizeof(struct AsmCtx) == 560);
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
        struct AsmCtx *ctx = make_asmctx("\tnop\n", 0, 0, 0, 0);
        TEST_ASSERT(ctx != NULL);
        TEST_CHECK(assemble(ctx) == ERR_NONE);
        TEST_CHECK(ctx->bintxt_size == 0);
        free_asmctx(ctx);

        // Test that the `assemble` function accepts an empty input.
        ctx = make_asmctx("", 0, 0, 0, 0);
        TEST_ASSERT(ctx != NULL);
        TEST_CHECK(assemble(ctx) == ERR_NONE);
        TEST_CHECK(ctx->bintxt_size == 0);

        // Test that the `assemble` function handles unknown instructions
        ctx = make_asmctx("\tnop\n\tinvalid\n", 0, 0, 0, 0);
        TEST_ASSERT(ctx != NULL);
        TEST_CHECK(assemble(ctx) == ERR_UNKNOWN_INSTRUCTION);
}

void test_as_call(void)
{
        struct AsmOp op = { 0 };
        struct AsmCtx *ctx = make_asmctx(" .label ; Comment", 16, 8, 8, 0);
        TEST_ASSERT(ctx != NULL);
        as_call(ctx, &op);

        TEST_CHECK(op.encoding == ENCODING_D);
        TEST_CHECK(op.op_size == 32);
        TEST_CHECK(op.n_opcodes == 1);
        TEST_CHECK(op.opcodes[0] == 0xE8);
        TEST_CHECK(op.imm_size == 32);

        free_asmctx(ctx);
}

void test_as_nop(void)
{
        struct AsmOp op = { 0 };
        struct AsmCtx *ctx = make_asmctx("", 0, 0, 0, 0);
        TEST_ASSERT(ctx != NULL);
        as_nop(ctx, &op);

        TEST_CHECK(op.encoding == ENCODING_ZO);
        TEST_CHECK(op.n_opcodes == 1);
        TEST_CHECK(op.opcodes[0] == 0x90);

        free_asmctx(ctx);
}

void test_as_retn(void)
{
        struct AsmOp op = { 0 };
        struct AsmCtx *ctx = make_asmctx("", 0, 0, 0, 0);
        TEST_ASSERT(ctx != NULL);
        as_retn(ctx, &op);

        TEST_CHECK(op.encoding == ENCODING_ZO);
        TEST_CHECK(op.n_opcodes == 1);
        TEST_CHECK(op.opcodes[0] == 0xc3);

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

void test_ckopsize(void)
{
        TEST_CHECK(ckopsize("al", 1) == 8);
        TEST_CHECK(ckopsize("bx", 1) == 16);
        TEST_CHECK(ckopsize("ecx", 1) == 32);
        TEST_CHECK(ckopsize("rdx", 1) == 64);

        TEST_CHECK(ckopsize("[rsi + 32], bx", 2) == 16);
        TEST_CHECK(ckopsize("rsi, source_label", 2) == 64);
        TEST_CHECK(ckopsize("al, 0xff", 2) == 8);
}

void test_ckoptps(void)
{
        uint16_t op_type = OP_TYPE_REG;
        TEST_CHECK(ckoptps("al", 1) == op_type);
        TEST_CHECK(ckoptps("foobar", 1) == OP_TYPE_LBL);
        TEST_CHECK(ckoptps("0xff", 1) == OP_TYPE_IMM);
        op_type = (OP_TYPE_RGNDRCT << 4) | OP_TYPE_REG;
        TEST_CHECK(ckoptps("[rbp - 8], al", 2) == op_type);
        op_type = (OP_TYPE_REG << 4) | OP_TYPE_LBL;
        TEST_CHECK(ckoptps("rsi, label", 2) == op_type);
        op_type = (OP_TYPE_REG << 4) | OP_TYPE_REG;
        TEST_CHECK(ckoptps("eax, edi", 2) == op_type);
        op_type = (OP_TYPE_REG << 4) | OP_TYPE_REG;
        TEST_CHECK(ckoptps("bl, ah\n", 2) == op_type);
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

void test_isrgndrct(void)
{
        TEST_CHECK(isrgndrct("[rdi]") == 1);
        TEST_CHECK(isrgndrct("[rsi + 10]") == 1);
        TEST_CHECK(isrgndrct("rdi") == 0);
        TEST_CHECK(isrgndrct("0xff") == 0);
}

void test_pint(void)
{
        char *b = NULL;
        char *buf = calloc(32, 1);
        TEST_ASSERT(buf != NULL);
        b = buf;

        strncpy(buf, "0", 32);
        TEST_CHECK(pint(&buf) == 0);
        TEST_CHECK(buf == b + 1);
        buf = b;

        strncpy(buf, "42", 32);
        TEST_CHECK(pint(&buf) == 42);
        TEST_CHECK(buf == b + 2);
        buf = b;

        strncpy(buf, "1024", 32);
        TEST_CHECK(pint(&buf) == 1024);
        TEST_CHECK(buf == b + 4);
        buf = b;

        strncpy(buf, "4294967295", 32);
        TEST_CHECK(pint(&buf) == 0xffffffff);
        TEST_CHECK(buf == b + 10);
        buf = b;

        strncpy(buf, "18446744073709551615", 32);
        TEST_CHECK(pint(&buf) == 0xffffffffffffffff);
        TEST_CHECK(buf == b + 20);
        buf = b;

        strncpy(buf, "0x00", 32);
        TEST_CHECK(pint(&buf) == 0);
        TEST_CHECK(buf == b + 4);
        buf = b;

        strncpy(buf, "0x10", 32);
        TEST_CHECK(pint(&buf) == 16);
        TEST_CHECK(buf == b + 4);
        buf = b;

        strncpy(buf, "0xcafebabe", 32);
        TEST_CHECK(pint(&buf) == 0xcafebabe);
        TEST_CHECK(buf == b + 10);
        buf = b;

        strncpy(buf, "0xdeadc0de", 32);
        TEST_CHECK(pint(&buf) == 0xdeadc0de);
        TEST_CHECK(buf == b + 10);
        buf = b;

        strncpy(buf, "0xffffffff", 32);
        TEST_CHECK(pint(&buf) == 0xffffffff);
        TEST_CHECK(buf == b + 10);
        buf = b;

        strncpy(buf, "0xffffffffeeddccbb", 32);
        TEST_CHECK(pint(&buf) == 0xffffffffeeddccbb);
        TEST_CHECK(buf == b + 18);
        buf = b;

        free(buf);
}

void test_pr8(void)
{
        char *b = NULL;
        char *buf = calloc(32, 1);
        TEST_ASSERT(buf != NULL);
        b = buf;

        strncpy(buf, "al", 32);
        TEST_CHECK(pr8(&buf) == 0b0000);
        TEST_CHECK(buf == b + 2);
        buf = b;

        strncpy(buf, "ah", 32);
        TEST_CHECK(pr8(&buf) == 0b0100);
        TEST_CHECK(buf == b + 2);
        buf = b;

        strncpy(buf, "bl", 32);
        TEST_CHECK(pr8(&buf) == 0b0011);
        TEST_CHECK(buf == b + 2);
        buf = b;

        strncpy(buf, "bh", 32);
        TEST_CHECK(pr8(&buf) == 0b0111);
        TEST_CHECK(buf == b + 2);
        buf = b;

        strncpy(buf, "cl; Comment here", 32);
        TEST_CHECK(pr8(&buf) == 0b0001);
        TEST_CHECK(buf == b + 2);
        buf = b;

        strncpy(buf, "ch  test", 32);
        TEST_CHECK(pr8(&buf) == 0b0101);
        TEST_CHECK(buf == b + 2);
        buf = b;

        strncpy(buf, "dl\n", 32);
        TEST_CHECK(pr8(&buf) == 0b0010);
        TEST_CHECK(buf == b + 2);
        buf = b;

        strncpy(buf, "dh\t", 32);
        TEST_CHECK(pr8(&buf) == 0b0110);
        TEST_CHECK(buf == b + 2);
        buf = b;

        strncpy(buf, "ax\t", 32);
        TEST_CHECK(pr8(&buf) == 0xff);
        TEST_CHECK(buf == b);
        buf = b;

        strncpy(buf, "rax", 32);
        TEST_CHECK(pr8(&buf) == 0xff);
        TEST_CHECK(buf == b);
        buf = b;

        strncpy(buf, "ecx", 32);
        TEST_CHECK(pr8(&buf) == 0xff);
        TEST_CHECK(buf == b);
        buf = b;

        strncpy(buf, "alt", 32);
        TEST_CHECK(pr8(&buf) == 0xff);
        TEST_CHECK(buf == b);
        buf = b;

        free(buf);
}

void test_pr16(void)
{
        char *b = NULL;
        char *buf = calloc(32, 1);
        TEST_ASSERT(buf != NULL);
        b = buf;

        strncpy(buf, "ax", 32);
        TEST_CHECK(pr16(&buf) == 0b0000);
        TEST_CHECK(buf == b + 2);
        buf = b;

        strncpy(buf, "bx", 32);
        TEST_CHECK(pr16(&buf) == 0b0011);
        TEST_CHECK(buf == b + 2);
        buf = b;

        strncpy(buf, "cx; Comment here", 32);
        TEST_CHECK(pr16(&buf) == 0b0001);
        TEST_CHECK(buf == b + 2);
        buf = b;

        strncpy(buf, "dx  test", 32);
        TEST_CHECK(pr16(&buf) == 0b0010);
        TEST_CHECK(buf == b + 2);
        buf = b;

        strncpy(buf, "rax", 32);
        TEST_CHECK(pr16(&buf) == 0xff);
        TEST_CHECK(buf == b);
        buf = b;

        strncpy(buf, "ecx", 32);
        TEST_CHECK(pr16(&buf) == 0xff);
        TEST_CHECK(buf == b);
        buf = b;

        strncpy(buf, "ex", 32);
        TEST_CHECK(pr16(&buf) == 0xff);
        TEST_CHECK(buf == b);
        buf = b;

        free(buf);
}

void test_pr32(void)
{
        char *b = NULL;
        char *buf = calloc(32, 1);
        TEST_ASSERT(buf != NULL);
        b = buf;

        strncpy(buf, "eax", 32);
        TEST_CHECK(pr32(&buf) == 0b0000);
        TEST_CHECK(buf == b + 3);
        buf = b;

        strncpy(buf, "ebx", 32);
        TEST_CHECK(pr32(&buf) == 0b0011);
        TEST_CHECK(buf == b + 3);
        buf = b;

        strncpy(buf, "ecx; Comment here", 32);
        TEST_CHECK(pr32(&buf) == 0b0001);
        TEST_CHECK(buf == b + 3);
        buf = b;

        strncpy(buf, "edx  test", 32);
        TEST_CHECK(pr32(&buf) == 0b0010);
        TEST_CHECK(buf == b + 3);
        buf = b;

        strncpy(buf, "edi", 32);
        TEST_CHECK(pr32(&buf) == 0b0111);
        TEST_CHECK(buf == b + 3);
        buf = b;

        strncpy(buf, "esi", 32);
        TEST_CHECK(pr32(&buf) == 0b0110);
        TEST_CHECK(buf == b + 3);
        buf = b;

        strncpy(buf, "r8d", 32);
        TEST_CHECK(pr32(&buf) == 0b1000);
        TEST_CHECK(buf == b + 3);
        buf = b;

        strncpy(buf, "r9d", 32);
        TEST_CHECK(pr32(&buf) == 0b1001);
        TEST_CHECK(buf == b + 3);
        buf = b;

        strncpy(buf, "r10d", 32);
        TEST_CHECK(pr32(&buf) == 0b1010);
        TEST_CHECK(buf == b + 4);
        buf = b;

        strncpy(buf, "r11d", 32);
        TEST_CHECK(pr32(&buf) == 0b1011);
        TEST_CHECK(buf == b + 4);
        buf = b;

        strncpy(buf, "r12d", 32);
        TEST_CHECK(pr32(&buf) == 0b1100);
        TEST_CHECK(buf == b + 4);
        buf = b;

        strncpy(buf, "r13d", 32);
        TEST_CHECK(pr32(&buf) == 0b1101);
        TEST_CHECK(buf == b + 4);
        buf = b;

        strncpy(buf, "r14d", 32);
        TEST_CHECK(pr32(&buf) == 0b1110);
        TEST_CHECK(buf == b + 4);
        buf = b;

        strncpy(buf, "r15d", 32);
        TEST_CHECK(pr32(&buf) == 0b1111);
        TEST_CHECK(buf == b + 4);
        buf = b;

        strncpy(buf, "r8", 32);
        TEST_CHECK(pr32(&buf) == 0xff);
        TEST_CHECK(buf == b);
        buf = b;

        strncpy(buf, "r9", 32);
        TEST_CHECK(pr32(&buf) == 0xff);
        TEST_CHECK(buf == b);
        buf = b;

        strncpy(buf, "r10", 32);
        TEST_CHECK(pr32(&buf) == 0xff);
        TEST_CHECK(buf == b);
        buf = b;

        strncpy(buf, "r11", 32);
        TEST_CHECK(pr32(&buf) == 0xff);
        TEST_CHECK(buf == b);
        buf = b;

        strncpy(buf, "r12", 32);
        TEST_CHECK(pr32(&buf) == 0xff);
        TEST_CHECK(buf == b);
        buf = b;

        strncpy(buf, "r13", 32);
        TEST_CHECK(pr32(&buf) == 0xff);
        TEST_CHECK(buf == b);
        buf = b;

        strncpy(buf, "r14", 32);
        TEST_CHECK(pr32(&buf) == 0xff);
        TEST_CHECK(buf == b);
        buf = b;

        strncpy(buf, "r15", 32);
        TEST_CHECK(pr32(&buf) == 0xff);
        TEST_CHECK(buf == b);
        buf = b;

        free(buf);
}

void test_pr64(void)
{
        char *b = NULL;
        char *buf = calloc(32, 1);
        TEST_ASSERT(buf != NULL);
        b = buf;

        strncpy(buf, "rax", 32);
        TEST_CHECK(pr64(&buf) == 0);
        TEST_CHECK(buf == b + 3);
        buf = b;

        strncpy(buf, "rbx", 32);
        TEST_CHECK(pr64(&buf) == 0b0011);
        TEST_CHECK(buf == b + 3);
        buf = b;

        strncpy(buf, "rcx ; Comment", 32);
        TEST_CHECK(pr64(&buf) == 0b0001);
        TEST_CHECK(buf == b + 3);
        buf = b;

        strncpy(buf, "rdx, [rdi]", 32);
        TEST_CHECK(pr64(&buf) == 0b0010);
        TEST_CHECK(buf == b + 3);
        buf = b;

        strncpy(buf, "rsp", 32);
        TEST_CHECK(pr64(&buf) == 0b0100);
        TEST_CHECK(buf == b + 3);
        buf = b;

        strncpy(buf, "rbp", 32);
        TEST_CHECK(pr64(&buf) == 0b0101);
        TEST_CHECK(buf == b + 3);
        buf = b;

        strncpy(buf, "rdi", 32);
        TEST_CHECK(pr64(&buf) == 0b0111);
        TEST_CHECK(buf == b + 3);
        buf = b;

        strncpy(buf, "rsi", 32);
        TEST_CHECK(pr64(&buf) == 0b0110);
        TEST_CHECK(buf == b + 3);
        buf = b;

        strncpy(buf, "r8", 32);
        TEST_CHECK(pr64(&buf) == 0b1000);
        TEST_CHECK(buf == b + 2);
        buf = b;

        strncpy(buf, "r9", 32);
        TEST_CHECK(pr64(&buf) == 0b1001);
        TEST_CHECK(buf == b + 2);
        buf = b;

        strncpy(buf, "r10", 32);
        TEST_CHECK(pr64(&buf) == 0b1010);
        TEST_CHECK(buf == b + 3);
        buf = b;

        strncpy(buf, "r11", 32);
        TEST_CHECK(pr64(&buf) == 0b1011);
        TEST_CHECK(buf == b + 3);
        buf = b;

        strncpy(buf, "r12", 32);
        TEST_CHECK(pr64(&buf) == 0b1100);
        TEST_CHECK(buf == b + 3);
        buf = b;

        strncpy(buf, "r13", 32);
        TEST_CHECK(pr64(&buf) == 0b1101);
        TEST_CHECK(buf == b + 3);
        buf = b;

        strncpy(buf, "r14", 32);
        TEST_CHECK(pr64(&buf) == 0b1110);
        TEST_CHECK(buf == b + 3);
        buf = b;

        strncpy(buf, "r15\t", 32);
        TEST_CHECK(pr64(&buf) == 0b1111);
        TEST_CHECK(buf == b + 3);
        buf = b;

        free(buf);
}

void test_preg(void)
{
        char *b = NULL;
        char *buf = calloc(32, 1);
        TEST_ASSERT(buf != NULL);
        b = buf;

        strncpy(buf, "lol", 32);
        TEST_CHECK(preg(&buf) == 0xff);
        TEST_CHECK(buf == b);
        buf = b;

        strncpy(buf, "r15d ; Comment ", 32);
        TEST_CHECK(preg(&buf) == 0b1111);
        TEST_CHECK(buf == b + 4);
        buf = b;

        strncpy(buf, "ebx\n", 32);
        TEST_CHECK(preg(&buf) == 0b0011);
        TEST_CHECK(buf == b + 3);
        buf = b;

        free(buf);
}

void test_prgndrct(void)
{
        char *b = NULL;
        char *buf = calloc(64, 1);
        uint8_t reg = 0;
        uint32_t disp = 0;
        TEST_ASSERT(buf != NULL);
        b = buf;

        strncpy(buf, "[rbp - 8] ; <- This is a register indirect access", 64);
        prgndrct(&buf, &reg, &disp);
        TEST_CHECK(reg == 0b0101);
        TEST_CHECK(disp == -8);
        TEST_CHECK(buf == b + 9);
        buf = b;

        strncpy(buf, "[  rsi + 1024 \t ]", 64);
        prgndrct(&buf, &reg, &disp);
        TEST_CHECK(reg == 0b0110);
        TEST_CHECK(disp == 1024);
        TEST_CHECK(buf == b + 17);
        buf = b;

        strncpy(buf, "[r8]", 64);
        prgndrct(&buf, &reg, &disp);
        TEST_CHECK(reg == 0b1000);
        TEST_CHECK(disp == 0);
        TEST_CHECK(buf == b + 4);
        buf = b;

        free(buf);
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
        struct AsmCtx *ctx = make_asmctx(NULL, 32, 8, 8, 0);
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

        // Test calling the function repeatedly does nothing
        skp2lbinst(&assembly1);
        TEST_CHECK(assembly1 == label1);

        skp2lbinst(&assembly2);
        TEST_CHECK(assembly2 == label2);
}

void test_skp2nxtop(void)
{
        char const *assembly1 = " , rax";
        const char * const op1 = strstr(assembly1, "rax");
        char const *assembly2 = "\t, \t 0x864";
        const char * const op2 = strstr(assembly2, "0x864");

        skp2nxtop(&assembly1);
        TEST_CHECK(assembly1 == op1);

        skp2nxtop(&assembly2);
        TEST_CHECK(assembly2 == op2);
}

void test_storing_globals(void)
{
        struct AsmCtx *ctx = make_asmctx(
                "\tglobal foo\n"
                "\tglobal bar\n"
                "\foo:\n"
                "\tnop\n"
                "\bar:\n"
                "\tnop\n",
                8, 4, 0, 8);
        TEST_ASSERT(ctx != NULL);
        assemble(ctx);
        TEST_CHECK(ctx->bintxt_size == 2);

        TEST_CHECK(strncmp(ctx->globals[0], "foo", 4) == 0);
        TEST_CHECK(strncmp(ctx->globals[1], "bar", 4) == 0);
        TEST_CHECK(ctx->globals[2][0] == 0);

        free_asmctx(ctx);
}

void test_strglbl(void)
{
        struct AsmCtx *ctx = NULL;

        ctx = make_asmctx(" foobar ; test", 0, 0, 0, 0);
        TEST_ASSERT(ctx != NULL);
        TEST_CHECK(strglbl(ctx) == -1);
        free_asmctx(ctx);

        ctx = make_asmctx(" foobar ; test", 0, 0, 0, 8);
        TEST_ASSERT(ctx != NULL);
        TEST_CHECK(strglbl(ctx) == 7);
        TEST_CHECK(strncmp(ctx->globals[0], "foobar", 7) == 0);
        free_asmctx(ctx);
}

void test_strlbl(void)
{
        const char *assembly = "label1:\n"
                ".sublabel1:\n"
                ".sublabel2:\n"
                ".end:\n"
                "label2:\n"
                "end:\n";

        struct AsmCtx *ctx = make_asmctx(assembly, 0, 0, 0, 0);
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
                8, 4, 0, 0);
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
