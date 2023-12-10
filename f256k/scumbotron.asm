; This is the scumbotron front end which unpacks the code and graphics and
; sets it all running.
;
; assemble with 64tass
; 64tass --cbm-prg -o a.prg -I build_f256k f256k/scumbotron.asm
;

; DEFINITIONS

	UNPACKER_START_ADDR = $E000	 ; slot 7 (usually kernal space)
	GAME_START_ADDR = $1000

	MMU_MEM_CTRL = $00
	MMU_IO_CTRL = $01
	MMU_MEM_BANK_0  = $08
	MMU_MEM_BANK_1  = $09
	MMU_MEM_BANK_2  = $0A
	MMU_MEM_BANK_3  = $0B
	MMU_MEM_BANK_4  = $0C
	MMU_MEM_BANK_5  = $0D
	MMU_MEM_BANK_6  = $0E
	MMU_MEM_BANK_7  = $0F

	; zeropage vars (same addresses as used by lzsa code)
	srcptr = $fc
	dstptr = $fe
	len = $fa

	r0 = $10

	

; START CODE
	* = $1000

	; we switch out kernel bank which includes the irq/nmi/reset vectors
	; so lets just disable interrupts for the duration...
	sei
	lda	#$80
	sta MMU_MEM_CTRL
	; set ram into all the slots
	ldx #0
	stx MMU_MEM_BANK_0
	ldx #1
	stx MMU_MEM_BANK_1
	ldx #2
	stx MMU_MEM_BANK_2
	ldx #3
	stx MMU_MEM_BANK_3
	ldx #4
	stx MMU_MEM_BANK_4
	ldx #5
	stx MMU_MEM_BANK_5
	ldx #6
	stx MMU_MEM_BANK_6
	ldx #7
	stx MMU_MEM_BANK_7		; usually kernel, but we want ram
	; done editing
	lda #0
	sta MMU_MEM_CTRL

	; copy unpacker out of the way (so we don't blat ourself while running)

	; src
	lda #<shifted_start
	sta srcptr
	lda #>shifted_start
	sta srcptr+1

	; dest
	lda #<unpacker
	sta dstptr
	lda #>unpacker
	sta dstptr+1

	; size
	; x = number of full 256-byte chunks
	ldx #>(shifted_end-shifted_start)
	beq _leftover
_copy256
	ldy #0 ; 256
_loop2
	lda (srcptr),y
	sta (dstptr),y
	iny
	bne _loop2
	inc srcptr+1	; src += 256
	inc dstptr+1	; dest += 256
	dex
	bne _copy256

	; copy the leftover bytes <256
_leftover
	ldy #0
_leftover_loop
	cpy #<(shifted_end-shifted_start)
	beq _done
	lda (srcptr),y
	sta (dstptr),y
	iny
	jmp _leftover_loop
_done
	jmp unpacker


shifted_start
	.logical UNPACKER_START_ADDR

; THE UNPACKER


unpacker:
	
	; 0) unpack charset into IO bank 1 (font)
	; appears at slot 6 ($c000)
	lda #1
	sta MMU_IO_CTRL
	lda #<chars
	sta lzsa_srcptr
	lda #>chars
	sta lzsa_srcptr+1
	lda #<$C000
	sta lzsa_dstptr
	lda #>$C000
	sta lzsa_dstptr+1
	jsr lzsa2_unpack

	; 1) unpack all the data chunks into their banks
	; disable IO mapping to ensure memory appears at slot 6 ($c000)
	lda #$04
	sta MMU_IO_CTRL

	ldx #0
_datachunk_loop
	txa
	pha
	jsr unpackdatachunk
	pla
	tax
	inx
	cpx #NUM_DATA_CHUNKS
	bne _datachunk_loop


	; 2) copy compressed code chunks into ram banks
	ldx #0
_codechunk_copy_loop
	txa
	pha
	jsr copycodechunk
	pla
	tax
	inx
	cpx #NUM_CODE_CHUNKS
	bne _codechunk_copy_loop


	; 3) go through the ram banks containing compressed code chunks and
	;    decompress them into contigious ram ($0200-$c000)

	lda #<GAME_START_ADDR
	sta r0
	lda #>GAME_START_ADDR
	sta r0+1

	ldx #0
_code_unpack_loop
	; map in the bank containing the compressed code
	txa
	pha
	clc
	adc #FIRST_CODE_BANK
	jsr mapbank
	pla
	tax

	; uncompress it
	lda #<$C000
	sta lzsa_srcptr
	lda #>$C000
	sta lzsa_srcptr+1

	lda r0
	sta lzsa_dstptr
	sta $F00D
	lda r0+1
	sta lzsa_dstptr+1
	sta $F00D

	txa
	pha
	jsr lzsa2_unpack
	pla
	tax

	lda r0
	clc
	adc #<$2000
	sta r0
	lda r0+1
	adc #>$2000
	sta r0+1

	inx
	cpx #NUM_CODE_CHUNKS
	bne _code_unpack_loop

	; FINALLY! We can run the game.
	jmp GAME_START_ADDR	


