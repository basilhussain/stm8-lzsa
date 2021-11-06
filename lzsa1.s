; ------------------------------------------------------------------------------
; LZSA1 BLOCK DECOMPRESSION FOR STM8
; ------------------------------------------------------------------------------
;
; lzsa1.s - Main LZSA1 decompression routine
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
;
; Function declaration:
;     void * lzsa1_decompress_block(void *dst, const void *src)
; Arguments:
;     dst = pointer to destination decompression buffer
;     src = pointer to source compressed data
; Returns:
;     Pointer to a position in the given destination buffer after the last byte
;     of decompressed data.
;
; NOTE: this function is not re-entrant, due to use of static variables.
;
; Inspiration for algorithm and structure taken from decompression routine for
; 6809 microprocessor by Emmanuel Marty.
; https://github.com/emmanuel-marty/lzsa
;
; LZSA1 block format documentation:
; https://github.com/emmanuel-marty/lzsa/blob/master/BlockFormat_LZSA1.md

.module lzsa1
.globl _lzsa1_decompress_block

; ------------------------------------------------------------------------------
; Static global variables (plus MSB/LSB aliases for convenience)
; ------------------------------------------------------------------------------

.area DATA

lit_len: .blkw 1
lit_len_msb .equ (lit_len+0)
lit_len_lsb .equ (lit_len+1)

match_off: .blkw 1
match_off_msb .equ (match_off+0)
match_off_lsb .equ (match_off+1)

match_len: .blkw 1
match_len_msb .equ (match_len+0)
match_len_lsb .equ (match_len+1)

; ------------------------------------------------------------------------------
; Function code
; ------------------------------------------------------------------------------

.area CODE

_lzsa1_decompress_block:
	; Load source pointer to X reg and destination pointer to Y reg.
	ldw x, (ARGS_SP_OFFSET+2, sp)
	ldw y, (ARGS_SP_OFFSET+0, sp)

lzsa1_token:
	; Token format: O|LLL|MMMM

	; Load next token into A. Also save it on the stack for later.
	ld a, (x)
	incw x
	push a

	; Mask off LLL literal length from token in A. Branch if no literals (length
	; is zero). Check if there is optional extra literal length byte (i.e.
	; length is 7). If not, we have final count, so go ahead and copy literals.
	and a, #0x70
	jreq lzsa1_no_lit
	cp a, #0x70
	jrne lzsa1_decode_lit_len

	; Load extra literal length byte. Add 7 to it and if there is no carry,
	; value was 0-248 (final literal length). If carry but now non-zero, value
	; was 250 (one more byte). Otherwise, value was 249 (two more bytes).
	ld a, (x)
	incw x
	add a, #7
	jrnc lzsa1_small_lit_len
	jrne lzsa1_medium_lit_len

	; Load two more bytes and set as length word var, converting from little- to
	; big-endian as we go. Then go ahead and copy literals.
	ld a, (x)
	incw x
	ld lit_len_lsb, a
	ld a, (x)
	incw x
	ld lit_len_msb, a
	jra lzsa1_got_lit_len

lzsa1_medium_lit_len:
	; Load second literal length byte. Add 256 to it by setting MSB of literal
	; length word variable to 1 and setting LSB to loaded value. Then go ahead
	; and copy literals.
	ld a, (x)
	incw x
	mov lit_len_msb, #0x01
	ld lit_len_lsb, a
	jra lzsa1_got_lit_len

lzsa1_decode_lit_len:
	; Shift literal count right by 4 bits, by simply swapping nibbles.
	swap a

lzsa1_small_lit_len:
	; Clear MSB of literal length word variable, set current value of A to LSB.
	clr lit_len_msb
	ld lit_len_lsb, a

lzsa1_got_lit_len:
lzsa1_copy_lit_loop:
	; Test if literal length variable value is zero. If so, proceed to handling
	; match offset. Otherwise, continue to copy next literal byte.
	tnz lit_len_msb
	jrne lzsa1_copy_lit
	tnz lit_len_lsb
	jrne lzsa1_copy_lit
	jra lzsa1_no_lit

