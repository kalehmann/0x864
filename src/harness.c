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

#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "0x864.h"

enum Format {
        BIN = 0,
        ELF64,
};

struct args {
        char inpath[256];
        char outpath[256];
        enum Format format;
        FILE *fin;
        FILE *fout;
        bool verbose;
};

void banner(void);
void dump_context(const char * const, struct AsmCtx *);
int parse_args(int, char * const [], struct args * const);
void print_error(const char * const, struct AsmCtx *, enum AsmErr);
void usage(const char * const);

void banner(void)
{
        printf("\033[1m0x864\033[0m - "
               "\033[38;5;52ms"
               "\033[38;5;88me"
               "\033[38;5;124ml"
               "\033[38;5;160mf"
               "\033[38;5;166m."
               "\033[38;5;172mc"
               "\033[1;38;5;208m0"
               "\033[0;38;5;202mn"
               "\033[38;5;214mt"
               "\033[38;5;220ma"
               "\033[38;5;226mi"
               "\033[38;5;11mn"
               "\033[38;5;154me"
               "\033[38;5;119md"
               "\033[38;5;118m."
               "\033[1;38;5;106mx"
               "\033[38;5;45m8"
               "\033[38;5;39m6"
               "\033[0;38;5;33m."
               "\033[1;38;5;32m4"
               "\033[0;38;5;21ms"
               "\033[38;5;20ms"
               "\033[38;5;19me"
               "\033[38;5;18mm"
               "\033[38;5;135mb"
               "\033[38;5;92ml"
               "\033[38;5;127me"
               "\033[38;5;90mr"
               "\033[0m\n");
}

void dump_context(const char * assembly_text, struct AsmCtx *ctx)
{
        size_t current_line = ckln(assembly_text, ctx->assembly);
        size_t globals = 0;
        size_t lines = 1;

        while (*(assembly_text++) != '\0') {
                if (*assembly_text == '\n')
                        lines++;
        }

        for (size_t i = 0; i < ctx->max_globals; i++) {
                if (ctx->globals[i][0] != '\0') {
                        globals++;
                }
        }

        printf("Context usage:\n"
               " [%4zu of %4zu] bytes of assembled code \n"
               " [%4zu of %4zu] globals \n"
               " [%4zu of %4zu] labels \n"
               " [%4zu of %4zu] references \n"
               "\n"
               "Parser is currently on line %zu of %zu\n",
               ctx->bintxt_size, ctx->max_bintxt_size,
               globals, ctx->max_globals,
               symtablen(ctx->symtab, ctx->max_symtab_entries),
               ctx->max_symtab_entries,
               symtablen(ctx->reftab, ctx->max_reftab_entries),
               ctx->max_reftab_entries, current_line, lines);
}

int parse_args(int argc, char * const argv[], struct args * const args)
{
        char opt;
        const char * const argv0 = argv[0];

        while ((opt = getopt(argc, argv, "f:ho:v")) != -1) {
                switch (opt) {
                case 'f':
                        if (strlen(optarg) > 255) {
                                fprintf(stderr, "Format is too long!\n");
                                usage(argv0);

                                return 1;
                        }
                        if (strncmp(optarg, "elf64", 6) == 0) {
                                args->format = ELF64;
                        }
                        break;
                case 'h':
                        usage(argv0);

                        return 0;
                case 'o':
                        if (strlen(optarg) > 255) {
                                fprintf(stderr, "Outfile is too long!\n");
                                usage(argv0);

                                return 1;
                        }
                        strncpy(args->outpath, optarg, 255);
                        break;
                case 'v':
                        args->verbose = true;
                        break;
                case ':':
                case '?':
                default:
                        usage(argv0);

                        return 1;
                }
        }

        if (optind >= argc) {
                fprintf(stderr, "Missing input file\n");
                usage(argv0);

                return 1;
        } else if (argc - optind > 1) {
                fprintf(stderr, "Too many input files\n");
                usage(argv0);

                return 1;
        } else if (strlen(argv[optind]) > 255) {
                fprintf(stderr, "Infile is too long!\n");
                usage(argv0);

                return 1;
        }

        strncpy(args->inpath, argv[optind], 255);
        if (0 == args->outpath[0]) {
                char *pos = strrchr(args->inpath, '.');
                if (NULL == pos) {
                        strncpy(args->outpath, args->inpath, 250);
                        strncat(args->outpath, ".out", 5);
                } else {
                        size_t n = pos - args->inpath;
                        strncpy(args->outpath, args->inpath, n);
                }
        }

        args->fin = fopen(args->inpath, "r");
        if (args->fin == NULL) {
                fprintf(stderr, "Unable to open input file %s: %s\n",
                        args->inpath, strerror(errno));

                return -1;
        }

        args->fout = fopen(args->outpath, "w");
        if (args->fout == NULL) {
                fclose(args->fin);
                fprintf(stderr, "Unable to open output file %s: %s\n",
                        args->outpath, strerror(errno));

                return -1;
        }

        return 0;
}

