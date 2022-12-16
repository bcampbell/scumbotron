.global irq_init
.global tick
.global waitvbl
.global inp_virtpad


VERA_L=$9f20

VERA_L=$9f20
VERA_M=$9f21
VERA_H=$9f22
VERA_D0=$9f23
VERA_D1=$9f24

VERA_CTRL=$9f25
VERA_IEN=$9f26
VERA_ISR=$9F27
VERA_VIDEO=$9f29


tick: .byte 0

old_handler: .short 0

irq_init:
	jsr keyhandler_init

	; save the old one
    lda $0314
    sta old_handler
    lda $0315
    sta old_handler+1
    sei
    lda #<irq_handler
    sta $0314
    lda #>irq_handler
    sta $0315

	lda #$01
	sta VERA_ISR
	sta VERA_IEN
    cli
    rts

irq_handler:
    ;Verify vblank interrupt
    lda VERA_ISR
    and #$01
    beq done

	ldx tick
	inx
	stx tick
    
	lda #$01
	sta VERA_ISR
done:
    jmp (old_handler)


keyhandler_init:
    sei
    lda #<keyhandler
    sta $032e
    lda #>keyhandler
    sta $032f
    cli
    rts


// Map PS/2 scancodes to gamepad bits
inp_virtpad_map:			  ;x,b,y,a
	;     up,  dn,  lf,  rt,  W,   S,   A,   D   
inp_virtpad_map_prefix:
	.byte $E0, $E0, $E0, $E0, $00, $00, $00, $00	// prefix
inp_virtpad_map_scancode:
	.byte $75, $72, $6B, $74, $1D, $1B, $1C, $23	// scancode
inp_virtpad_map_out_hi:
	.byte $08, $04, $02, $01, $00, $80, $40, $00	// scancode
inp_virtpad_map_out_lo:
	.byte $00, $00, $00, $00, $40, $00, $00, $80	// scancode

; cx16 pad bits:
;  .A, byte 0:      | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
;              SNES | B | Y |SEL|STA|UP |DN |LT |RT |
;
;  .X, byte 1:      | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
;              SNES | A | X | L | R | 1 | 1 | 1 | 1 |

; the virtual gamepad, driven by keyboard.
inp_virtpad: .short 0

keyhandler:
    ; .X: PS/2 prefix ($00, $E0 or $E1)
    ; .A: PS/2 scancode
    ; .C: clear if key down, set if key up event
    php         ;Save input on stack
    pha
    phx
	phy

	ldy #8	; size of inp_virtpad_map
    bcs kh_keyup

kh_keydown_loop:
	dey
	bmi kh_done
	cmp inp_virtpad_map_scancode,y
	bne kh_keydown_loop
	pha
	txa
	cmp inp_virtpad_map_prefix,y
	beq kh_keydown_match
	pla
	jmp kh_keydown_loop
kh_keydown_match:
	pla
	; set the appropriate bit
	lda inp_virtpad_map_out_hi,y
	ora inp_virtpad+0
	sta inp_virtpad+0
	lda inp_virtpad_map_out_lo,y
	ora inp_virtpad+1
	sta inp_virtpad+1
	jmp kh_done

kh_keyup:
	dey
	bmi kh_done
	cmp inp_virtpad_map_scancode,y
	bne kh_keyup
	pha
	txa
	cmp inp_virtpad_map_prefix,y
	beq kh_keyup_match
	pla
	jmp kh_keyup
kh_keyup_match:
	pla
	; clear the appropriate bit
	lda inp_virtpad_map_out_hi,y
	eor #$FF
	and inp_virtpad+0
	sta inp_virtpad+0
	lda inp_virtpad_map_out_lo,y
	eor #$FF
	and inp_virtpad+1
	sta inp_virtpad+1

kh_done:
	ply
    plx		;Restore input
    pla
    plp
    rts

waitvbl:
	lda tick
.loop:
	cmp tick
	beq .loop
	rts