lzsa1_copy_lit:
	; Decrement literal length word variable in-place (without using X/Y
	; registers and DECW instruction).
	ld a, lit_len_lsb
	sub a, #1
	ld lit_len_lsb, a
	ld a, lit_len_msb
	sbc a, #0
	ld lit_len_msb, a

	; Copy a single byte from source to destination.
	ld a, (x)
	incw x
	ld (y), a
	incw y

	; Loop around to next byte (if any remain).
	jra lzsa1_copy_lit_loop

lzsa1_no_lit:
	; Load match offset low byte from source and set as LSB of match offset var.
	ld a, (x)
	incw x
	ld match_off_lsb, a

	; Retrieve token from stack (without popping it) and check O flag bit.
	; If set, proceed to load optional high match offset byte.
	ld a, (1, sp)
	jrmi lzsa1_big_match_off

	; Otherwise, we don't have optional high match offset byte, so default MSB
	; of var to 0xFF.
	mov match_off_msb, #0xFF
	jra lzsa1_got_match_off

lzsa1_big_match_off:
	; Load second high match offset byte from source. Set as MSB of match offset
	; word variable.
	ld a, (x)
	incw x
	ld match_off_msb, a

lzsa1_got_match_off:
	; Retrieve token from stack (popping this time), mask off MMMM match length
	; bits, add the minimum match length (3) to the value. Place in LSB of match
	; length word variable (and clear MSB).
	pop a
	and a, #0x0F
	add a, #3
	clr match_len_msb
	ld match_len_lsb, a

	; Check if we have optional extra match length bytes (i.e. match length was
	; 15 before addition). Otherwise, we have final length, so proceed to copy
	; matched bytes.
	cp a, #18
	jrne lzsa1_got_match_len

	; Read another byte from source and add to current match length (18). If
	; there is no carry, value was 0-237 and we now have the final match length.
	; If carry but now non-zero, value was 239 (one more byte). Otherwise, value
	; was 238 (two more bytes).
	add a, (x)
	incw x
	tnz a
	jrnc lzsa1_small_match_len
	jrne lzsa1_medium_match_len

	; Load two more bytes and set as match length word variable, converting from
	; little- to big-endian as we go. Then proceed to copy matched bytes.
	ld a, (x)
	incw x
	ld match_len_lsb, a
	ld a, (x)
	incw x
	ld match_len_msb, a

	; Check if the two-byte match length is zero, which indicates end-of-data
	; (EOD) for the block. If it is, we're done, so carry on and exit.
	tnz match_len_msb
	jrne lzsa1_got_match_len
	tnz match_len_lsb
	jrne lzsa1_got_match_len

	; Return current destination pointer in X reg.
	ldw x, y
	return
	; ret

lzsa1_medium_match_len:
	; Load second match length byte. Add 256 to it by setting MSB of match
	; length word variable to 1 and setting LSB to loaded value. Then proceed to
	; copy matched bytes.
	ld a, (x)
	incw x
	mov match_len_msb, #0x01
	ld match_len_lsb, a
	jra lzsa1_got_match_len

lzsa1_small_match_len:
	; Clear MSB of match length word variable, set current value of A to LSB.
	clr match_len_msb
	ld match_len_lsb, a

lzsa1_got_match_len:
	; Save current source pointer on stack. Copy current destination pointer to
	; X reg and add match offset to it.
	pushw x
	ldw x, y
	addw x, match_off

lzsa1_copy_match_loop:
	; Test if match length variable value is zero. If not, continue to copy next
	; matched byte. Otherwise, exit loop.
	tnz match_len_msb
	jrne lzsa1_copy_match
	tnz match_len_lsb
	jrne lzsa1_copy_match
	jra lzsa1_no_match

lzsa1_copy_match:
	; Decrement match length word variable in-place (without using X/Y registers
	; and DECW instruction).
	ld a, match_len_lsb
	sub a, #1
	ld match_len_lsb, a
	ld a, match_len_msb
	sbc a, #0
	ld match_len_msb, a

	; Copy a single byte from source to destination.
	ld a, (x)
	incw x
	ld (y), a
	incw y

	; Loop around to next byte (if any remain).
	jra lzsa1_copy_match_loop

lzsa1_no_match:
	; Restore source pointer from stack. Proceed to next token.
	popw x
	jump_abs lzsa1_token
	; jp lzsa1_token
