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
	static const struct {
		size_t length_plain;
		uint8_t data_plain[600];
		size_t length_compressed;
		uint8_t data_compressed[600];
	} in[] = {
		{
			.length_plain = 51,
			.data_plain = {
				0x48, 0x65, 0x6C, 0x6C, 0x6F, 0x2C, 0x20, 0x68, 0x65, 0x6C, 0x6C, 0x6F, 0x2C, 0x20, 0x69, 0x73,
				0x20, 0x74, 0x68, 0x69, 0x73, 0x20, 0x74, 0x68, 0x69, 0x6E, 0x67, 0x20, 0x6F, 0x6E, 0x3F, 0x20,
				0x42, 0x6C, 0x61, 0x68, 0x2C, 0x20, 0x62, 0x6C, 0x61, 0x68, 0x2C, 0x20, 0x62, 0x6C, 0x61, 0x68,
				0x2E, 0x2E, 0x2E
			},
			.length_compressed = 43,
			.data_compressed = {
				0x73, 0x01, 0x48, 0x65, 0x6C, 0x6C, 0x6F, 0x2C, 0x20, 0x68, 0xF9, 0x53, 0x69, 0x73, 0x20, 0x74,
				0x68, 0xFB, 0x76, 0x07, 0x6E, 0x67, 0x20, 0x6F, 0x6E, 0x3F, 0x20, 0x42, 0x6C, 0x61, 0x68, 0x2C,
				0x20, 0x62, 0xFA, 0x3F, 0x2E, 0x2E, 0x2E, 0x00, 0xEE, 0x00, 0x00
			}
		},
		{
			.length_plain = 229,
			.data_plain = {
				0x46, 0x6F, 0x72, 0x20, 0x6D, 0x65, 0x20, 0x69, 0x74, 0x20, 0x77, 0x61, 0x73, 0x20, 0x61, 0x63,
				0x74, 0x75, 0x61, 0x6C, 0x6C, 0x79, 0x20, 0x61, 0x20, 0x72, 0x65, 0x6C, 0x69, 0x65, 0x66, 0x20,
				0x74, 0x6F, 0x20, 0x73, 0x65, 0x65, 0x20, 0x74, 0x68, 0x61, 0x74, 0x20, 0x6E, 0x6F, 0x74, 0x20,
				0x65, 0x76, 0x65, 0x72, 0x79, 0x74, 0x68, 0x69, 0x6E, 0x67, 0x20, 0x69, 0x73, 0x20, 0x62, 0x65,
				0x69, 0x6E, 0x67, 0x20, 0x6F, 0x76, 0x65, 0x72, 0x2D, 0x65, 0x78, 0x70, 0x6C, 0x61, 0x69, 0x6E,
				0x65, 0x64, 0x2C, 0x20, 0x61, 0x20, 0x73, 0x69, 0x63, 0x6B, 0x6E, 0x65, 0x73, 0x73, 0x20, 0x6D,
				0x61, 0x6E, 0x79, 0x20, 0x6D, 0x6F, 0x64, 0x65, 0x72, 0x6E, 0x20, 0x6D, 0x6F, 0x76, 0x69, 0x65,
				0x73, 0x20, 0x73, 0x75, 0x66, 0x66, 0x65, 0x72, 0x20, 0x66, 0x72, 0x6F, 0x6D, 0x2E, 0x20, 0x57,
				0x68, 0x65, 0x72, 0x65, 0x20, 0x69, 0x73, 0x20, 0x74, 0x68, 0x65, 0x20, 0x66, 0x75, 0x6E, 0x20,
				0x69, 0x66, 0x20, 0x61, 0x66, 0x74, 0x65, 0x72, 0x20, 0x74, 0x68, 0x65, 0x20, 0x6D, 0x6F, 0x76,
				0x69, 0x65, 0x20, 0x79, 0x6F, 0x75, 0x20, 0x64, 0x6F, 0x6E, 0x27, 0x74, 0x20, 0x74, 0x61, 0x6C,
				0x6B, 0x20, 0x61, 0x62, 0x6F, 0x75, 0x74, 0x20, 0x69, 0x74, 0x2C, 0x20, 0x6C, 0x6F, 0x6F, 0x6B,
				0x20, 0x74, 0x68, 0x69, 0x6E, 0x67, 0x73, 0x20, 0x75, 0x70, 0x2C, 0x20, 0x6D, 0x61, 0x79, 0x62,
				0x65, 0x20, 0x65, 0x76, 0x65, 0x6E, 0x20, 0x72, 0x65, 0x61, 0x64, 0x20, 0x74, 0x68, 0x65, 0x20,
				0x62, 0x6F, 0x6F, 0x6B, 0x3F
			},
			.length_compressed = 216,
			.data_compressed = {
				0x71, 0x39, 0x46, 0x6F, 0x72, 0x20, 0x6D, 0x65, 0x20, 0x69, 0x74, 0x20, 0x77, 0x61, 0x73, 0x20,
				0x61, 0x63, 0x74, 0x75, 0x61, 0x6C, 0x6C, 0x79, 0x20, 0x61, 0x20, 0x72, 0x65, 0x6C, 0x69, 0x65,
				0x66, 0x20, 0x74, 0x6F, 0x20, 0x73, 0x65, 0x65, 0x20, 0x74, 0x68, 0x61, 0x74, 0x20, 0x6E, 0x6F,
				0x74, 0x20, 0x65, 0x76, 0x65, 0x72, 0x79, 0x74, 0x68, 0x69, 0x6E, 0x67, 0x20, 0x69, 0x73, 0x20,
				0x62, 0x65, 0xF7, 0x10, 0x6F, 0xEC, 0x71, 0x35, 0x2D, 0x65, 0x78, 0x70, 0x6C, 0x61, 0x69, 0x6E,
				0x65, 0x64, 0x2C, 0x20, 0x61, 0x20, 0x73, 0x69, 0x63, 0x6B, 0x6E, 0x65, 0x73, 0x73, 0x20, 0x6D,
				0x61, 0x6E, 0x79, 0x20, 0x6D, 0x6F, 0x64, 0x65, 0x72, 0x6E, 0x20, 0x6D, 0x6F, 0x76, 0x69, 0x65,
				0x73, 0x20, 0x73, 0x75, 0x66, 0x66, 0x65, 0x72, 0x20, 0x66, 0x72, 0x6F, 0x6D, 0x2E, 0x20, 0x57,
				0x68, 0x65, 0x72, 0x65, 0xB6, 0x71, 0x09, 0x74, 0x68, 0x65, 0x20, 0x66, 0x75, 0x6E, 0x20, 0x69,
				0x66, 0x20, 0x61, 0x66, 0x74, 0x65, 0x72, 0xEF, 0x03, 0xCE, 0x72, 0x18, 0x20, 0x79, 0x6F, 0x75,
				0x20, 0x64, 0x6F, 0x6E, 0x27, 0x74, 0x20, 0x74, 0x61, 0x6C, 0x6B, 0x20, 0x61, 0x62, 0x6F, 0x75,
				0x74, 0x20, 0x69, 0x74, 0x2C, 0x20, 0x6C, 0x6F, 0x6F, 0x6B, 0x20, 0x74, 0x50, 0x73, 0x20, 0x75,
				0x70, 0x2C, 0x93, 0x31, 0x79, 0x62, 0x65, 0x5E, 0x10, 0x6E, 0x42, 0x22, 0x61, 0x64, 0xBD, 0x10,
				0x62, 0xDC, 0x1F, 0x3F, 0x00, 0xEE, 0x00, 0x00
			}
		},
		{
			.length_plain = 185,
			.data_plain = {
				0x54, 0x68, 0x65, 0x20, 0x61, 0x63, 0x74, 0x75, 0x61, 0x6C, 0x20, 0x64, 0x72, 0x69, 0x76, 0x65,
				0x20, 0x63, 0x61, 0x70, 0x61, 0x62, 0x69, 0x6C, 0x69, 0x74, 0x69, 0x65, 0x73, 0x20, 0x6F, 0x66,
				0x20, 0x49, 0x53, 0x41, 0x20, 0x6D, 0x6F, 0x74, 0x68, 0x65, 0x72, 0x62, 0x6F, 0x61, 0x72, 0x64,
				0x73, 0x20, 0x63, 0x61, 0x6E, 0x20, 0x76, 0x61, 0x72, 0x79, 0x20, 0x67, 0x72, 0x65, 0x61, 0x74,
				0x6C, 0x79, 0x2E, 0x0D, 0x0A, 0x54, 0x68, 0x65, 0x20, 0x49, 0x45, 0x45, 0x45, 0x20, 0x50, 0x39,
				0x39, 0x36, 0x20, 0x73, 0x70, 0x65, 0x63, 0x73, 0x20, 0x31, 0x2E, 0x30, 0x20, 0x6F, 0x66, 0x66,
				0x65, 0x72, 0x73, 0x20, 0x74, 0x68, 0x65, 0x73, 0x65, 0x20, 0x67, 0x75, 0x69, 0x64, 0x65, 0x6C,
				0x69, 0x6E, 0x65, 0x73, 0x3A, 0x0D, 0x0A, 0x20, 0x20, 0x20, 0x2B, 0x31, 0x32, 0x56, 0x20, 0x61,
				0x74, 0x20, 0x31, 0x2E, 0x35, 0x41, 0x0D, 0x0A, 0x20, 0x20, 0x20, 0x2D, 0x31, 0x32, 0x56, 0x20,
				0x61, 0x74, 0x20, 0x30, 0x2E, 0x33, 0x41, 0x0D, 0x0A, 0x20, 0x20, 0x20, 0x20, 0x2B, 0x35, 0x56,
				0x20, 0x61, 0x74, 0x20, 0x34, 0x2E, 0x35, 0x41, 0x0D, 0x0A, 0x20, 0x20, 0x20, 0x20, 0x2D, 0x35,
				0x56, 0x20, 0x61, 0x74, 0x20, 0x30, 0x2E, 0x32, 0x41
			},
			.length_compressed = 162,
			.data_compressed = {
				0x71, 0x3E, 0x54, 0x68, 0x65, 0x20, 0x61, 0x63, 0x74, 0x75, 0x61, 0x6C, 0x20, 0x64, 0x72, 0x69,
				0x76, 0x65, 0x20, 0x63, 0x61, 0x70, 0x61, 0x62, 0x69, 0x6C, 0x69, 0x74, 0x69, 0x65, 0x73, 0x20,
				0x6F, 0x66, 0x20, 0x49, 0x53, 0x41, 0x20, 0x6D, 0x6F, 0x74, 0x68, 0x65, 0x72, 0x62, 0x6F, 0x61,
				0x72, 0x64, 0x73, 0x20, 0x63, 0x61, 0x6E, 0x20, 0x76, 0x61, 0x72, 0x79, 0x20, 0x67, 0x72, 0x65,
				0x61, 0x74, 0x6C, 0x79, 0x2E, 0x0D, 0x0A, 0xBB, 0x70, 0x0C, 0x49, 0x45, 0x45, 0x45, 0x20, 0x50,
				0x39, 0x39, 0x36, 0x20, 0x73, 0x70, 0x65, 0x63, 0x73, 0x20, 0x31, 0x2E, 0x30, 0xC1, 0x50, 0x66,
				0x65, 0x72, 0x73, 0x20, 0xC3, 0x70, 0x13, 0x73, 0x65, 0x20, 0x67, 0x75, 0x69, 0x64, 0x65, 0x6C,
				0x69, 0x6E, 0x65, 0x73, 0x3A, 0x0D, 0x0A, 0x20, 0x20, 0x20, 0x2B, 0x31, 0x32, 0x56, 0x20, 0x61,
				0x74, 0xD7, 0x22, 0x35, 0x41, 0xEF, 0x14, 0x2D, 0xEF, 0x33, 0x30, 0x2E, 0x33, 0xEF, 0x32, 0x20,
				0x2B, 0x35, 0xEF, 0x15, 0x34, 0xDE, 0x34, 0x20, 0x2D, 0x35, 0xDE, 0x2F, 0x32, 0x41, 0x00, 0xEE,
				0x00, 0x00
			}
		},
		{
			.length_plain = 240,
			.data_plain = {
				0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41,
				0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41,
				0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41,
				0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41,
				0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41,
				0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41,
				0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41,
				0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42,
				0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42,
				0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42,
				0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42,
				0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42,
				0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42,
				0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42,
				0x43, 0x43, 0x43, 0x43, 0x43, 0x43, 0x43, 0x43, 0x43, 0x43, 0x43, 0x43, 0x43, 0x43, 0x43, 0x43
			},
			.length_compressed = 16,
			.data_compressed = {
				0x1F, 0x41, 0xFF, 0x5D, 0x1F, 0x42, 0xFF, 0x5D, 0x1C, 0x43, 0xFF, 0x0F, 0x00, 0xEE, 0x00, 0x00
			}
		},
		{
			.length_plain = 192,
			.data_plain = {
				0x4A, 0x35, 0x72, 0x38, 0x4B, 0x41, 0x44, 0x42, 0x31, 0x53, 0x5A, 0x49, 0x79, 0x35, 0x70, 0x4E,
				0x44, 0x69, 0x53, 0x52, 0x6A, 0x4A, 0x4C, 0x43, 0x6D, 0x58, 0x44, 0x35, 0x6E, 0x4A, 0x47, 0x35,
				0x5A, 0x65, 0x62, 0x76, 0x70, 0x58, 0x51, 0x70, 0x37, 0x67, 0x63, 0x72, 0x6A, 0x6D, 0x69, 0x31,
				0x48, 0x6B, 0x49, 0x4E, 0x30, 0x55, 0x34, 0x73, 0x37, 0x78, 0x41, 0x55, 0x59, 0x66, 0x30, 0x34,
				0x6A, 0x66, 0x63, 0x66, 0x58, 0x6A, 0x61, 0x68, 0x32, 0x52, 0x6E, 0x37, 0x4D, 0x5A, 0x48, 0x42,
				0x45, 0x69, 0x39, 0x68, 0x4C, 0x57, 0x61, 0x43, 0x56, 0x71, 0x79, 0x44, 0x34, 0x59, 0x4D, 0x43,
				0x4C, 0x33, 0x56, 0x42, 0x6E, 0x71, 0x68, 0x4C, 0x64, 0x53, 0x42, 0x49, 0x32, 0x76, 0x74, 0x6F,
				0x45, 0x56, 0x33, 0x55, 0x39, 0x6A, 0x58, 0x71, 0x52, 0x65, 0x4F, 0x65, 0x75, 0x4D, 0x4A, 0x33,
				0x30, 0x61, 0x70, 0x51, 0x41, 0x61, 0x6F, 0x46, 0x36, 0x4A, 0x4E, 0x30, 0x51, 0x6D, 0x62, 0x39,
				0x32, 0x4D, 0x50, 0x4B, 0x4A, 0x6B, 0x69, 0x75, 0x62, 0x46, 0x65, 0x4E, 0x58, 0x66, 0x70, 0x64,
				0x6E, 0x34, 0x78, 0x63, 0x71, 0x6A, 0x72, 0x38, 0x72, 0x30, 0x30, 0x49, 0x79, 0x34, 0x56, 0x36,
				0x65, 0x45, 0x64, 0x4D, 0x47, 0x4B, 0x4E, 0x4F, 0x56, 0x42, 0x4D, 0x4D, 0x70, 0x63, 0x6F, 0x64
			},
			.length_compressed = 198,
			.data_compressed = {
				0x7F, 0xB9, 0x4A, 0x35, 0x72, 0x38, 0x4B, 0x41, 0x44, 0x42, 0x31, 0x53, 0x5A, 0x49, 0x79, 0x35,
				0x70, 0x4E, 0x44, 0x69, 0x53, 0x52, 0x6A, 0x4A, 0x4C, 0x43, 0x6D, 0x58, 0x44, 0x35, 0x6E, 0x4A,
				0x47, 0x35, 0x5A, 0x65, 0x62, 0x76, 0x70, 0x58, 0x51, 0x70, 0x37, 0x67, 0x63, 0x72, 0x6A, 0x6D,
				0x69, 0x31, 0x48, 0x6B, 0x49, 0x4E, 0x30, 0x55, 0x34, 0x73, 0x37, 0x78, 0x41, 0x55, 0x59, 0x66,
				0x30, 0x34, 0x6A, 0x66, 0x63, 0x66, 0x58, 0x6A, 0x61, 0x68, 0x32, 0x52, 0x6E, 0x37, 0x4D, 0x5A,
				0x48, 0x42, 0x45, 0x69, 0x39, 0x68, 0x4C, 0x57, 0x61, 0x43, 0x56, 0x71, 0x79, 0x44, 0x34, 0x59,
				0x4D, 0x43, 0x4C, 0x33, 0x56, 0x42, 0x6E, 0x71, 0x68, 0x4C, 0x64, 0x53, 0x42, 0x49, 0x32, 0x76,
				0x74, 0x6F, 0x45, 0x56, 0x33, 0x55, 0x39, 0x6A, 0x58, 0x71, 0x52, 0x65, 0x4F, 0x65, 0x75, 0x4D,
				0x4A, 0x33, 0x30, 0x61, 0x70, 0x51, 0x41, 0x61, 0x6F, 0x46, 0x36, 0x4A, 0x4E, 0x30, 0x51, 0x6D,
				0x62, 0x39, 0x32, 0x4D, 0x50, 0x4B, 0x4A, 0x6B, 0x69, 0x75, 0x62, 0x46, 0x65, 0x4E, 0x58, 0x66,
				0x70, 0x64, 0x6E, 0x34, 0x78, 0x63, 0x71, 0x6A, 0x72, 0x38, 0x72, 0x30, 0x30, 0x49, 0x79, 0x34,
				0x56, 0x36, 0x65, 0x45, 0x64, 0x4D, 0x47, 0x4B, 0x4E, 0x4F, 0x56, 0x42, 0x4D, 0x4D, 0x70, 0x63,
				0x6F, 0x64, 0x00, 0xEE, 0x00, 0x00
			}
		},
		{
			// This is a test case consisting of a single literal of length between
			// 256 and 511 (i.e. length denoted by single byte + 256).
			.length_plain = 304,
			.data_plain = {
				0x31, 0x69, 0x6a, 0x5a, 0x55, 0x63, 0x36, 0x32, 0x69, 0x67, 0x64, 0x56,
				0x6e, 0x67, 0x6f, 0x75, 0x64, 0x37, 0x64, 0x4b, 0x47, 0x76, 0x39, 0x36,
				0x6e, 0x55, 0x37, 0x34, 0x35, 0x37, 0x62, 0x4e, 0x4f, 0x56, 0x74, 0x42,
				0x67, 0x7a, 0x4a, 0x62, 0x70, 0x65, 0x6c, 0x4e, 0x43, 0x6b, 0x78, 0x72,
				0x55, 0x75, 0x36, 0x6f, 0x58, 0x61, 0x42, 0x74, 0x43, 0x4d, 0x42, 0x39,
				0x74, 0x43, 0x43, 0x67, 0x36, 0x4e, 0x78, 0x4c, 0x71, 0x53, 0x41, 0x68,
				0x49, 0x76, 0x78, 0x69, 0x58, 0x68, 0x45, 0x53, 0x73, 0x7a, 0x34, 0x62,
				0x57, 0x36, 0x6e, 0x79, 0x4a, 0x53, 0x43, 0x6c, 0x75, 0x53, 0x32, 0x6e,
				0x56, 0x4c, 0x72, 0x31, 0x34, 0x6b, 0x4c, 0x4e, 0x54, 0x7a, 0x58, 0x32,
				0x5a, 0x59, 0x69, 0x6c, 0x59, 0x46, 0x61, 0x4a, 0x61, 0x55, 0x4d, 0x75,
				0x50, 0x4c, 0x45, 0x78, 0x77, 0x43, 0x6d, 0x39, 0x75, 0x66, 0x56, 0x71,
				0x74, 0x43, 0x67, 0x51, 0x46, 0x55, 0x37, 0x49, 0x38, 0x65, 0x69, 0x69,
				0x6b, 0x65, 0x34, 0x52, 0x38, 0x46, 0x57, 0x4a, 0x4f, 0x6f, 0x7a, 0x65,
				0x64, 0x50, 0x75, 0x33, 0x59, 0x54, 0x6f, 0x33, 0x67, 0x65, 0x42, 0x4a,
				0x78, 0x4e, 0x32, 0x47, 0x47, 0x5a, 0x6b, 0x65, 0x4b, 0x79, 0x65, 0x52,
				0x34, 0x78, 0x6a, 0x68, 0x72, 0x77, 0x36, 0x69, 0x36, 0x66, 0x6e, 0x6a,
				0x68, 0x4e, 0x34, 0x76, 0x64, 0x45, 0x69, 0x6d, 0x45, 0x4b, 0x76, 0x36,
				0x51, 0x54, 0x78, 0x79, 0x4f, 0x36, 0x6f, 0x75, 0x68, 0x49, 0x41, 0x6f,
				0x39, 0x7a, 0x41, 0x31, 0x7a, 0x70, 0x49, 0x43, 0x57, 0x62, 0x78, 0x56,
				0x6b, 0x52, 0x4d, 0x58, 0x35, 0x50, 0x32, 0x4e, 0x32, 0x4f, 0x36, 0x77,
				0x56, 0x73, 0x39, 0x6f, 0x71, 0x47, 0x4d, 0x38, 0x6c, 0x52, 0x41, 0x6e,
				0x4e, 0x4d, 0x54, 0x51, 0x63, 0x62, 0x53, 0x36, 0x34, 0x34, 0x54, 0x76,
				0x49, 0x41, 0x30, 0x42, 0x57, 0x45, 0x31, 0x64, 0x33, 0x52, 0x59, 0x58,
				0x4f, 0x50, 0x67, 0x6c, 0x52, 0x66, 0x4d, 0x47, 0x70, 0x34, 0x4d, 0x72,
				0x6f, 0x4d, 0x44, 0x65, 0x33, 0x37, 0x6e, 0x5a, 0x51, 0x57, 0x54, 0x31,
				0x4f, 0x43, 0x61, 0x65
			},
			.length_compressed = 311,
			.data_compressed = {
				0x7f, 0xfa, 0x30, 0x31, 0x69, 0x6a, 0x5a, 0x55, 0x63, 0x36, 0x32, 0x69,
				0x67, 0x64, 0x56, 0x6e, 0x67, 0x6f, 0x75, 0x64, 0x37, 0x64, 0x4b, 0x47,
				0x76, 0x39, 0x36, 0x6e, 0x55, 0x37, 0x34, 0x35, 0x37, 0x62, 0x4e, 0x4f,
				0x56, 0x74, 0x42, 0x67, 0x7a, 0x4a, 0x62, 0x70, 0x65, 0x6c, 0x4e, 0x43,
				0x6b, 0x78, 0x72, 0x55, 0x75, 0x36, 0x6f, 0x58, 0x61, 0x42, 0x74, 0x43,
				0x4d, 0x42, 0x39, 0x74, 0x43, 0x43, 0x67, 0x36, 0x4e, 0x78, 0x4c, 0x71,
				0x53, 0x41, 0x68, 0x49, 0x76, 0x78, 0x69, 0x58, 0x68, 0x45, 0x53, 0x73,
				0x7a, 0x34, 0x62, 0x57, 0x36, 0x6e, 0x79, 0x4a, 0x53, 0x43, 0x6c, 0x75,
				0x53, 0x32, 0x6e, 0x56, 0x4c, 0x72, 0x31, 0x34, 0x6b, 0x4c, 0x4e, 0x54,
				0x7a, 0x58, 0x32, 0x5a, 0x59, 0x69, 0x6c, 0x59, 0x46, 0x61, 0x4a, 0x61,
				0x55, 0x4d, 0x75, 0x50, 0x4c, 0x45, 0x78, 0x77, 0x43, 0x6d, 0x39, 0x75,
				0x66, 0x56, 0x71, 0x74, 0x43, 0x67, 0x51, 0x46, 0x55, 0x37, 0x49, 0x38,
				0x65, 0x69, 0x69, 0x6b, 0x65, 0x34, 0x52, 0x38, 0x46, 0x57, 0x4a, 0x4f,
				0x6f, 0x7a, 0x65, 0x64, 0x50, 0x75, 0x33, 0x59, 0x54, 0x6f, 0x33, 0x67,
				0x65, 0x42, 0x4a, 0x78, 0x4e, 0x32, 0x47, 0x47, 0x5a, 0x6b, 0x65, 0x4b,
				0x79, 0x65, 0x52, 0x34, 0x78, 0x6a, 0x68, 0x72, 0x77, 0x36, 0x69, 0x36,
				0x66, 0x6e, 0x6a, 0x68, 0x4e, 0x34, 0x76, 0x64, 0x45, 0x69, 0x6d, 0x45,
				0x4b, 0x76, 0x36, 0x51, 0x54, 0x78, 0x79, 0x4f, 0x36, 0x6f, 0x75, 0x68,
				0x49, 0x41, 0x6f, 0x39, 0x7a, 0x41, 0x31, 0x7a, 0x70, 0x49, 0x43, 0x57,
				0x62, 0x78, 0x56, 0x6b, 0x52, 0x4d, 0x58, 0x35, 0x50, 0x32, 0x4e, 0x32,
				0x4f, 0x36, 0x77, 0x56, 0x73, 0x39, 0x6f, 0x71, 0x47, 0x4d, 0x38, 0x6c,
				0x52, 0x41, 0x6e, 0x4e, 0x4d, 0x54, 0x51, 0x63, 0x62, 0x53, 0x36, 0x34,
				0x34, 0x54, 0x76, 0x49, 0x41, 0x30, 0x42, 0x57, 0x45, 0x31, 0x64, 0x33,
				0x52, 0x59, 0x58, 0x4f, 0x50, 0x67, 0x6c, 0x52, 0x66, 0x4d, 0x47, 0x70,
				0x34, 0x4d, 0x72, 0x6f, 0x4d, 0x44, 0x65, 0x33, 0x37, 0x6e, 0x5a, 0x51,
				0x57, 0x54, 0x31, 0x4f, 0x43, 0x61, 0x65, 0x00, 0xee, 0x00, 0x00
			}
		},
		{
			// This is a test case consisting of a single literal of length greater
			// than 512 (i.e. length denoted by two bytes).
			.length_plain = 560,
			.data_plain = {
				0x31, 0x69, 0x6a, 0x5a, 0x55, 0x63, 0x36, 0x32, 0x69, 0x67, 0x64, 0x56,
				0x6e, 0x67, 0x6f, 0x75, 0x64, 0x37, 0x64, 0x4b, 0x47, 0x76, 0x39, 0x36,
				0x6e, 0x55, 0x37, 0x34, 0x35, 0x37, 0x62, 0x4e, 0x4f, 0x56, 0x74, 0x42,
				0x67, 0x7a, 0x4a, 0x62, 0x70, 0x65, 0x6c, 0x4e, 0x43, 0x6b, 0x78, 0x72,
				0x55, 0x75, 0x36, 0x6f, 0x58, 0x61, 0x42, 0x74, 0x43, 0x4d, 0x42, 0x39,
				0x74, 0x43, 0x43, 0x67, 0x36, 0x4e, 0x78, 0x4c, 0x71, 0x53, 0x41, 0x68,
				0x49, 0x76, 0x78, 0x69, 0x58, 0x68, 0x45, 0x53, 0x73, 0x7a, 0x34, 0x62,
				0x57, 0x36, 0x6e, 0x79, 0x4a, 0x53, 0x43, 0x6c, 0x75, 0x53, 0x32, 0x6e,
				0x56, 0x4c, 0x72, 0x31, 0x34, 0x6b, 0x4c, 0x4e, 0x54, 0x7a, 0x58, 0x32,
				0x5a, 0x59, 0x69, 0x6c, 0x59, 0x46, 0x61, 0x4a, 0x61, 0x55, 0x4d, 0x75,
				0x50, 0x4c, 0x45, 0x78, 0x77, 0x43, 0x6d, 0x39, 0x75, 0x66, 0x56, 0x71,
				0x74, 0x43, 0x67, 0x51, 0x46, 0x55, 0x37, 0x49, 0x38, 0x65, 0x69, 0x69,
				0x6b, 0x65, 0x34, 0x52, 0x38, 0x46, 0x57, 0x4a, 0x4f, 0x6f, 0x7a, 0x65,
				0x64, 0x50, 0x75, 0x33, 0x59, 0x54, 0x6f, 0x33, 0x67, 0x65, 0x42, 0x4a,
				0x78, 0x4e, 0x32, 0x47, 0x47, 0x5a, 0x6b, 0x65, 0x4b, 0x79, 0x65, 0x52,
				0x34, 0x78, 0x6a, 0x68, 0x72, 0x77, 0x36, 0x69, 0x36, 0x66, 0x6e, 0x6a,
				0x68, 0x4e, 0x34, 0x76, 0x64, 0x45, 0x69, 0x6d, 0x45, 0x4b, 0x76, 0x36,
				0x51, 0x54, 0x78, 0x79, 0x4f, 0x36, 0x6f, 0x75, 0x68, 0x49, 0x41, 0x6f,
				0x39, 0x7a, 0x41, 0x31, 0x7a, 0x70, 0x49, 0x43, 0x57, 0x62, 0x78, 0x56,
				0x6b, 0x52, 0x4d, 0x58, 0x35, 0x50, 0x32, 0x4e, 0x32, 0x4f, 0x36, 0x77,
				0x56, 0x73, 0x39, 0x6f, 0x71, 0x47, 0x4d, 0x38, 0x6c, 0x52, 0x41, 0x6e,
				0x4e, 0x4d, 0x54, 0x51, 0x63, 0x62, 0x53, 0x36, 0x34, 0x34, 0x54, 0x76,
				0x49, 0x41, 0x30, 0x42, 0x57, 0x45, 0x31, 0x64, 0x33, 0x52, 0x59, 0x58,
				0x4f, 0x50, 0x67, 0x6c, 0x52, 0x66, 0x4d, 0x47, 0x70, 0x34, 0x4d, 0x72,
				0x6f, 0x4d, 0x44, 0x65, 0x33, 0x37, 0x6e, 0x5a, 0x51, 0x57, 0x54, 0x31,
				0x4f, 0x43, 0x61, 0x65, 0x4a, 0x43, 0x69, 0x65, 0x45, 0x6a, 0x53, 0x78,
				0x49, 0x6f, 0x4e, 0x4d, 0x6c, 0x70, 0x51, 0x72, 0x54, 0x4e, 0x6d, 0x48,
				0x7a, 0x49, 0x44, 0x70, 0x6a, 0x45, 0x73, 0x49, 0x73, 0x48, 0x6b, 0x66,
				0x36, 0x65, 0x6e, 0x35, 0x4d, 0x48, 0x6d, 0x65, 0x72, 0x59, 0x79, 0x6c,
				0x42, 0x52, 0x41, 0x76, 0x71, 0x45, 0x48, 0x52, 0x71, 0x4c, 0x66, 0x41,
				0x46, 0x56, 0x67, 0x6c, 0x41, 0x6e, 0x33, 0x4e, 0x47, 0x6f, 0x68, 0x35,
				0x38, 0x68, 0x31, 0x61, 0x30, 0x5a, 0x64, 0x73, 0x4d, 0x6d, 0x65, 0x58,
				0x64, 0x68, 0x6c, 0x6d, 0x74, 0x46, 0x32, 0x4d, 0x44, 0x47, 0x45, 0x41,
				0x45, 0x70, 0x74, 0x56, 0x42, 0x67, 0x6d, 0x6b, 0x75, 0x6e, 0x62, 0x61,
				0x36, 0x36, 0x5a, 0x32, 0x39, 0x49, 0x55, 0x55, 0x50, 0x69, 0x62, 0x72,
				0x33, 0x36, 0x51, 0x30, 0x49, 0x61, 0x36, 0x39, 0x37, 0x5a, 0x69, 0x44,
				0x37, 0x63, 0x7a, 0x47, 0x61, 0x37, 0x41, 0x73, 0x77, 0x55, 0x42, 0x42,
				0x64, 0x50, 0x76, 0x44, 0x39, 0x31, 0x78, 0x47, 0x32, 0x6b, 0x56, 0x75,
				0x57, 0x58, 0x75, 0x31, 0x59, 0x6d, 0x67, 0x61, 0x46, 0x78, 0x4d, 0x42,
				0x35, 0x6a, 0x37, 0x78, 0x4c, 0x39, 0x51, 0x5a, 0x4d, 0x73, 0x59, 0x4c,
				0x42, 0x54, 0x44, 0x48, 0x52, 0x67, 0x38, 0x77, 0x76, 0x78, 0x45, 0x70,
				0x48, 0x6e, 0x5a, 0x43, 0x74, 0x4e, 0x56, 0x43, 0x41, 0x74, 0x45, 0x6e,
				0x47, 0x4a, 0x46, 0x6d, 0x32, 0x30, 0x56, 0x45, 0x31, 0x30, 0x73, 0x6b,
				0x6b, 0x43, 0x36, 0x46, 0x37, 0x70, 0x69, 0x46, 0x43, 0x6c, 0x53, 0x31,
				0x55, 0x77, 0x36, 0x73, 0x4a, 0x50, 0x76, 0x6a, 0x52, 0x72, 0x78, 0x69,
				0x63, 0x68, 0x56, 0x5a, 0x7a, 0x68, 0x33, 0x6b, 0x55, 0x53, 0x54, 0x4c,
				0x45, 0x33, 0x44, 0x32, 0x33, 0x45, 0x71, 0x54
			},
			.length_compressed = 568,
			.data_compressed = {
				0x7f, 0xf9, 0x30, 0x02, 0x31, 0x69, 0x6a, 0x5a, 0x55, 0x63, 0x36, 0x32,
				0x69, 0x67, 0x64, 0x56, 0x6e, 0x67, 0x6f, 0x75, 0x64, 0x37, 0x64, 0x4b,
				0x47, 0x76, 0x39, 0x36, 0x6e, 0x55, 0x37, 0x34, 0x35, 0x37, 0x62, 0x4e,
				0x4f, 0x56, 0x74, 0x42, 0x67, 0x7a, 0x4a, 0x62, 0x70, 0x65, 0x6c, 0x4e,
				0x43, 0x6b, 0x78, 0x72, 0x55, 0x75, 0x36, 0x6f, 0x58, 0x61, 0x42, 0x74,
				0x43, 0x4d, 0x42, 0x39, 0x74, 0x43, 0x43, 0x67, 0x36, 0x4e, 0x78, 0x4c,
				0x71, 0x53, 0x41, 0x68, 0x49, 0x76, 0x78, 0x69, 0x58, 0x68, 0x45, 0x53,
				0x73, 0x7a, 0x34, 0x62, 0x57, 0x36, 0x6e, 0x79, 0x4a, 0x53, 0x43, 0x6c,
				0x75, 0x53, 0x32, 0x6e, 0x56, 0x4c, 0x72, 0x31, 0x34, 0x6b, 0x4c, 0x4e,
				0x54, 0x7a, 0x58, 0x32, 0x5a, 0x59, 0x69, 0x6c, 0x59, 0x46, 0x61, 0x4a,
				0x61, 0x55, 0x4d, 0x75, 0x50, 0x4c, 0x45, 0x78, 0x77, 0x43, 0x6d, 0x39,
				0x75, 0x66, 0x56, 0x71, 0x74, 0x43, 0x67, 0x51, 0x46, 0x55, 0x37, 0x49,
				0x38, 0x65, 0x69, 0x69, 0x6b, 0x65, 0x34, 0x52, 0x38, 0x46, 0x57, 0x4a,
				0x4f, 0x6f, 0x7a, 0x65, 0x64, 0x50, 0x75, 0x33, 0x59, 0x54, 0x6f, 0x33,
				0x67, 0x65, 0x42, 0x4a, 0x78, 0x4e, 0x32, 0x47, 0x47, 0x5a, 0x6b, 0x65,
				0x4b, 0x79, 0x65, 0x52, 0x34, 0x78, 0x6a, 0x68, 0x72, 0x77, 0x36, 0x69,
				0x36, 0x66, 0x6e, 0x6a, 0x68, 0x4e, 0x34, 0x76, 0x64, 0x45, 0x69, 0x6d,
				0x45, 0x4b, 0x76, 0x36, 0x51, 0x54, 0x78, 0x79, 0x4f, 0x36, 0x6f, 0x75,
				0x68, 0x49, 0x41, 0x6f, 0x39, 0x7a, 0x41, 0x31, 0x7a, 0x70, 0x49, 0x43,
				0x57, 0x62, 0x78, 0x56, 0x6b, 0x52, 0x4d, 0x58, 0x35, 0x50, 0x32, 0x4e,
				0x32, 0x4f, 0x36, 0x77, 0x56, 0x73, 0x39, 0x6f, 0x71, 0x47, 0x4d, 0x38,
				0x6c, 0x52, 0x41, 0x6e, 0x4e, 0x4d, 0x54, 0x51, 0x63, 0x62, 0x53, 0x36,
				0x34, 0x34, 0x54, 0x76, 0x49, 0x41, 0x30, 0x42, 0x57, 0x45, 0x31, 0x64,
				0x33, 0x52, 0x59, 0x58, 0x4f, 0x50, 0x67, 0x6c, 0x52, 0x66, 0x4d, 0x47,
				0x70, 0x34, 0x4d, 0x72, 0x6f, 0x4d, 0x44, 0x65, 0x33, 0x37, 0x6e, 0x5a,
				0x51, 0x57, 0x54, 0x31, 0x4f, 0x43, 0x61, 0x65, 0x4a, 0x43, 0x69, 0x65,
				0x45, 0x6a, 0x53, 0x78, 0x49, 0x6f, 0x4e, 0x4d, 0x6c, 0x70, 0x51, 0x72,
				0x54, 0x4e, 0x6d, 0x48, 0x7a, 0x49, 0x44, 0x70, 0x6a, 0x45, 0x73, 0x49,
				0x73, 0x48, 0x6b, 0x66, 0x36, 0x65, 0x6e, 0x35, 0x4d, 0x48, 0x6d, 0x65,
				0x72, 0x59, 0x79, 0x6c, 0x42, 0x52, 0x41, 0x76, 0x71, 0x45, 0x48, 0x52,
				0x71, 0x4c, 0x66, 0x41, 0x46, 0x56, 0x67, 0x6c, 0x41, 0x6e, 0x33, 0x4e,
				0x47, 0x6f, 0x68, 0x35, 0x38, 0x68, 0x31, 0x61, 0x30, 0x5a, 0x64, 0x73,
				0x4d, 0x6d, 0x65, 0x58, 0x64, 0x68, 0x6c, 0x6d, 0x74, 0x46, 0x32, 0x4d,
				0x44, 0x47, 0x45, 0x41, 0x45, 0x70, 0x74, 0x56, 0x42, 0x67, 0x6d, 0x6b,
				0x75, 0x6e, 0x62, 0x61, 0x36, 0x36, 0x5a, 0x32, 0x39, 0x49, 0x55, 0x55,
				0x50, 0x69, 0x62, 0x72, 0x33, 0x36, 0x51, 0x30, 0x49, 0x61, 0x36, 0x39,
				0x37, 0x5a, 0x69, 0x44, 0x37, 0x63, 0x7a, 0x47, 0x61, 0x37, 0x41, 0x73,
				0x77, 0x55, 0x42, 0x42, 0x64, 0x50, 0x76, 0x44, 0x39, 0x31, 0x78, 0x47,
				0x32, 0x6b, 0x56, 0x75, 0x57, 0x58, 0x75, 0x31, 0x59, 0x6d, 0x67, 0x61,
				0x46, 0x78, 0x4d, 0x42, 0x35, 0x6a, 0x37, 0x78, 0x4c, 0x39, 0x51, 0x5a,
				0x4d, 0x73, 0x59, 0x4c, 0x42, 0x54, 0x44, 0x48, 0x52, 0x67, 0x38, 0x77,
				0x76, 0x78, 0x45, 0x70, 0x48, 0x6e, 0x5a, 0x43, 0x74, 0x4e, 0x56, 0x43,
				0x41, 0x74, 0x45, 0x6e, 0x47, 0x4a, 0x46, 0x6d, 0x32, 0x30, 0x56, 0x45,
				0x31, 0x30, 0x73, 0x6b, 0x6b, 0x43, 0x36, 0x46, 0x37, 0x70, 0x69, 0x46,
				0x43, 0x6c, 0x53, 0x31, 0x55, 0x77, 0x36, 0x73, 0x4a, 0x50, 0x76, 0x6a,
				0x52, 0x72, 0x78, 0x69, 0x63, 0x68, 0x56, 0x5a, 0x7a, 0x68, 0x33, 0x6b,
				0x55, 0x53, 0x54, 0x4c, 0x45, 0x33, 0x44, 0x32, 0x33, 0x45, 0x71, 0x54,
				0x00, 0xee, 0x00, 0x00
			}
		},
		{
			// Binary data test
			.length_plain = 288,
			.data_plain = {
				0x04, 0x97, 0x89, 0x8d, 0x00, 0xa6, 0xc9, 0x5b, 0x02, 0x87, 0x1e, 0x06,
				0x89, 0x1e, 0x06, 0x89, 0x5f, 0x89, 0x4b, 0x1e, 0x4b, 0xa9, 0x4b, 0x00,
				0x8d, 0x00, 0xaa, 0x04, 0x5b, 0x09, 0x87, 0x96, 0x1c, 0x00, 0x06, 0x89,
				0x1e, 0x06, 0x89, 0x5f, 0x89, 0x4b, 0x1e, 0x4b, 0xa9, 0x4b, 0x00, 0x8d,
				0x00, 0xaa, 0x04, 0x5b, 0x09, 0x87, 0x7b, 0x04, 0xab, 0x30, 0xa1, 0x39,
				0x23, 0x08, 0xab, 0x07, 0x0d, 0x05, 0x27, 0x02, 0xab, 0x20, 0x1e, 0x09,
				0x89, 0x88, 0x4b, 0x77, 0x4b, 0xa9, 0x4b, 0x00, 0x1e, 0x0d, 0x89, 0x7b,
				0x0e, 0x88, 0x87, 0x5b, 0x03, 0x87, 0x88, 0x7b, 0x05, 0x4e, 0xa4, 0x0f,
				0x6b, 0x01, 0x1e, 0x0a, 0x89, 0x1e, 0x0a, 0x89, 0x7b, 0x0b, 0x88, 0x7b,
				0x0b, 0x88, 0x7b, 0x07, 0x88, 0x8d, 0x00, 0xa9, 0x56, 0x5b, 0x07, 0x7b,
				0x05, 0xa4, 0x0f, 0x6b, 0x01, 0x1e, 0x0a, 0x89, 0x1e, 0x0a, 0x89, 0x7b,
				0x0b, 0x88, 0x7b, 0x0b, 0x88, 0x7b, 0x07, 0x88, 0x8d, 0x00, 0xa9, 0x56,
				0x5b, 0x08, 0x87, 0x52, 0x0b, 0x16, 0x0f, 0x17, 0x01, 0x93, 0x90, 0xee,
				0x02, 0xfe, 0x1f, 0x07, 0x1e, 0x01, 0x1c, 0x00, 0x04, 0xa6, 0x20, 0x6b,
				0x0b, 0xf6, 0x48, 0x6b, 0x06, 0x7b, 0x07, 0x48, 0x4f, 0x49, 0x1a, 0x06,
				0xf7, 0x90, 0x58, 0x09, 0x08, 0x09, 0x07, 0x11, 0x11, 0x25, 0x15, 0xf6,
				0x10, 0x11, 0xf7, 0x90, 0x54, 0x99, 0x90, 0x59, 0x7b, 0x07, 0x6b, 0x03,
				0x7b, 0x08, 0x6b, 0x08, 0x7b, 0x03, 0x6b, 0x07, 0x0a, 0x0b, 0x0d, 0x0b,
				0x26, 0xcf, 0x1e, 0x01, 0xef, 0x02, 0x16, 0x07, 0xff, 0x5b, 0x0b, 0x87,
				0x52, 0x29, 0x5f, 0x1f, 0x10, 0x96, 0x1c, 0x00, 0x09, 0x1f, 0x12, 0x1f,
				0x14, 0x16, 0x12, 0x17, 0x16, 0x1e, 0x32, 0xf6, 0x5c, 0x1f, 0x32, 0x97,
				0x4d, 0x26, 0x04, 0xac, 0x00, 0xb0, 0xb4, 0x9f, 0xa1, 0x25, 0x27, 0x04,
				0xac, 0x00, 0xb0, 0x96, 0x0f, 0x18, 0x0f, 0x19, 0x0f, 0x1a, 0x0f, 0x1b,
				0x0f, 0x1c, 0x0f, 0x1d, 0x0f, 0x1e, 0x0f, 0x1f, 0x0f, 0x20, 0x5f, 0x1f
			},
			.length_compressed = 254,
			.data_compressed = {
				0x7f, 0x1b, 0x04, 0x97, 0x89, 0x8d, 0x00, 0xa6, 0xc9, 0x5b, 0x02, 0x87,
				0x1e, 0x06, 0x89, 0x1e, 0x06, 0x89, 0x5f, 0x89, 0x4b, 0x1e, 0x4b, 0xa9,
				0x4b, 0x00, 0x8d, 0x00, 0xaa, 0x04, 0x5b, 0x09, 0x87, 0x96, 0x1c, 0x00,
				0xe9, 0x02, 0x71, 0x0f, 0x7b, 0x04, 0xab, 0x30, 0xa1, 0x39, 0x23, 0x08,
				0xab, 0x07, 0x0d, 0x05, 0x27, 0x02, 0xab, 0x20, 0x1e, 0x09, 0x89, 0x88,
				0x4b, 0x77, 0xdf, 0x70, 0x0e, 0x1e, 0x0d, 0x89, 0x7b, 0x0e, 0x88, 0x87,
				0x5b, 0x03, 0x87, 0x88, 0x7b, 0x05, 0x4e, 0xa4, 0x0f, 0x6b, 0x01, 0x1e,
				0x0a, 0x89, 0xfd, 0x31, 0x7b, 0x0b, 0x88, 0xfd, 0x7f, 0x03, 0x07, 0x88,
				0x8d, 0x00, 0xa9, 0x56, 0x5b, 0x07, 0x7b, 0x05, 0xe5, 0x06, 0x71, 0x6f,
				0x08, 0x87, 0x52, 0x0b, 0x16, 0x0f, 0x17, 0x01, 0x93, 0x90, 0xee, 0x02,
				0xfe, 0x1f, 0x07, 0x1e, 0x01, 0x1c, 0x00, 0x04, 0xa6, 0x20, 0x6b, 0x0b,
				0xf6, 0x48, 0x6b, 0x06, 0x7b, 0x07, 0x48, 0x4f, 0x49, 0x1a, 0x06, 0xf7,
				0x90, 0x58, 0x09, 0x08, 0x09, 0x07, 0x11, 0x11, 0x25, 0x15, 0xf6, 0x10,
				0x11, 0xf7, 0x90, 0x54, 0x99, 0x90, 0x59, 0x7b, 0x07, 0x6b, 0x03, 0x7b,
				0x08, 0x6b, 0x08, 0x7b, 0x03, 0x6b, 0x07, 0x0a, 0x0b, 0x0d, 0x0b, 0x26,
				0xcf, 0x1e, 0x01, 0xef, 0x02, 0x16, 0x07, 0xff, 0x5b, 0x0b, 0x87, 0x52,
				0x29, 0x5f, 0x1f, 0x10, 0x96, 0x1c, 0x00, 0x09, 0x1f, 0x12, 0x1f, 0x14,
				0x16, 0x12, 0x17, 0x16, 0x1e, 0x32, 0xf6, 0x5c, 0x1f, 0x32, 0x97, 0x4d,
				0x26, 0x04, 0xac, 0x00, 0xb0, 0xb4, 0x9f, 0xa1, 0x25, 0x27, 0xf7, 0x7f,
				0x0e, 0x96, 0x0f, 0x18, 0x0f, 0x19, 0x0f, 0x1a, 0x0f, 0x1b, 0x0f, 0x1c,
				0x0f, 0x1d, 0x0f, 0x1e, 0x0f, 0x1f, 0x0f, 0x20, 0x5f, 0x1f, 0x00, 0xee,
				0x00, 0x00
			}
		},
		{
			// This is a test case consisting of a single match of length between
			// 256 and 511 (i.e. length denoted by single byte + 256).
			.length_plain = 288,
			.data_plain = {
				0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41,
				0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41,
				0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41,
				0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41,
				0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41,
				0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41,
				0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41,
				0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41,
				0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41,
				0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41,
				0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41,
				0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41,
				0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41,
				0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41,
				0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41,
				0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41,
				0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41,
				0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41,
				0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41,
				0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41,
				0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41,
				0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41,
				0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41,
				0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41
			},
			.length_compressed = 10,
			.data_compressed = {
				0x1f, 0x41, 0xff, 0xef, 0x1f, 0x0f, 0x00, 0xee, 0x00, 0x00
			}
		},
		{
			// This is a test case consisting of a single match of length greater
			// than 512 (i.e. length denoted by two bytes).
			.length_plain = 560,
			.data_plain = {
				0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41,
				0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41,
				0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41,
				0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41,
				0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41,
				0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41,
				0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41,
				0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41,
				0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41,
				0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41,
				0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41,
				0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41,
				0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41,
				0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41,
				0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41,
				0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41,
				0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41,
				0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41,
				0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41,
				0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41,
				0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41,
				0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41,
				0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41,
				0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41,
				0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41,
				0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41,
				0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41,
				0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41,
				0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41,
				0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41,
				0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41,
				0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41,
				0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41,
				0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41,
				0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41,
				0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41,
				0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41,
				0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41,
				0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41,
				0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41,
				0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41,
				0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41,
				0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41,
				0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41,
				0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41,
				0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41,
				0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41
			},
			.length_compressed = 11,
			.data_compressed = {
				0x1f, 0x41, 0xff, 0xee, 0x2f, 0x02, 0x0f, 0x00, 0xee, 0x00, 0x00
			}
		},
	};
	static uint8_t out[600];
	ptrdiff_t out_len;
	int cmp;

	for(size_t i = 0; i < (sizeof(in) / sizeof(in[0])); i++) {
		memset(out, '\0', sizeof(out));
		out_len = lzsa1_decompress_block(out, &in[i].data_compressed) - out;
		cmp = memcmp(out, &in[i].data_plain, in[i].length_plain);
		printf("\x1B[1m\x1B[33mTEST\x1B[0m %u:\n", i);
		print_hex_data(out, out_len);
		printf("length_plain = %u, out_len = %td\n", in[i].length_plain, out_len);
		puts(cmp == 0 && out_len == in[i].length_plain ? pass_str : fail_str);
		count_test_result(cmp == 0 && out_len == in[i].length_plain, result);
	}
}

static void benchmark_lzsa1(void) {
	static const struct {
		size_t length;
		uint8_t data[50];
	} in = {
		.length = 43, // Expands to 51 bytes
		.data = {
			0x73, 0x01, 0x48, 0x65, 0x6C, 0x6C, 0x6F, 0x2C, 0x20, 0x68, 0xF9, 0x53, 0x69, 0x73, 0x20, 0x74,
			0x68, 0xFB, 0x76, 0x07, 0x6E, 0x67, 0x20, 0x6F, 0x6E, 0x3F, 0x20, 0x42, 0x6C, 0x61, 0x68, 0x2C,
			0x20, 0x62, 0xFA, 0x3F, 0x2E, 0x2E, 0x2E, 0x00, 0xEE, 0x00, 0x00
		}
	};
	static uint8_t out[60];

	benchmark("lzsa1_decompress_block_ref", 1000, lzsa1_decompress_block_ref(out, &in.data));
	benchmark("lzsa1_decompress_block", 1000, lzsa1_decompress_block(out, &in.data));
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

	printf("TOTAL RESULTS: passed = %u, failed = %u\n", results.pass_count, results.fail_count);

	puts(hrule_str);

	benchmark_lzsa1();

	puts(hrule_str);

	while(1);
}
