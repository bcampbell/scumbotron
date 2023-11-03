; This is the scumbotron front end which unpacks the code and graphics and
; sets it all running.

; 1. copy our unpacker routine to free space at $0400 and jump to it.
; 2. uncompress graphics data directly into VRAM
; 3. copy our compressed code chunks to RAM banks (one per bank).
; 4. decompress each code chunk back out to $0801 onward
;    (overwriting this code and the compressed data)
; 5. run the game

; assembles with 64tass, eg:
; $ 64tass -o scumbotron.prg scumbotron.asm


; DEFINITIONS 
	; cx16 kernal 16bit pseudo-registers
	r0 = $02
	r1 = $04
	r2 = $06
	r3 = $08

	VERA_ADDR_L = $9F20
	VERA_ADDR_M = $9F21
	VERA_ADDR_H = $9F22

	VERA_DATA0 = $9F23
	VERA_DATA1 = $9F24
	VERA_CTRL = $9F25

	VERA_INC_1 = 1<<4;

	; memory_copy()
	; source: r0, target: r1, num_bytes: r2
	cx16_k_memory_copy = $FEE7

	; memory_decompress()
	; source: r0, target: r1
	cx16_k_memory_decompress = $FEED


; BASIC stub

	*=$0801
	; basic stub
	; 64tass prepends start address $01,$08 by default
	; 1 SYS 2061
    .byte $0b,$08,$01,$00,$9e,$32,$30,$36,$31,$00,$00,$00


; COPY unpacker out of the way (so we don't blat ourself while running)

	; src
	lda #<shifted_start
	sta r0
	lda #>shifted_start
	sta r0+1

	; dest
	lda #<unpacker
	sta r1
	lda #>unpacker
	sta r1+1

	; size
	lda #<(shifted_end-shifted_start)
	sta r2
	lda #>(shifted_end-shifted_start)
	sta r2+1

	jsr cx16_k_memory_copy
	jmp unpacker

shifted_start
	.logical $0400

; THE UNPACKER

unpacker
	; Load the gfx data into VRAM.
	; set up data0 to write bytes at 0x10000
	lda #0
	sta VERA_CTRL
	sta VERA_ADDR_L
	sta VERA_ADDR_M
	lda #VERA_INC_1 | 1
	sta VERA_ADDR_H

	; src: compressed gfx data
	lda #<gfx
	sta r0
	lda #>gfx
	sta r0+1
	
	; dest vera data0
	lda #<VERA_DATA0
	sta r1
	lda #>VERA_DATA0
	sta r1+1

    jsr cx16_k_memory_decompress

	; Unpack the game.prg code. First copy each compressed chunk into it's own 8KB
	; RAM bank, then go through and uncompress them back out into 0801 onward.
	; That way we won't accidentially overwrite anything.

	ldx #0	; byte index into code_chunks table
	ldy #1	; bank number
_copyloop
	; map in the bank we want
	sty $0
	iny

	; src addr
	lda code_chunks,x
	sta r0	
	inx
	lda code_chunks,x
	sta r0+1	
	inx

	; dest addr - the mapped ram
	lda #<$A000
	sta r1
	lda #>$A000
	sta r1+1

	; size
	lda code_chunks,x
	sta r2
	inx
	lda code_chunks,x
	sta r2+1
	inx

	stx r3
	sty r3+1
	jsr cx16_k_memory_copy
	ldx r3
	ldy r3+1
	cpx #4*num_code_chunks
	bne _copyloop

	; now go through the banks, uncompressing them back into main mem
	
	; decompress will advance r1 (dest addr) as we go, so only need to set it once.
	lda #<$0801
	sta r1
	lda #>$0801
	sta r1+1
	ldy #1	; bank number
_unpackloop
	sty 0	; set bank
	iny

	lda #<$A000
	sta r0
	lda #>$A000
	sta r0+1

	sty r3
	jsr cx16_k_memory_decompress
	ldy r3

	cpy #1+num_code_chunks
	bne _unpackloop	

	; run the game!
	; installed game code still has llvm-mos BASIC header.
	; 7773 sys 2071.

	; NOTE: for some reason, llvm-mos isn't consistent with _start address,
    ; is the address that appears in the BASIC sys statement.
	; I've seen 2061 2071, 2072, 2075, 2082, 2087, 2088...
	; I think it sometimes just decides to place other small data sections
    ; before the main code chunk?

	; Read the SYS address from our BASIC stub.
	; This is stupid - it should be a fixed location, but hey.

	lda #<$0806
	sta r0
	lda #>$0806
	sta r0+1
	jsr asc4_to_word
	
	; FINALLY! We can run the game.
	jmp (r1)	


	; asc4_to_word converts a 4-digit ascii(/petscii) number into
	; binary.
	; in: r0 = addr of ascii number
	; out: r1 = result
	; destroys: a x y
asc4_to_word

	ldy #4	; assume 4 digits
	; r1 = 0
	lda #0
	sta r1
	sta r1+1

asc_to_word_loop
	dey
	bpl fetch_digit
	rts	; done.

fetch_digit
	lda (r0),y
	sec
	sbc #$30	; '0'
	tax
	; loop x times, adding the column value
mult_loop
	dex
	bmi asc_to_word_loop
	clc
	lda tens_lo,y
	adc r1
	sta r1
	lda tens_hi,y
	adc r1+1
	sta r1+1
	jmp mult_loop

tens_lo
	.byte <1000, <100, <10, <1
tens_hi
	.byte >1000, >100, >10, >1

	.endlogical
shifted_end


; include the compressed data
gfx
	.binary "gfx.zbin"
gfx_end

code00
	.binary "game00.zbin"
code00_end
code01
	.binary "game01.zbin"
code01_end
code02
	.binary "game02.zbin"
code02_end
code03
	.binary "game03.zbin"
code03_end
code04
	.binary "game04.zbin"
code04_end


num_code_chunks=5
code_chunks
	.word code00, code00_end-code00
	.word code01, code01_end-code01
	.word code02, code02_end-code02
	.word code03, code03_end-code03
	.word code04, code04_end-code04