void print_error(const char * const assembly_text, struct AsmCtx *ctx,
                 enum AsmErr err)
{
        fflush(stdout);
        char *line = NULL;
        char const *linebreak = strchr(ctx->assembly, '\n');
        size_t n = 0;

        if (linebreak != NULL) {
                n = linebreak - ctx->assembly;
        } else {
                n = strlen(ctx->assembly);
        }

        line = calloc(n + 1, 1);
        assert(line != NULL);
        strncpy(line, ctx->assembly, n);
        line[n] = '\0';

        fprintf(stderr, "\033[1;38;5;160mFailure\033[0;m\n\n");

        switch (err) {
        case ERR_UNKNOWN_INSTRUCTION:
                fprintf(stderr, "Unknown instruction in line %zu "
                        "arround \"%s\"\n", ckln(assembly_text, ctx->assembly),
                        line);
                break;
        case ERR_INVALID_OPERANDS:
                fprintf(stderr, "Invalid operands for instruction in line %zu "
                        "arround \"%s\"\n", ckln(assembly_text, ctx->assembly),
                        line);
                break;
        case ERR_UNKNOWN_REFERENCE:
                fprintf(stderr, "Reference to unknown label \"%s\" \n", ctx->label);
                break;
        case ERR_TOO_MANY_GLOBALS:
                fprintf(stderr, "Too many globals - unable to store \"%s\"\n",
                        ctx->label);
                break;
        case ERR_TOO_MANY_LABELS:
                fprintf(stderr, "Too many labels - unable to store \"%s\"\n",
                        ctx->label);
                break;
        case ERR_TOO_MANY_REFERENCES:
                fprintf(stderr, "Too many referencess - unable to store "
                        "reference to \"%s\" on line %zu\n",
                        ctx->label, ckln(assembly_text, ctx->assembly));
                break;
        case ERR_BINTXT_BUFFER_TOO_SMALL:
                fprintf(stderr, "bintxt buffer is full - assembled program "
                        "exceeds %zu bytes\n", ctx->max_bintxt_size);
                break;
        default:
                fprintf(stderr, "Unknown error in line %zu arround \"%s\"\n",
                        ckln(assembly_text, ctx->assembly), line);
        }
}

void usage(const char * const argv0)
{
        printf("Usage: %s [...options] filename\n", argv0);
        printf("    Options:\n");
        printf("        -f [bin|elf64] Output format\n");
        printf("        -h             Show help\n");
        printf("        -o outfile     Write output to outfile\n");
}


int main(int argc, char * const argv[])
{
        struct args args = { 0 };
        struct AsmCtx *ctx = NULL;
        enum AsmErr err;
        char *assembly_buffer = NULL;
        int exit_code = 0;
        size_t assembly_buffer_size = 0;
        banner();

        if (parse_args(argc, argv, &args) != 0)
                return 1;
        if (args.inpath[0] == '\0')
                return 0;

        printf("Assembling %s to %s ... ", args.inpath, args.outpath);
        fseek(args.fin, 0L, SEEK_END);
        assembly_buffer_size = ftell(args.fin) + 1;
        rewind(args.fin);

        assembly_buffer = calloc(1, assembly_buffer_size);
        if (assembly_buffer == NULL) {
                fprintf(stderr, "Unable to allocate %zu bytes: %s\n",
                        assembly_buffer_size, strerror(errno));
                exit_code = 1;
                goto cleanup;
        }
        fread(assembly_buffer, assembly_buffer_size, 1, args.fin);

        ctx = make_asmctx(assembly_buffer, 512, 256, 1024, 128);
        assert(ctx != NULL);

        err = assemble(ctx);

        if (err != ERR_NONE) {
                print_error(assembly_buffer, ctx, err);
                exit_code = 1;
                goto cleanup;
        }

        if (args.format == BIN) {
                fwrite(ctx->bintxt, ctx->bintxt_size, 1, args.fout);
                printf("Written %zu bytes of binary output\n", ctx->bintxt_size);
        } else if (args.format == ELF64) {
                void *buffer = calloc(1024 * 64, 1);
                if (buffer == NULL) {
                        fprintf(stderr, "Unable to allocate %d bytes: %s\n",
                                1024 * 64, strerror(errno));
                        exit_code = 1;
                        goto cleanup;
                }
                size_t size = elf64_dump(ctx, buffer, 1024 * 64, args.inpath);
                fwrite(buffer, size, 1, args.fout);
                printf("Written %zu bytes of binary output\n", size);
        }

cleanup:
        if (args.verbose == true) {
                dump_context(assembly_buffer, ctx);
        }

        if (args.fin != NULL)
                fclose(args.fin);
        if (args.fout != NULL)
                fclose(args.fout);
        if (ctx != NULL)
                free_asmctx(ctx);

        return exit_code;
}
