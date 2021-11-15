/*******************************************************************************
 *
 * tests.h - Header for test corpus
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

#ifndef TESTS_H_
#define TESTS_H_

#include <stddef.h>
#include <stdint.h>

#define TESTS_COUNT 11
#define TESTS_DATA_PLAIN_MAX_LEN 1700
#define TESTS_DATA_LZSA_MAX_LEN 1200

typedef struct {
	struct {
		size_t length;
		uint8_t *data;
	} plain;
	struct {
		size_t length;
		uint8_t *data;
	} lzsa1;
	struct {
		size_t length;
		uint8_t *data;
	} lzsa2;
} test_case_t;

extern const test_case_t tests[TESTS_COUNT];

#endif // TESTS_H_
