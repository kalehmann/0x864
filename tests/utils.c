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
#include "0x864.h"
#include "utils.h"

void test_clr(void)
{
        uint8_t buffer[] = { 0x01, 0x02, 0x03, 0x04 };
        clr(buffer, 0);
        TEST_CHECK(buffer[0] == 0x01);
        clr(buffer, 2);
        TEST_CHECK(buffer[0] == 0x00);
        TEST_CHECK(buffer[1] == 0x00);
        TEST_CHECK(buffer[2] == 0x03);
        TEST_CHECK(buffer[3] == 0x04);
}

void test_cpy(void)
{
        uint8_t src[] = { 0x01, 0x02, 0x03, 0x04 };
        uint8_t dst[] = { 0x00, 0x00, 0x00, 0x00 };

        cpy(src, dst, 0);
        TEST_CHECK(dst[0] == 0x00);
        TEST_CHECK(dst[1] == 0x00);
        TEST_CHECK(dst[2] == 0x00);
        TEST_CHECK(dst[3] == 0x00);

        cpy(src, dst, 2);
        TEST_CHECK(dst[0] == 0x01);
        TEST_CHECK(dst[1] == 0x02);
        TEST_CHECK(dst[2] == 0x00);
        TEST_CHECK(dst[3] == 0x00);
}

void test_len(void)
{
        TEST_CHECK(len("") == 1);
        TEST_CHECK(len("test") == 5);
}
