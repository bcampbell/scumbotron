game.prg: game.c gfx.c irq.s glue.s
	mos-cx16-clang -Os -o game.prg game.c gfx.c irq.s glue.s


gfx.c: sprites16.png
	echo "#include <stdint.h>" >gfx.c
	echo "uint8_t sprites[16*16*16] = {" >>gfx.c
	tools/png2sprites -n 16 -w 16 -h 16 sprites16.png >>gfx.c
	echo "};" >>gfx.c
