/*******************************************************************************
 *
 * main.c - Test and benchmarking code for STM8 LZSA decompression library
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
#include <stdbool.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "uart.h"
#include "ucsim.h"
#include "lzsa_ref.h"
#include "lzsa.h"
#include "tests.h"

#define CLK_CKDIVR (*(volatile uint8_t *)(0x50C6))

// PC5 is connected to the built-in LED on the STM8S208 Nucleo-64 board.
// Toggling this pin is used for benchmarking.
#define PC_ODR (*(volatile uint8_t *)(0x500A))
#define PC_ODR_ODR5 5
#define PC_DDR (*(volatile uint8_t *)(0x500C))
#define PC_DDR_DDR5 5
#define PC_CR1 (*(volatile uint8_t *)(0x500D))
#define PC_CR1_C15 5

/******************************************************************************/

typedef struct {
	uint16_t pass_count;
	uint16_t fail_count;
} test_result_t;

// Use ANSI terminal escape codes for highlighting pass/fail text.
static const char pass_str[] = "\x1B[1m\x1B[32mPASS\x1B[0m"; // Bold green
static const char fail_str[] = "\x1B[1m\x1B[31mFAIL\x1B[0m"; // Bold red
static const char test_str[] = "\x1B[1m\x1B[33mTEST\x1B[0m"; // Bold yellow
static const char hrule_str[] = "----------------------------------------";

#define count_test_result(x, r) \
	do { \
		if(x) { \
			(r)->pass_count++; \
		} else { \
			(r)->fail_count++; \
		} \
	} while(0)

#define benchmark_marker_start() do { PC_ODR |= (1 << PC_ODR_ODR5); } while(0)
#define benchmark_marker_end() do { PC_ODR &= ~(1 << PC_ODR_ODR5); } while(0)
#define benchmark(s, i, o) \
	do { \
		printf("\x1B[1m\x1B[33mBENCHMARK\x1B[0m: " s "\n"); \
		uint16_t n = (i); \
		benchmark_marker_start(); \
		while(n--) (o); \
		benchmark_marker_end(); \
	} while(0)

static uint8_t test_out[TESTS_DATA_PLAIN_MAX_LEN];

/******************************************************************************/

static void print_hex_data(const uint8_t *data, const size_t len) {
	const size_t row_len = 16;
	bool end = false;

	for(size_t i = 0; i < len; i += row_len) {
		printf("0x%04X: ", i);
		for(size_t j = 0; j < row_len; j++) {
			end = (i + j >= len);
			if(!end) {
				printf("%02X", data[i + j]);
			} else {
				putchar(' ');
				putchar(' ');
			}
		}
		putchar(' ');
		putchar(' ');
		for(size_t j = 0; j < row_len; j++) {
			end = (i + j >= len);
			if(!end) {
				putchar(isprint(data[i + j]) ? data[i + j] : '.');
			} else {
				break;
			}
		}
		putchar('\n');
		if(end) break;
	}
}

static void test_lzsa1(test_result_t *result) {
	ptrdiff_t out_len;
	bool pass;

	for(size_t i = 0; i < TESTS_COUNT; i++) {
		printf("%s %02u:\n", test_str, i + 1);

		memset(test_out, '\0', sizeof(test_out));
		puts("lzsa1_decompress_block_ref()");
		out_len = lzsa1_decompress_block_ref(test_out, tests[i].lzsa1.data) - test_out;
		pass = (memcmp(test_out, tests[i].plain.data, tests[i].plain.length) == 0 && out_len == tests[i].plain.length);
		print_hex_data(test_out, out_len);
		printf("plain_len = %u, out_len = %td\n", tests[i].plain.length, out_len);
		puts(pass ? pass_str : fail_str);
		count_test_result(pass, result);

		memset(test_out, '\0', sizeof(test_out));
		puts("lzsa1_decompress_block()");
		out_len = lzsa1_decompress_block(test_out, tests[i].lzsa1.data) - test_out;
		pass = (memcmp(test_out, tests[i].plain.data, tests[i].plain.length) == 0 && out_len == tests[i].plain.length);
		print_hex_data(test_out, out_len);
		printf("plain_len = %u, out_len = %td\n", tests[i].plain.length, out_len);
		puts(pass ? pass_str : fail_str);
		count_test_result(pass, result);
	}
}

static void test_lzsa2(test_result_t *result) {
	ptrdiff_t out_len;
	bool pass;

	for(size_t i = 0; i < TESTS_COUNT; i++) {
		printf("%s %02u:\n", test_str, i + 1);

		memset(test_out, '\0', sizeof(test_out));
		puts("lzsa2_decompress_block_ref()");
		out_len = lzsa2_decompress_block_ref(test_out, tests[i].lzsa2.data) - test_out;
		pass = (memcmp(test_out, tests[i].plain.data, tests[i].plain.length) == 0 && out_len == tests[i].plain.length);
		print_hex_data(test_out, out_len);
		printf("plain_len = %u, out_len = %td\n", tests[i].plain.length, out_len);
		puts(pass ? pass_str : fail_str);
		count_test_result(pass, result);

		memset(test_out, '\0', sizeof(test_out));
		puts("lzsa2_decompress_block()");
		out_len = lzsa2_decompress_block(test_out, tests[i].lzsa2.data) - test_out;
		pass = (memcmp(test_out, tests[i].plain.data, tests[i].plain.length) == 0 && out_len == tests[i].plain.length);
		print_hex_data(test_out, out_len);
		printf("plain_len = %u, out_len = %td\n", tests[i].plain.length, out_len);
		puts(pass ? pass_str : fail_str);
		count_test_result(pass, result);
	}
}

static void benchmark_lzsa1(void) {
	benchmark("lzsa1_decompress_block_ref", 100, lzsa1_decompress_block_ref(test_out, tests[10].lzsa1.data));
	benchmark("lzsa1_decompress_block", 100, lzsa1_decompress_block(test_out, tests[10].lzsa1.data));
}

static void benchmark_lzsa2(void) {
	benchmark("lzsa2_decompress_block_ref", 100, lzsa2_decompress_block_ref(test_out, tests[10].lzsa2.data));
	benchmark("lzsa2_decompress_block", 100, lzsa2_decompress_block(test_out, tests[10].lzsa2.data));
}

void main(void) {
	test_result_t results = { 0, 0 };

	CLK_CKDIVR = 0;
	PC_DDR = (1 << PC_DDR_DDR5);
	PC_CR1 = (1 << PC_CR1_C15);

	if(ucsim_if_detect()) {
		uart_init(UART_BAUD_115200, ucsim_if_putchar, NULL);
	} else {
		uart_init(UART_BAUD_115200, uart_putchar, uart_getchar);
	}

	puts(hrule_str);

	test_lzsa1(&results);
	test_lzsa2(&results);

	printf("TOTAL RESULTS: passed = %u, failed = %u\n", results.pass_count, results.fail_count);

	puts(hrule_str);

	if(results.fail_count == 0) {
		benchmark_lzsa1();
		benchmark_lzsa2();
	} else {
		puts("One or more tests failed, skipping benchmark");
	}

	puts(hrule_str);

	while(1);
}
