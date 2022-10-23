
game.prg: game.c sys_cx16.c sys_cx16.h gfx.c irq.s glue.s
	mos-cx16-clang -Os -o game.prg game.c sys_cx16.c gfx.c irq.s glue.s

gfx.c: sprites16.png
	echo "#include <stdint.h>" >gfx.c
	echo "uint8_t palette[16*2] = {" >>gfx.c
	tools/png2sprites -p sprites16.png >>gfx.c
	echo "};" >>gfx.c
	echo "uint8_t sprites[16*16*16] = {" >>gfx.c
	tools/png2sprites -n 16 -w 16 -h 16 sprites16.png >>gfx.c
	echo "};" >>gfx.c

run: game.prg
	x16emu -prg game.prg -run


