; Assorted glue asm

;.global inp_joystate
;.global inp_tick

; kernal
;joystick_get=$FF56

;inp_joystate: .byte 0,0

;inp_tick:
;	; update joystate
;	lda #0
;	jsr joystick_get
;	sta inp_joystate+0
;	stx inp_joystate+1
;	rts


; uint8_t* cx16_k_memory_decompress(uint8_t* src, uint8_t* dest);
;
; Kernel signature: void memory_decompress(word input: r0, inout word output: r1);
; Purpose: Decompress an LZSA2 block
; Call address: $FEED

__memory_decompress = $FEED

__rc2 = $04
__rc3 = $05
__rc4 = $06
__rc5 = $07

.global cx16_k_memory_decompress
cx16_k_memory_decompress:
;	lda mos8(__rc5) ; desthi
;	pha
;	lda mos8(__rc4) ; destlo
;	pha
;	lda mos8(__rc3) ; srchi
;	pha
;	lda mos8(__rc2)	; srclo
	lda __rc5 ; desthi
	pha
	lda __rc4 ; destlo
	pha
	lda __rc3 ; srchi
	pha
	lda __rc2	; srclo


	sta $02	; srclo
	pla
	sta $03	; srchi
	pla
	sta $04	; destlo
	pla
	sta $05	; desthi

	jmp __memory_decompress


