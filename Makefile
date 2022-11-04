SRCS=game.c gob.c sys_cx16.c irq.s glue.s _palette.c _sprites16.c _sprites32.c
INCS=gob.h sys_cx16.h

game.prg: $(SRCS) $(INCS)
	mos-cx16-clang -Os -o game.prg $(SRCS)

_palette.c: sprites16.png
	echo "#include <stdint.h>" >$@
	echo "uint8_t palette[16*2] = {" >>$@
	tools/png2sprites -p $< >>$@
	echo "};" >>$@

_sprites16.c: sprites16.png
	echo "#include <stdint.h>" >$@
	echo "uint8_t sprites16[64*16*16] = {" >>$@
	tools/png2sprites -n 64 -w 16 -h 16 $< >>$@
	echo "};" >>$@

_sprites32.c: sprites32.png
	echo "#include <stdint.h>" >$@
	echo "uint8_t sprites32[8*32*32] = {" >>$@
	tools/png2sprites -n 8 -w 32 -h 32 $< >>$@
	echo "};" >>$@


run: game.prg
	x16emu -prg game.prg -run


