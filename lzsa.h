/*******************************************************************************
 *
 * lzsa.h - Header for LZSA decompression library
 *
 * Copyright (c) 2022 Basil Hussain
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

#ifndef LZSA_H_
#define LZSA_H_

#include <stddef.h>
#include <stdint.h>

// Short-term fix to force usage of old ABI when compiled with SDCC v4.2.0
// (or newer). New ABI passes simple arguments (e.g. one or two 8- or 16-bit
// values) in A/X registers, versus previous where all args are on the stack.
// Eventually, all assembly code should be changed to use new ABI. May actually
// offer a performance increase, eliminating code that simply takes arg values
// and loads them to A/X registers.
#if defined(__SDCCCALL) && __SDCCCALL != 0
#define __stack_args __sdcccall(0)
#else
#define __stack_args
#endif

extern void * lzsa1_decompress_block(void *dst, const void *src) __stack_args;
extern void * lzsa2_decompress_block(void *dst, const void *src) __stack_args;

#endif // LZSA_H_
