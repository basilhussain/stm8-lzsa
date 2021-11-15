/*******************************************************************************
 *
 * lzsa_ref.c - Reference LZSA decompression implementations
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
#include <stdio.h>
#include "lzsa_ref.h"

#define LZSA1_TOKEN_16B_MATCH_OFFSET_FLAG_MASK 0x80
#define LZSA1_TOKEN_LITERAL_LEN_MASK 0x70
#define LZSA1_TOKEN_MATCH_LEN_MASK 0x0F
#define LZSA1_MATCH_LEN_MIN 3

#define LZSA2_TOKEN_LITERAL_LEN_MASK 0x18
#define LZSA2_TOKEN_MATCH_LEN_MASK 0x07
#define LZSA2_TOKEN_MATCH_OFFSET_MODE_MASK 0xC0
#define LZSA2_TOKEN_MATCH_OFFSET_MODE_5BIT 0x00
#define LZSA2_TOKEN_MATCH_OFFSET_MODE_9BIT 0x40
#define LZSA2_TOKEN_MATCH_OFFSET_MODE_13BIT 0x80
#define LZSA2_TOKEN_MATCH_OFFSET_MODE_16BIT 0xC0
#define LZSA2_MATCH_LEN_MIN 2

// Macro to read a pair of nibbles (i.e. a byte) from given input pointer and
// cache them. Upon first invocation, when a new byte is read, returns the high
// nibble; subsequent invocation returns the low nibble. This sequence repeats.
// First arg is a boolean 'ready' variable (which must be initialised to true),
// second a uint8_t variable holding the cache, third the pointer to read from.
#define lzsa2_fetch_nibble(r, n, p) (((r) = !(r)) ? ((n) & 0x0F) : ((((n) = *(p)++) & 0xF0) >> 4))

/******************************************************************************/

void * lzsa1_decompress_block_ref(void *dst, const void *src) {
	const uint8_t *in = (const uint8_t *)src;
	uint8_t *out = (uint8_t *)dst;
	uint8_t n;

#ifdef LZSA_REF_DEBUG
	printf("lzsa1_decompress_block_ref(): in = %p, out = %p\n", in, out);
#endif

	while(1) {
		// Get next token byte and parse out values.
		const uint8_t token = *in++;
		uint16_t lit_len = ((token & LZSA1_TOKEN_LITERAL_LEN_MASK) >> 4);
		uint16_t match_len = ((token & LZSA1_TOKEN_MATCH_LEN_MASK) >> 0);

#ifdef LZSA_REF_DEBUG
		printf("lzsa1_decompress_block_ref(): token = %02x, lit_len = %u, match_len = %u\n", token, lit_len, match_len);
#endif

		// Handle optional extra literal length. Can either be a single extra
		// byte which is added to the initial length, a second extra byte which
		// sets the literal length to be 256 + <2nd byte>, or 2 extra bytes
		// which form a little-endian 16-bit value which sets the length.
		if(lit_len == 7) {
			n = *in++;
			if(n == 250) {
				lit_len = 256 + *in++;
			} else if(n == 249) {
				lit_len = *in++;
				lit_len |= (*in++ << 8);
			} else {
				lit_len += n;
			}
		}

#ifdef LZSA_REF_DEBUG
		printf("lzsa1_decompress_block_ref(): lit_len = %u\n", lit_len);
#endif

		// Copy the specified number of literal bytes to the output.
		while(lit_len-- > 0) *out++ = *in++;

		// First match offset byte is LSB of offset. If flag in token is set, an
		// optional second byte exists, so read and make MSB of offset.
		// Otherwise, the MSB is 0xFF.
		int16_t match_off = *in++;
		if(token & LZSA1_TOKEN_16B_MATCH_OFFSET_FLAG_MASK) {
			match_off |= ((int16_t)*in++ << 8);
		} else {
			match_off |= 0xFF00;
		}

#ifdef LZSA_REF_DEBUG
		printf("lzsa1_decompress_block_ref(): match_off = %d\n", match_off);
#endif

		// When actual match length is 15 or more, an extra byte follows to
		// represent the length, whose interpretation depends on its value. For
		// a value of 0-237, final match length is the byte plus 15 from the
		// token plus the minimum match length (e.g. <byte>+15+3). For a value
		// of 239, another byte follows, and final match length is
		// <2nd byte>+256. For a value of 238, two more bytes follow, forming a
		// little-endian 16-bit value that is the final match length. If that
		// length is zero, we have reached end-of-data (EOD), so quit.
		if(match_len == 15) {
			n = *in++;
			if(n == 239) {
				match_len = 256 + *in++;
			} else if(n == 238) {
				match_len = *in++;
				match_len |= (*in++ << 8);
				if(match_len == 0) break;
			} else {
				match_len += n + LZSA1_MATCH_LEN_MIN;
			}
		} else {
			match_len += LZSA1_MATCH_LEN_MIN;
		}

#ifdef LZSA_REF_DEBUG
		printf("lzsa1_decompress_block_ref(): match_len = %u\n", match_len);
#endif

		// Calculate the absolute position for copy by adding negative match
		// offset to current output position.
		const uint8_t *match_src = out + match_off;

		// Copy the specified number of bytes from previous output data to the
		// output.
		while(match_len-- > 0) *out++ = *match_src++;
	}

#ifdef LZSA_REF_DEBUG
	printf("lzsa1_decompress_block_ref(): out = %p\n", out);
#endif

	return out;
}

