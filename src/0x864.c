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

#include <stdlib.h>
#include "0x864.h"

struct AsmCtx *make_asmctx(char const *assembly, size_t max_bintxt_size,
                           size_t max_symtab_entries, size_t max_reftab_entries)
{
        struct AsmCtx *ctx = calloc(1, sizeof(struct AsmCtx));
        if (ctx == NULL) {
                return NULL;
        }
        ctx->assembly = assembly;
        if (max_bintxt_size > 0) {
                ctx->bintxt = calloc(max_bintxt_size, 1);
                if (ctx->bintxt == NULL) {
                        free(ctx);

                        return NULL;
                }
        }
        ctx->max_bintxt_size = max_bintxt_size;
        if (max_symtab_entries > 0) {
                ctx->symtab = calloc(max_symtab_entries,
                                     (sizeof(struct SymTabNtr)));
                if (ctx->symtab == NULL) {
                        free(ctx->bintxt);
                        free(ctx);

                        return NULL;
                }
        }
        ctx->max_symtab_entries = max_symtab_entries;
        if (max_reftab_entries > 0) {
                ctx->reftab = calloc(max_reftab_entries,
                                     sizeof(struct SymTabNtr));
                if (ctx->reftab == NULL) {
                        free(ctx->bintxt);
                        free(ctx->symtab);
                        free(ctx);

                        return NULL;
                }
        }
        ctx->max_reftab_entries = max_reftab_entries;

        return ctx;
}

void free_asmctx(struct AsmCtx *ctx)
{
        if (ctx == NULL)
                return;

        free(ctx->bintxt);
        free(ctx->symtab);
        free(ctx->reftab);
        free(ctx);
}

