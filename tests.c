/*******************************************************************************
 *
 * tests.c - Test corpus
 *
 * Copyright (c) 2021 Basil Hussain
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 ******************************************************************************/

#include <stddef.h>
#include <stdint.h>
#include "tests.h"

// This included file is auto-generated from the contents of the 'tests' folder.
// See make_tests.bat there.
#include "tests/tests_data.c"

const test_case_t tests[TESTS_COUNT] = {
	{
		.plain = { .data = lzsa_test_01_plain, .length = sizeof(lzsa_test_01_plain) },
		.lzsa1 = { .data = lzsa_test_01_lzsa1, .length = sizeof(lzsa_test_01_lzsa1) },
		.lzsa2 = { .data = lzsa_test_01_lzsa2, .length = sizeof(lzsa_test_01_lzsa2) }
	},
	{
		.plain = { .data = lzsa_test_02_plain, .length = sizeof(lzsa_test_02_plain) },
		.lzsa1 = { .data = lzsa_test_02_lzsa1, .length = sizeof(lzsa_test_02_lzsa1) },
		.lzsa2 = { .data = lzsa_test_02_lzsa2, .length = sizeof(lzsa_test_02_lzsa2) }
	},
	{
		.plain = { .data = lzsa_test_03_plain, .length = sizeof(lzsa_test_03_plain) },
		.lzsa1 = { .data = lzsa_test_03_lzsa1, .length = sizeof(lzsa_test_03_lzsa1) },
		.lzsa2 = { .data = lzsa_test_03_lzsa2, .length = sizeof(lzsa_test_03_lzsa2) }
	},
	{
		.plain = { .data = lzsa_test_04_plain, .length = sizeof(lzsa_test_04_plain) },
		.lzsa1 = { .data = lzsa_test_04_lzsa1, .length = sizeof(lzsa_test_04_lzsa1) },
		.lzsa2 = { .data = lzsa_test_04_lzsa2, .length = sizeof(lzsa_test_04_lzsa2) }
	},
	{
		.plain = { .data = lzsa_test_05_plain, .length = sizeof(lzsa_test_05_plain) },
		.lzsa1 = { .data = lzsa_test_05_lzsa1, .length = sizeof(lzsa_test_05_lzsa1) },
		.lzsa2 = { .data = lzsa_test_05_lzsa2, .length = sizeof(lzsa_test_05_lzsa2) }
	},
	{
		.plain = { .data = lzsa_test_06_plain, .length = sizeof(lzsa_test_06_plain) },
		.lzsa1 = { .data = lzsa_test_06_lzsa1, .length = sizeof(lzsa_test_06_lzsa1) },
		.lzsa2 = { .data = lzsa_test_06_lzsa2, .length = sizeof(lzsa_test_06_lzsa2) }
	},
	{
		.plain = { .data = lzsa_test_07_plain, .length = sizeof(lzsa_test_07_plain) },
		.lzsa1 = { .data = lzsa_test_07_lzsa1, .length = sizeof(lzsa_test_07_lzsa1) },
		.lzsa2 = { .data = lzsa_test_07_lzsa2, .length = sizeof(lzsa_test_07_lzsa2) }
	},
	{
		.plain = { .data = lzsa_test_08_plain, .length = sizeof(lzsa_test_08_plain) },
		.lzsa1 = { .data = lzsa_test_08_lzsa1, .length = sizeof(lzsa_test_08_lzsa1) },
		.lzsa2 = { .data = lzsa_test_08_lzsa2, .length = sizeof(lzsa_test_08_lzsa2) }
	},
	{
		.plain = { .data = lzsa_test_09_plain, .length = sizeof(lzsa_test_09_plain) },
		.lzsa1 = { .data = lzsa_test_09_lzsa1, .length = sizeof(lzsa_test_09_lzsa1) },
		.lzsa2 = { .data = lzsa_test_09_lzsa2, .length = sizeof(lzsa_test_09_lzsa2) }
	},
	{
		.plain = { .data = lzsa_test_10_plain, .length = sizeof(lzsa_test_10_plain) },
		.lzsa1 = { .data = lzsa_test_10_lzsa1, .length = sizeof(lzsa_test_10_lzsa1) },
		.lzsa2 = { .data = lzsa_test_10_lzsa2, .length = sizeof(lzsa_test_10_lzsa2) }
	},
	{
		.plain = { .data = lzsa_test_11_plain, .length = sizeof(lzsa_test_11_plain) },
		.lzsa1 = { .data = lzsa_test_11_lzsa1, .length = sizeof(lzsa_test_11_lzsa1) },
		.lzsa2 = { .data = lzsa_test_11_lzsa2, .length = sizeof(lzsa_test_11_lzsa2) }
	},
};
