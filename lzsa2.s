; ------------------------------------------------------------------------------
; LZSA2 BLOCK DECOMPRESSION FOR STM8
; ------------------------------------------------------------------------------
;
; lzsa2.s - Main LZSA2 decompression routine
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
;     void * lzsa2_decompress_block(void *dst, const void *src)
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
; LZSA2 block format documentation:
; https://github.com/emmanuel-marty/lzsa/blob/master/BlockFormat_LZSA2.md

.module lzsa2
.globl _lzsa2_decompress_block

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

nibbles: .blkb 1
nibbles_rdy: .blkb 1

; ------------------------------------------------------------------------------
; Function code
; ------------------------------------------------------------------------------

.area CODE

_lzsa2_decompress_block:
	; Load source pointer to X reg and destination pointer to Y reg.
	ldw x, (ARGS_SP_OFFSET+2, sp)
	ldw y, (ARGS_SP_OFFSET+0, sp)

	mov nibbles_rdy, #0x01

lzsa2_token:
	; Token format: XYZ|LL|MMM

	; Load next token into A. Also save it on the stack for later.
	ld a, (x)
	incw x
	push a

	; Mask off LL literal length from token in A. Branch if no literals (length
	; is zero). Check if there is optional extra literal length byte (i.e.
	; length is 3). If not, we have final count, so go ahead and copy literals.
	and a, #0x18
	jreq lzsa2_no_lit
	cp a, #0x18
	jrne lzsa2_decode_lit_len

	; Fetch a nibble in to A reg. Add the existing literal length (3) to it and
	; if it's now 18, an optional extra literal length byte follows. Otherwise,
	; we have final length.
	call_abs lzsa2_fetch_nibble
	add a, #3
	cp a, #18
	jrne lzsa2_small_lit_len

	; Load extra literal length byte and add to existing value. If there was no
	; carry (i.e. byte read was 0-237), we have final length. Otherwise, value
	; was 239, signifying two more bytes.
	add a, (x)
	incw x
	jrnc lzsa2_small_lit_len

	; Load two more bytes and set as length word var, converting from little- to
	; big-endian as we go. Then go ahead and copy literals.
	ld a, (x)
	incw x
	ld lit_len_lsb, a
	ld a, (x)
	incw x
	ld lit_len_msb, a
	jra lzsa2_got_lit_len

lzsa2_decode_lit_len:
	; Shift literal length over 3 places.
	srl a
	srl a
	srl a

lzsa2_small_lit_len:
	; Clear MSB of literal length word variable, set current value of A to LSB.
	clr lit_len_msb
	ld lit_len_lsb, a

lzsa2_got_lit_len:
lzsa2_copy_lit_loop:
	; Test if literal length variable value is zero. If so, proceed to handling
	; match offset. Otherwise, continue to copy next literal byte.
	tnz lit_len_msb
	jrne lzsa2_copy_lit
	tnz lit_len_lsb
	jrne lzsa2_copy_lit
	jra lzsa2_no_lit

lzsa2_copy_lit:
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

	; Loop around to next byte.
	jra lzsa2_copy_lit_loop

lzsa2_no_lit:
	; Retrieve token from stack (without popping it). Shift off the match offset
	; mode X bit into carry. If set, we have 13- or 16-bit match offset. If not,
	; then shift off Y bit into carry. If set, we have 9-bit match offset.
	ld a, (1, sp)
	sll a
	jrc lzsa2_match_off_13b_16b
	sll a
	jrc lzsa2_match_off_9b

	; Otherwise, we have a 5-bit match offset. Shift off Z bit of mode to carry.
	; Read a nibble (into A) and rotate the value of that to offset bits 1-4 and
	; Z bit from mode (in carry) to bit 0. Then XOR with a mask to set bits 5-7
	; of the offset to 1 and flip the Z bit. Also set MSB of offset to all 1s.
	sll a
	call_abs lzsa2_fetch_nibble
	rlc a
	xor a, #0xE1
	ld match_off_lsb, a
	mov match_off_msb, #0xFF
	jra lzsa2_got_match_off

lzsa2_match_off_9b:
	; We have a 9-bit match offset. Shift off Z bit of mode to carry and invert.
	; Set MSB of offset to all 1s, then rotate Z bit in to bit 8. Load another
	; byte and set as LSB (bits 0-7) of offset.
	sll a
	ccf
	mov match_off_msb, #0xFF
	rlc match_off_msb
	ld a, (x)
	incw x
	ld match_off_lsb, a
	jra lzsa2_got_match_off

