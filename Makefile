SRCS=game.c gob.c player.c sys_cx16.c irq.s glue.s export/palette.c export/spr16.c export/spr32.c
INCS=gob.h player.h sys_cx16.h

game.prg: $(SRCS) $(INCS)
	mos-cx16-clang -Os -o game.prg $(SRCS)

# graphics exporting
export/palette.bin: sprites16.png
	tools/png2sprites -palette -out $@ -fmt bin $<

export/spr16.bin: sprites16.png
	tools/png2sprites -w 16 -h 16 -bpp 4 -num 64 -out $@ -fmt bin $<

export/spr32.bin: sprites32.png
	tools/png2sprites -w 32 -h 32 -bpp 4 -num 8 -out $@ -fmt bin $<

%.zbin: %.bin
	lzsa -f2 -r $< $@

%.c: %.bin
	xxd -i $< $@

run: game.prg
	x16emu -prg game.prg -run


