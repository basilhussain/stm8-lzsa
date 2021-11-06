/*******************************************************************************
 *
 * lzsa_ref.c - Reference LZSA decompression implementation
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
#include "lzsa_ref.h"

#define TOKEN_16B_MATCH_OFFSET_FLAG_MASK 0x80
#define TOKEN_LITERAL_LEN_MASK 0x70
#define TOKEN_MATCH_LEN_MASK 0x0F
#define MATCH_LEN_MIN 3

void * lzsa1_decompress_block_ref(void *dst, const void *src) {
	const uint8_t *in = (const uint8_t *)src;
	uint8_t *out = (uint8_t *)dst;
	uint8_t n;

	while(1) {
		// Get next token byte.
		const uint8_t token = *in++;
		uint16_t lit_len = ((token & TOKEN_LITERAL_LEN_MASK) >> 4);
		uint16_t match_len = ((token & TOKEN_MATCH_LEN_MASK) >> 0);

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

		// Copy the specified number of literal bytes to the output.
		while(lit_len-- > 0) *out++ = *in++;

		// First match offset byte is LSB of offset. If flag in token is set, an
		// optional second byte exists, so read and make MSB of offset.
		// Otherwise, the MSB is 0xFF.
		int16_t match_off = *in++;
		if(token & TOKEN_16B_MATCH_OFFSET_FLAG_MASK) {
			match_off |= ((int16_t)*in++ << 8);
		} else {
			match_off |= 0xFF00;
		}

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
				match_len += n + MATCH_LEN_MIN;
			}
		} else {
			match_len += MATCH_LEN_MIN;
		}

		// Calculate the absolute position for copy by adding negative match
		// offset to current output position.
		const uint8_t *match_src = out + match_off;

		// Copy the specified number of bytes from previous output data to the
		// output.
		while(match_len-- > 0) *out++ = *match_src++;
	}

	return out;
}