lzsa2_match_off_13b_16b:
	; Shift off Y bit into carry. If set, we have a 16-bit match offset.
	sll a
	jrc lzsa2_match_off_16b

	; Otherwise, we have a 13-bit offset. Shift off Z bit of mode to carry. Read
	; a nibble (into A) and rotate the value of that to offset bits 9-12 and Z
	; bit from mode (in carry) to bit 8. Then XOR with a mask to set bits 13-15
	; of the offset to 1 and flip the Z bit. Subtract 512 from final offset by
	; subtracting 2 from MSB. Finally, read a new byte and set as LSB (bits 0-7)
	; of offset.
	sll a
	call_abs lzsa2_fetch_nibble
	rlc a
	xor a, #0xE1
	sub a, #2
	ld match_off_msb, a
	ld a, (x)
	incw x
	ld match_off_lsb, a
	jra lzsa2_got_match_off

lzsa2_match_off_16b:
	; If Z bit of mode is set, we repeat the previous offset value.
	jrmi lzsa2_got_match_off

	; Otherwise, we have a 16-bit offset. Read two bytes containing the final
	; match offset value, already in big-endian format.
	ld a, (x)
	incw x
	ld match_off_msb, a
	ld a, (x)
	incw x
	ld match_off_lsb, a

lzsa2_got_match_off:
	; Retrieve token from stack (popping this time), mask off MMM match length
	; bits, add the minimum match length (2) to the value.
	pop a
	and a, #0x07
	add a, #2

	; Check if we have optional extra match length bytes (i.e. match length was
	; 7 before addition). Otherwise, we have final length, so proceed to copy
	; matched bytes.
	cp a, #9
	jrne lzsa2_small_match_len

	; Read a nibble (into A) and add the current match length (9) to it. If the
	; nibble value was 0-14 (before addition), we have final match length, so
	; proceed to copy matched bytes.
	call_abs lzsa2_fetch_nibble
	add a, #9
	cp a, #24
	jrne lzsa2_small_match_len

	; Read another byte from source and add to current match length. If there is
	; no carry, value was 0-231 and we have final length. If carry, but length
	; is zero, value was 232, signifying end-of-data (EOD), so quit. Otherwise,
	; value was 233, meaning two more bytes.
	add a, (x)
	incw x
	jrnc lzsa2_small_match_len
	tnz a
	jreq lzsa2_end

	; Load two more bytes and set as match length word variable, converting from
	; little- to big-endian as we go. Then proceed to copy matched bytes.
	ld a, (x)
	incw x
	ld match_len_lsb, a
	ld a, (x)
	incw x
	ld match_len_msb, a
	jra lzsa2_got_match_len

lzsa2_small_match_len:
	; Place match length value in LSB of length word variable and clear MSB.
	ld match_len_lsb, a
	clr match_len_msb

lzsa2_got_match_len:
	; Save current source pointer on stack. Copy current destination pointer to
	; X reg and add match offset to it.
	pushw x
	ldw x, y
	addw x, match_off

lzsa2_copy_match_loop:
	; Test if match length variable value is zero. If not, continue to copy next
	; matched byte. Otherwise, exit loop.
	tnz match_len_msb
	jrne lzsa2_copy_match
	tnz match_len_lsb
	jrne lzsa2_copy_match
	jra lzsa2_no_match

lzsa2_copy_match:
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

	; Loop around to next byte.
	jra lzsa2_copy_match_loop

lzsa2_no_match:
	; Restore source pointer from stack. Proceed to next token.
	popw x
	jump_abs lzsa2_token

lzsa2_end:
	; Return current destination pointer in X reg.
	ldw x, y
	return

; ------------------------------------------------------------------------------

; NOTE: we must be careful in this function not to alter the carry flag! Calling
; code relies on the value of the carry flag being maintained.

lzsa2_fetch_nibble:
	; Toggle the ready flag.
	bcpl nibbles_rdy, #0
	tnz nibbles_rdy        ; }
	jreq lzsa2_nib_not_rdy ; } Can't use btjf here as it changes carry.

	; We have nibbles ready. Mask off the low nibble and return in A reg.
	ld a, nibbles
	and a, #0x0F
	return

lzsa2_nib_not_rdy:
	; Load a new pair of nibbles (i.e. a byte) from input and store. Mask off
	; the high nibble, shift over and return the value in A reg.
	ld a, (x)
	incw x
	ld nibbles, a
	and a, #0xF0
	swap a
	return