BLAM:
	lda #$69
;	lda #2
;	sta MMU_IO_CTRL
;	ldx #0
_fooloop
;	txa
;	sta $C000
;	inx
	jmp _fooloop	
	
;-----------------------
; map memory bank N into slot 6 ($c000)
; a: bank number, N
mapbank
	; MMU_MEM_CTRL layout:  E.ee..aa
	;   E = edit enable
	;   ee = edit lut
	;   aa = active lut
	ldx	#$80
	stx MMU_MEM_CTRL
	sta MMU_MEM_BANK_6
	ldx #0
	stx MMU_MEM_CTRL
	rts


;-------------------
; copy a block of memory
; srcptr: src
; dstptr: dest
; len: number of bytes to copy
; trashes a,x,y
memory_copy:
	ldx len+1	; high byte, number of full 256-byte blocks
	beq _leftover
_copy256
	ldy #0 ; 256
_loop2
	lda (srcptr),y
	sta (dstptr),y
	iny
	bne _loop2
	inc srcptr+1	; src += 256
	inc dstptr+1	; dest += 256
	dex
	bne _copy256

	; copy the leftover bytes <256
_leftover
	ldy #0
_leftover_loop
	cpy len+0		; lo byte count
	beq _done
	lda (srcptr),y
	sta (dstptr),y
	iny
	jmp _leftover_loop
_done
	rts


;-----------------------
; unpack data chunk N into it's RAM bank
; a=chunk number
unpackdatachunk:
	; switch in FIRST_DATA_BANK+N to slot 6
	pha
	clc
	adc #FIRST_DATA_BANK
	jsr mapbank

	; read the src ptr from the chunk table
	pla
	asl		; * 2 bytes/chunk entry
	tax
	lda data_chunks,x
	sta lzsa_srcptr
	inx
	lda data_chunks,x
	sta lzsa_srcptr+1

	; unpack into slot 6 ($C000)
	lda #<$C000
	sta lzsa_dstptr	
	lda #>$C000
	sta lzsa_dstptr+1

	; decompress
	jsr lzsa2_unpack
	rts


;---------------
; copy code chunk N into it's own ram bank
; a: code chunk index
copycodechunk:
	; map the target bank into slot 6 ($c000)
	pha
	clc
	adc #FIRST_CODE_BANK
	jsr mapbank
	; copy the compressed chunk there

	; destpr = $c000
	lda #<$C000
	sta dstptr
	lda #>$C000
	sta dstptr+1

	; srcptr = beginning of compressed data
	pla
	asl
	asl	; each entry is 4 bytes
	tax
	lda code_chunks,x
	sta srcptr
	inx
	lda code_chunks,x
	sta srcptr+1
	inx

	; len = length of code chunk
	lda code_chunks,x
	sta len
	inx
	lda code_chunks,x
	sta len+1

	jsr memory_copy
	rts

	nop
	nop
	nop

	.include "decompress_faster_v2.asm"

	nop
	nop
	nop


	.endlogical
shifted_end

; the compressed data
spr16_00
	.binary "spr16_00.zbin"
spr16_00_end

spr16_01
	.binary "spr16_01.zbin"
spr16_01_end

spr16_02
	.binary "spr16_02.zbin"
spr16_02_end

spr16_03
	.binary "spr16_03.zbin"
spr16_03_end

spr32
	.binary "spr32.zbin"
spr32_end

spr64x8
	.binary "spr64x8.zbin"
spr64x8_end

chars
	.binary "chars.zbin"
chars_end

; The compressed code
code00
	.binary "game_00.zbin"
code00_end
code01
	.binary "game_01.zbin"
code01_end
code02
	.binary "game_02.zbin"
code02_end
code03
	.binary "game_03.zbin"
code03_end
code04
	.binary "game_04.zbin"
code04_end


NUM_DATA_CHUNKS = 6
FIRST_DATA_BANK = 8
; table of startaddr
data_chunks:
	.word spr16_00, spr16_01, spr16_02, spr16_03, spr32, spr64x8

NUM_CODE_CHUNKS = 5
FIRST_CODE_BANK = 8 + NUM_DATA_CHUNKS
; table of startaddr,length pairs
code_chunks:
	.word code00, code00_end - code00
	.word code01, code01_end - code01
	.word code02, code02_end - code02
	.word code03, code03_end - code03
	.word code04, code04_end - code04

