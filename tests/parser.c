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
        { "skp2lbinst", test_skp2lbinst },
        { NULL, NULL }
};
