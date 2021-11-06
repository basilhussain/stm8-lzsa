; ------------------------------------------------------------------------------
; LZSA1 BLOCK DECOMPRESSION FOR STM8
; ------------------------------------------------------------------------------
;
; lzsa1_medium.s - Medium memory model specific definitions and macros for LZSA1
;                  decompression routine
;
; Copyright (c) 2021 Basil Hussain
;
; Permission is hereby granted, free of charge, to any person obtaining a copy
; of this software and associated documentation files (the "Software"), to deal
; in the Software without restriction, including without limitation the rights
; to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
; copies of the Software, and to permit persons to whom the Software is
; furnished to do so, subject to the following conditions:
;
; The above copyright notice and this permission notice shall be included in all
; copies or substantial portions of the Software.
;
; THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
; IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
; FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
; AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
; LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
; OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
; SOFTWARE.
;
; ------------------------------------------------------------------------------

; Would prefer to use .define directives here instead of macros, but SDAS
; version of ASxxxx assembler (as of SDCC v4.1) doesn't support .define!

ARGS_SP_OFFSET .equ 3

.macro jump_abs lbl
	jp lbl
.endm

.macro return
	ret
.endm
