.global irq_init
.global tick
.global waitvbl


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


waitvbl:
	lda tick
.loop:
	cmp tick
	beq .loop
	rts

