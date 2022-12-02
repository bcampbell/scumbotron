SRCS=game.c gob.c player.c sys_cx16.c irq.s glue.s _palette.c _sprites16.c _sprites32.c
INCS=gob.h player.h sys_cx16.h

game.prg: $(SRCS) $(INCS)
	mos-cx16-clang -Os -o game.prg $(SRCS)

_palette.c: sprites16.png
	echo "#include <stdint.h>" >$@
	echo "uint8_t palette[16*2] = {" >>$@
	tools/png2sprites -palette $< >>$@
	echo "};" >>$@

_sprites16.c: sprites16.png
	echo "#include <stdint.h>" >$@
	echo "uint8_t sprites16[64*16*8] = {" >>$@
	tools/png2sprites -w 16 -h 16 -bpp 4 -num 64 $< >>$@
	echo "};" >>$@

_sprites32.c: sprites32.png
	echo "#include <stdint.h>" >$@
	echo "uint8_t sprites32[8*32*16] = {" >>$@
	tools/png2sprites -w 32 -h 32 -bpp 4 -num 8 $< >>$@
	echo "};" >>$@


run: game.prg
	x16emu -prg game.prg -run


