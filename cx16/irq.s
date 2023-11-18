.global irq_init
.global tick
.global waitvbl
.global inp_keystates
.global inp_enabletextentry
.global inp_disabletextentry

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

old_handler: .byte 0,0

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

	; Enable vsync interrupt
    ; NOTE: I think this happens at the top of the screen (line 0).
    ; Would be better to use the line interrupt and trigger
	; at the bottom of the screen (line 480), so we get the whole
    ; vblank interval to render sprites without tearing.
    ; But I couldn't get it working. Something to revisit.
	lda #$01
	sta VERA_ISR
	sta VERA_IEN
    cli
    rts

irq_handler:
    ;Verify vsync interrupt
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


; Scancodes are $01..$7E, so we need 128/8 = 16 bytes to have a bit
; for each key.
inp_keystates:
	.byte 0,0,0,0, 0,0,0,0
	.byte 0,0,0,0, 0,0,0,0

; $00 to suppress passing scancode on to usual keymap/keybuffer handling
; $ff to enable passing key on to keybuffer
kh_retmask:
	.byte 0

inp_enabletextentry:
	lda #$ff
	sta kh_retmask
	rts

inp_disabletextentry:
	lda #$00
	sta kh_retmask
	rts

kh_masks: .byte 1,2,4,8,16,32,64,128

; see
; https://github.com/X16Community/x16-docs/blob/master/X16%20Reference%20-%2002%20-%20Editor.md#custom-keyboard-keynum-code-handler
keyhandler:

    pha
    phx
	phy

	tax

	; y = bit offset
	and #$07
	tay

	; x = byte offset	
	txa
	lsr
	lsr
	lsr
	tax
	cmp #$0f
	bpl	kh_keyup	; >=16 = keyup

kh_keydown:
	lda kh_masks,y
	ora inp_keystates,x
	sta inp_keystates,x
	jmp kh_done

kh_keyup:
	txa
	and #$0f
	tax
	lda kh_masks,y
	eor #$ff
	and inp_keystates,x
	sta inp_keystates,x

kh_done:
	ply
    plx		;Restore input
    pla
	; if text entry is disabled, return 0
	and kh_retmask
    rts

waitvbl:
	lda tick
waitvbl_loop:
	cmp tick
	beq waitvbl_loop
	rts

