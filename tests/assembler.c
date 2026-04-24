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
#include "assembler.h"

void test_struct_AsmOp_is_packed(void)
{
	// The assembly code makes assumptions about the offsets in the
	// structure. Verify, that the struct is packed.
	TEST_CHECK(sizeof(struct AsmOp) == 24);
}

void test_assemble_op(void)
{
	struct AsmOp op = { 0 };
	struct AsmCtx *ctx = NULL;

	// Test assembling the nop instruction
	ctx = make_asmctx("", 16, 0, 0);
	TEST_ASSERT(ctx != NULL);
	op.encoding = ENCODING_ZO;
	op.n_opcodes = 1;
	op.opcodes[0] = 0x90;
	assemble_op(ctx, &op);
	TEST_CHECK(ctx->bintxt_size == 1);
	TEST_CHECK(ctx->bintxt[0] == 0x90);
	free_asmctx(ctx);
	memset(&op, 0, sizeof(struct AsmOp));

	// Test assembling the call near relative instruction
	ctx = make_asmctx("", 16, 8, 8);
	strncpy(ctx->label, "label1", 7);
	TEST_ASSERT(ctx != NULL);
	op.encoding = ENCODING_D;
	op.op_size = 32;
	op.n_opcodes = 1;
	op.opcodes[0] = 0xE8;
	op.imm_size = 32;
	op.d_label = D_LABEL_RELATIVE;
	op.imm.imm32 = 0;
	assemble_op(ctx, &op);
	TEST_CHECK(ctx->bintxt_size == 5);
	TEST_CHECK(ctx->bintxt[0] == 0xE8);
	TEST_CHECK(ctx->bintxt[1] == 0x00);
	TEST_CHECK(ctx->bintxt[2] == 0x00);
	TEST_CHECK(ctx->bintxt[3] == 0x00);
	TEST_CHECK(ctx->bintxt[4] == 0x00);

	TEST_CHECK(strncmp(ctx->reftab[0].label, "label1", 7) == 0);

	free_asmctx(ctx);
	memset(&op, 0, sizeof(struct AsmOp));

	// Test assembling `lea rsi, [rbp - 1234]`
	ctx = make_asmctx("", 16, 0, 0);
	TEST_ASSERT(ctx != NULL);
	op.encoding = ENCODING_RM;
	op.op_size = 64;
	op.src_reg = 0b0101;
	op.dst_reg = 0b0110;
	op.n_opcodes = 1;
	op.opcodes[0] = 0x8D;
	op.modrm_mod = MOD_INDIRECT_32;
	op.disp.disp32 = -1234;
	assemble_op(ctx, &op);
	TEST_CHECK(ctx->bintxt_size == 7);
	TEST_CHECK(ctx->bintxt[0] == 0x48);
	TEST_CHECK(ctx->bintxt[1] == 0x8D);
	TEST_CHECK(ctx->bintxt[2] == 0xB5);
	TEST_CHECK(ctx->bintxt[3] == 0x2E);
	TEST_CHECK(ctx->bintxt[4] == 0xFB);
	TEST_CHECK(ctx->bintxt[5] == 0xFF);
	TEST_CHECK(ctx->bintxt[6] == 0xFF);
	free_asmctx(ctx);
	memset(&op, 0, sizeof(struct AsmOp));

	// Test assembling `mov [rbp - 8], r15`
	ctx = make_asmctx("", 16, 0, 0);
	TEST_ASSERT(ctx != NULL);
	op.encoding = ENCODING_MR;
	op.op_size = 64;
	op.src_reg = 0b1111;
	op.dst_reg = 0b0101;
	op.modrm_mod = MOD_INDIRECT_8;
	op.n_opcodes = 1;
	op.opcodes[0] = 0x89;
	op.disp.disp8 = -8;
	assemble_op(ctx, &op);
	TEST_CHECK(ctx->bintxt_size == 4);
	TEST_CHECK(ctx->bintxt[0] == 0x4C);
	TEST_CHECK(ctx->bintxt[1] == 0x89);
	TEST_CHECK(ctx->bintxt[2] == 0x7D);
	TEST_CHECK(ctx->bintxt[3] == 0xF8);
	free_asmctx(ctx);
	memset(&op, 0, sizeof(struct AsmOp));

	// Test assembling `add r10, 0x11223344`
	ctx = make_asmctx("", 16, 0, 0);
	TEST_ASSERT(ctx != NULL);
	op.encoding = ENCODING_MI;
	op.op_size = 64;
	op.dst_reg = 0b1010;
	op.modrm_mod = MOD_DIRECT;
	op.n_opcodes = 1;
	op.opcodes[0] = 0x81;
	op.imm_size = 32;
	op.imm.imm32 = 0x11223344;
	assemble_op(ctx, &op);
	TEST_CHECK(ctx->bintxt_size == 7);
	TEST_CHECK(ctx->bintxt[0] == 0x49);
	TEST_CHECK(ctx->bintxt[1] == 0x81);
	TEST_CHECK(ctx->bintxt[2] == 0xC2);
	TEST_CHECK(ctx->bintxt[3] == 0x44);
	TEST_CHECK(ctx->bintxt[4] == 0x33);
	TEST_CHECK(ctx->bintxt[5] == 0x22);
	TEST_CHECK(ctx->bintxt[6] == 0x11);
	free_asmctx(ctx);
	memset(&op, 0, sizeof(struct AsmOp));

	// Test assembling `int 0x80`
	ctx = make_asmctx("", 16, 0, 0);
	TEST_ASSERT(ctx != NULL);
	op.encoding = ENCODING_I;
	op.n_opcodes = 1;
	op.opcodes[0] = 0xcd;
	op.imm_size = 8;
	op.imm.imm8 = 0x80;
	assemble_op(ctx, &op);
	TEST_CHECK(ctx->bintxt_size == 2);
	TEST_CHECK(ctx->bintxt[0] == 0xcd);
	TEST_CHECK(ctx->bintxt[1] == 0x80);
	free_asmctx(ctx);
	memset(&op, 0, sizeof(struct AsmOp));

	// Test assembling `dec r11`
	ctx = make_asmctx("", 16, 0, 0);
	TEST_ASSERT(ctx != NULL);
	op.encoding = ENCODING_M;
	op.op_size = 64;
	op.src_reg = 0b001; // `dec` encodes a single bit in ModRM:reg
	op.dst_reg = 0b1011;
	op.modrm_mod = MOD_DIRECT;
	op.n_opcodes = 1;
	op.opcodes[0] = 0xff;
	assemble_op(ctx, &op);
	TEST_CHECK(ctx->bintxt_size == 3);
	TEST_CHECK(ctx->bintxt[0] == 0x49);
	TEST_CHECK(ctx->bintxt[1] == 0xff);
	TEST_CHECK(ctx->bintxt[2] == 0xcb);
	free_asmctx(ctx);
	memset(&op, 0, sizeof(struct AsmOp));

	// Test assembling `mov bl, 0xab`
	ctx = make_asmctx("", 16, 0, 0);
	TEST_ASSERT(ctx != NULL);
	op.encoding = ENCODING_OI;
	op.op_size = 8;
	op.dst_reg = 0b0011;
	op.n_opcodes = 1;
	op.opcodes[0] = 0xb0;
	op.imm_size = 8;
	op.imm.imm8 = 0xab;
	assemble_op(ctx, &op);
	TEST_CHECK(ctx->bintxt_size == 2);
	TEST_CHECK(ctx->bintxt[0] == 0xb3);
	TEST_CHECK(ctx->bintxt[1] == 0xab);
	free_asmctx(ctx);
	memset(&op, 0, sizeof(struct AsmOp));

	// Test assembling `mov r11b, 0xab`
	ctx = make_asmctx("", 16, 0, 0);
	TEST_ASSERT(ctx != NULL);
	op.encoding = ENCODING_OI;
	op.op_size = 8;
	op.dst_reg = 0b1011;
	op.n_opcodes = 1;
	op.opcodes[0] = 0xb0;
	op.imm_size = 8;
	op.imm.imm8 = 0xab;
	assemble_op(ctx, &op);
	TEST_CHECK(ctx->bintxt_size == 3);
	TEST_CHECK(ctx->bintxt[0] == 0x41);
	TEST_CHECK(ctx->bintxt[1] == 0xb3);
	TEST_CHECK(ctx->bintxt[2] == 0xab);
	free_asmctx(ctx);
	memset(&op, 0, sizeof(struct AsmOp));
}

void test_strdspmodrmmod(void)
{
	struct AsmOp op = { 0 };

	op.disp.disp32 = 0;
	strdspmodrmmod(&op);
	TEST_CHECK(op.modrm_mod == MOD_INDIRECT);
	memset(&op, 0, sizeof(struct AsmOp));

	op.disp.disp32 = 127;
	strdspmodrmmod(&op);
	TEST_CHECK(op.modrm_mod == MOD_INDIRECT_8);
	memset(&op, 0, sizeof(struct AsmOp));

	op.disp.disp32 = -128;
	strdspmodrmmod(&op);
	TEST_CHECK(op.modrm_mod == MOD_INDIRECT_8);
	memset(&op, 0, sizeof(struct AsmOp));

	op.disp.disp32 = -129;
	strdspmodrmmod(&op);
	TEST_CHECK(op.modrm_mod == MOD_INDIRECT_32);
	memset(&op, 0, sizeof(struct AsmOp));

	op.disp.disp32 = 128;
	strdspmodrmmod(&op);
	TEST_CHECK(op.modrm_mod == MOD_INDIRECT_32);
	memset(&op, 0, sizeof(struct AsmOp));
}

