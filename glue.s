; Assorted glue asm

.global inp_joystate
.global inp_tick

; kernal
joystick_get=$FF56

inp_joystate: .short 0

inp_tick:
	; update joystate
	lda #0
	jsr joystick_get
	sta inp_joystate+0
	stx inp_joystate+1
	rts



