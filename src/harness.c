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

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "0x864.h"

struct args {
        char inpath[256];
        char outpath[256];
        FILE *fin;
        FILE *fout;
};

void banner(void);
int parse_args(int, char * const [], struct args * const);
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

int parse_args(int argc, char * const argv[], struct args * const args)
{
        char opt;
        const char * const argv0 = argv[0];

        while ((opt = getopt(argc, argv, "ho:")) != -1) {
                switch (opt) {
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

void usage(const char * const argv0)
{
        printf("Usage: %s [...options] filename\n", argv0);
        printf("    Options:\n");
        printf("        -h          Show help\n");
        printf("        -o outfile  Write output to outfile\n");
}


int main(int argc, char * const argv[])
{
        struct args args = { 0 };
        char *assembly_buffer = NULL;
        char binary_buffer[512] = { 0 };
        int exit_code = 0;
        size_t assembly_buffer_size = 0;
        size_t binary_size = 0;
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

        assemble(assembly_buffer, binary_buffer, 512, &binary_size);

        fwrite(binary_buffer, binary_size, 1, args.fout);
        printf("Written %zu bytes of binary output\n", binary_size);

cleanup:
        if (args.fin != NULL)
                fclose(args.fin);
        if (args.fout != NULL)
                fclose(args.fout);
        if (assembly_buffer != NULL)
                free(assembly_buffer);

        return exit_code;
}
