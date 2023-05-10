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
__MOUSE_GET = $FF6B
__MOUSE_CONFIG = $FF68

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

;
; typedef struct { int x, y; } mouse_pos_t;
; unsigned char cx16_k_mouse_get(mouse_pos_t *mouse_pos_ptr);	// returns mouse button byte
;                                             rc2/3
;
; https://github.com/X16Community/x16-docs/blob/master/X16%20Reference%20-%2004%20-%20KERNAL.md#function-name-mouse_get
;
.global cx16_k_mouse_get
cx16_k_mouse_get:
	ldx	#__rc4		; x = temp pos
	jsr	__MOUSE_GET
	tax			; save buttons
	ldy	#4-1		; copy 4 byte pos to xy_ptr
copypos:
	lda	__rc4,y
	sta	(__rc2),y
	dey
	bpl	copypos
	txa			; return buttons
	rts


;
; void cx16_k_mouse_config(unsigned char showmouse, unsigned char xsize8, unsigned char ysize8);
;                                        a                        x                     rc2
;
; https://github.com/X16Community/x16-docs/blob/master/X16%20Reference%20-%2004%20-%20KERNAL.md#function-name-mouse_config
;
.global cx16_k_mouse_config
cx16_k_mouse_config:
				; a = showmouse (already set)
				; x = xsize8 (already set)
	ldy	__rc2		; y = ysize8
	jmp	__MOUSE_CONFIG

