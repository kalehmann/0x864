#include <acutest.h>
#include <string.h>
#include "0x864.h"

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


TEST_LIST = {
        { "cklb", test_cklb },
        { "readnlbl", test_readnlbl },
        { "skp2lbinst", test_skp2lbinst },
        { NULL, NULL }
};
