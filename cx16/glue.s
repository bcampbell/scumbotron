; Shims for various kernal functions we need, just until cx16 kernal stuff
; makes it into llvm-mos-sdk.
; see https://github.com/llvm-mos/llvm-mos-sdk/issues/113

__MOUSE_GET = $FF6B
__MOUSE_CONFIG = $FF68
__JOYSTICK_GET = $FF56
__JOYSTICK_SCAN = $FF53
__memory_decompress = $FEED

regbk = $0400


; There's an llvm-mos / cx16 calling convention mismatch.
; llvm-mos expects 16bit imaginary registers rs0 and rs10-rs15 to be preserved
; cx16 kernal ABI says r11-r15 can be trashed (which means the kernal code
; could use these registers at will).
; Proper solution is for llvm-mos to make sure rs10-rs15 are in locations
; the cx16 kernal won't touch. But for now, we'll just stash 'em when we make
; kernal calls! 

; save all the llvm-mos callee-saved imaginary registers
stashreg:
	pha
	; rs0
	lda __rc0
	sta regbk+0
	lda __rc1
	sta regbk+1
	; rs10
	lda __rc20
	sta regbk+2
	lda __rc21
	sta regbk+3
	; rs11
	lda __rc22
	sta regbk+4
	lda __rc23
	sta regbk+5
	; rs12
	lda __rc24
	sta regbk+6
	lda __rc25
	sta regbk+7
	; rs13
	lda __rc26
	sta regbk+8
	lda __rc27
	sta regbk+9
	; rs14
	lda __rc28
	sta regbk+10
	lda __rc29
	sta regbk+11
	; rs15
	lda __rc30
	sta regbk+12
	lda __rc31
	sta regbk+13

	pla
	rts

; restore all the llvm-mos callee-saved imaginary registers
unstashreg:
	pha
	; rs0
	lda regbk+0
	sta __rc0
	lda regbk+1
	sta __rc1
	; rs10
	lda regbk+2
	sta __rc20
	lda regbk+3
	sta __rc21
	; rs11
	lda regbk+4
	sta __rc22
	lda regbk+5
	sta __rc23
	; rs12
	lda regbk+6
	sta __rc24
	lda regbk+7
	sta __rc25
	; rs13
	lda regbk+8
	sta __rc26
	lda regbk+9
	sta __rc27
	; rs14
	lda regbk+10
	sta __rc28
	lda regbk+11
	sta __rc29
	; rs15
	lda regbk+12
	sta __rc30
	lda regbk+13
	sta __rc31

	pla
	rts

; uint8_t* cx16_k_memory_decompress(uint8_t* src, uint8_t* dest);
;
; Kernel signature: void memory_decompress(word input: r0, inout word output: r1);
; Purpose: Decompress an LZSA2 block
; Call address: $FEED

.global cx16_k_memory_decompress
cx16_k_memory_decompress:
	jsr stashreg
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

	jsr __memory_decompress

	jsr unstashreg
	rts

;
; typedef struct { int x, y; } mouse_pos_t;
; unsigned char cx16_k_mouse_get(mouse_pos_t *mouse_pos_ptr);	// returns mouse button byte
;                                             rc2/3
;
; https://github.com/X16Community/x16-docs/blob/master/X16%20Reference%20-%2004%20-%20KERNAL.md#function-name-mouse_get
;
.global cx16_k_mouse_get
cx16_k_mouse_get:
	jsr stashreg
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
	jsr unstashreg
	rts


;
; void cx16_k_mouse_config(unsigned char showmouse, unsigned char xsize8, unsigned char ysize8);
;                                        a                        x                     rc2
;
; https://github.com/X16Community/x16-docs/blob/master/X16%20Reference%20-%2004%20-%20KERNAL.md#function-name-mouse_config
;
.global cx16_k_mouse_config
cx16_k_mouse_config:
	jsr stashreg
				; a = showmouse (already set)
				; x = xsize8 (already set)
	ldy	__rc2		; y = ysize8
	jsr	__MOUSE_CONFIG
	jsr unstashreg
	rts


;
; long cx16_k_joystick_get(unsigned char sticknum); // returns $YYYYXXAA (see docs, result negative if joystick not present)
;
; https://github.com/X16Community/x16-docs/blob/master/X16%20Reference%20-%2004%20-%20KERNAL.md#function-name-joystick_get
;
.global cx16_k_joystick_get
cx16_k_joystick_get:
	jsr stashreg
	jsr	__JOYSTICK_GET
	sty	__rc2
	sty	__rc3
	jsr unstashreg
	rts


;
; void cx16_k_joystick_scan(void);
;
; https://github.com/X16Community/x16-docs/blob/master/X16%20Reference%20-%2004%20-%20KERNAL.md#function-name-joystick_scan
;
.global cx16_k_joystick_scan
cx16_k_joystick_scan:
	jsr stashreg
	jsr __JOYSTICK_SCAN
	jsr unstashreg
	rts

