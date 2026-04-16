#include <acutest.h>
#include <string.h>
#include "0x864.h"

void test_assemble(void)
{
        unsigned char binary[16] = { 0 };
        size_t bytes_written = 0xff;

        // Test that calling `assemble` with an output size of 0 writes nothing
        // to the output buffer.
        assemble("\tnop\n", NULL, 0, &bytes_written);
        TEST_CHECK(bytes_written == 0);

        // Test that the `assemble` function accepts an empty input.
        bytes_written = 0xff;
        assemble("", binary, 16, &bytes_written);
        TEST_CHECK(bytes_written == 0);
        TEST_CHECK(binary[0] == 0);
}

void test_as_nop(void)
{
        char const *assembly = "\n"
                "        push rbp";
        char const *assembly2 = assembly;
        unsigned char binary[16] = { 0 };
        size_t bytes_written = 0xff;

        // Test that nothing happens if 0 bytes should be written into the output
        as_nop(&assembly, NULL, 0, &bytes_written);
        TEST_CHECK(assembly == assembly2);
        TEST_CHECK(bytes_written == 0);

        // Test assembling `nop` to 0x90
        as_nop(&assembly, binary, 16, &bytes_written);
        TEST_CHECK(assembly == assembly2);
        TEST_CHECK(bytes_written == 1);
        TEST_CHECK(binary[0] == 0x90);
}

void test_as_retn(void)
{
        char const *assembly = "\n"
                "        push rbp";
        char const *assembly2 = assembly;
        unsigned char binary[16] = { 0 };
        size_t bytes_written = 0xff;

        // Test that nothing happens if 0 bytes should be written into the output
        as_retn(&assembly, NULL, 0, &bytes_written);
        TEST_CHECK(assembly == assembly2);
        TEST_CHECK(bytes_written == 0);

        // Test assembling `retn` to 0xc3
        as_retn(&assembly, binary, 16, &bytes_written);
        TEST_CHECK(assembly == assembly2);
        TEST_CHECK(bytes_written == 1);
        TEST_CHECK(binary[0] == 0xc3);
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
        unsigned int offset1 = 0xDEADC0DE;
        unsigned int offset2 = 0xCAFEBABE;
        unsigned int offset3 = 0x0DEFACED;
        unsigned int offset = 0;

        void *symtab = calloc(1, 1024);
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
        unsigned int offset1 = 1234;
        unsigned int offset2 = 0xCAFEBABE;
        void *symtab = calloc(1, 512);

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

TEST_LIST = {
        { "assemble", test_assemble },
        { "as_nop", test_as_nop },
        { "as_retn", test_as_retn },
        { "cklb", test_cklb },
        { "readnlbl", test_readnlbl },
        { "rslvref", test_rslvref },
        { "skp2lbinst", test_skp2lbinst },
        { "strsymtabntr", test_strsymtabntr },
        { NULL, NULL }
};