void * lzsa2_decompress_block_ref(void *dst, const void *src) {
	const uint8_t *in = (const uint8_t *)src;
	uint8_t *out = (uint8_t *)dst;
	bool nibble_rdy = true;
	uint8_t n, nibbles = 0x00;
	int16_t match_off;

#ifdef LZSA_REF_DEBUG
	printf("lzsa2_decompress_block_ref(): in = %p, out = %p\n", in, out);
#endif

	while(1) {
		// Get next token byte and parse out values. Token format is XYZ|LL|MMM.
		const uint8_t token = *in++;
		const uint8_t offset_mode = (token & LZSA2_TOKEN_MATCH_OFFSET_MODE_MASK);
		uint16_t lit_len = ((token & LZSA2_TOKEN_LITERAL_LEN_MASK) >> 3);
		uint16_t match_len = ((token & LZSA2_TOKEN_MATCH_LEN_MASK) >> 0);

#ifdef LZSA_REF_DEBUG
		printf("lzsa2_decompress_block_ref(): token = %02x, offset_mode = %02x, lit_len = %u, match_len = %u\n", token, offset_mode, lit_len, match_len);
#endif

		// Handle optional extra literal length.
		if(lit_len == 3) {
			n = lzsa2_fetch_nibble(nibble_rdy, nibbles, in);
			if(n == 15) {
				n = *in++;
				if(n <= 237) {
					lit_len += n + 15;
				} else if(n == 239) {
					lit_len = *in++;
					lit_len |= (*in++ << 8);
				} else {
					// Uhhh... Guess value of 238 never occurs?
					// TODO: should we exit here?
				}
			} else {
				lit_len += n;
			}
		}

#ifdef LZSA_REF_DEBUG
		printf("lzsa2_decompress_block_ref(): lit_len = %u\n", lit_len);
#endif

		// Copy the specified number of literal bytes to the output.
		while(lit_len-- > 0) *out++ = *in++;

		switch(offset_mode) {
			case LZSA2_TOKEN_MATCH_OFFSET_MODE_5BIT:
				// 5-bit offset:
				// Read a nibble for offset bits 1-4 and use the inverted bit Z
				// of the token as bit 0 of the offset. Set bits 5-15 of the
				// offset to 1.
				match_off = lzsa2_fetch_nibble(nibble_rdy, nibbles, in) << 1;
				match_off |= (~token & 0x20) >> 5;
				match_off |= 0xFFE0;
				break;
			case LZSA2_TOKEN_MATCH_OFFSET_MODE_9BIT:
				// 9-bit offset:
				// Read a byte for offset bits 0-7 and use the inverted bit Z
				// for bit 8 of the offset. Set bits 9-15 of the offset to 1.
				match_off = *in++;
				match_off |= (int16_t)(~token & 0x20) << 3;
				match_off |= 0xFE00;
				break;
			case LZSA2_TOKEN_MATCH_OFFSET_MODE_13BIT:
				// 13-bit offset:
				// Read a nibble for offset bits 9-12 and use the inverted bit Z
				// for bit 8 of the offset, then read a byte for offset bits
				// 0-7. Set bits 13-15 of the offset to 1. Subtract 512 from the
				// offset to get the final value.
				match_off = (int16_t)lzsa2_fetch_nibble(nibble_rdy, nibbles, in) << 9;
				match_off |= (int16_t)(~token & 0x20) << 3;
				match_off |= *in++;
				match_off |= 0xE000;
				match_off -= 512;
				break;
			case LZSA2_TOKEN_MATCH_OFFSET_MODE_16BIT:
				// Either 16-bit offset or repeat offset:
				// If Z bit not set, read a byte for offset bits 8-15, then
				// another byte for offset bits 0-7. Otherwise, reuse the offset
				// value of the previous match command.
				if(!(token & 0x20)) {
					match_off = *in++ << 8;
					match_off |= *in++;
				}
				break;
		}

#ifdef LZSA_REF_DEBUG
		printf("lzsa2_decompress_block_ref(): match_off = %d\n", match_off);
#endif

		if(match_len == 7) {
			n = lzsa2_fetch_nibble(nibble_rdy, nibbles, in);
			if(n == 15) {
				n = *in++;
				if(n <= 231) {
					match_len += n + 15 + LZSA2_MATCH_LEN_MIN;
				} else if(n == 233) {
					match_len = *in++;
					match_len |= *in++ << 8;
				} else {
					break; // EOD
				}
			} else {
				match_len += n + LZSA2_MATCH_LEN_MIN;
			}
		} else {
			match_len += LZSA2_MATCH_LEN_MIN;
		}

#ifdef LZSA_REF_DEBUG
		printf("lzsa2_decompress_block_ref(): match_len = %u\n", match_len);
#endif

		// Calculate the absolute position for copy by adding negative match
		// offset to current output position.
		const uint8_t *match_src = out + match_off;

		// Copy the specified number of bytes from previous output data to the
		// output.
		while(match_len-- > 0) *out++ = *match_src++;
	}

#ifdef LZSA_REF_DEBUG
	printf("lzsa2_decompress_block_ref(): out = %p\n", out);
#endif

	return out;
}
