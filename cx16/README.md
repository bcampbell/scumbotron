This directory contains all the cx16-specific code.
The aim is to fit the whole game into a single .prg file.
These load into BASIC RAM, a space of about 38KB, from
$0800 to $9EFF.

NOTE: the code runs right up to the BASIC RAM limit...
It's possible a different version of llvm-mos will overrun if it generates
less compact code!
The linker is pretty good about warning if this happens.


## BUG - The cx16 version will currently not build!

It now overruns the BASIC RAM limit, and needs some fiddling to get it back down to size.
I suspect newer llvm-mos-sdk cx16 library/kernal bindings bloat things out a bit,
but haven't really investigated too much yet.

## cx16 build requirements

- llvm-mos-sdk
- golang (to compile the `png2sprites` tool)
- lzsa (compression tool)
- tass assembler
- x16emu (optional, for running the game)

## build instructions

Assumes a unixy system. Would build fine on windows if you've got everything
installed, although you might need to fiddle about to replicate
`cx16/pack.sh`, which is a bash script.

```
$ make -f Makefile.cx16
```

Run in the emulator:

```
$ make -f Makefile.cx16 run
```

## details

The game code itself easily takes up the whole 38KB BASIC space, leaving no
room for graphics.
So we separate out the code and graphics, compress them both separately,
then package them together in a .prg file, with a little bit of code which:
1. decompresses the graphics directly into VERA RAM
2. decompresses the game code into BASIC RAM
3. jumps to the game code

game.prg is the code-only part. It just assumes that the graphics are
already in VERA RAM by the time it starts.
You could load and run game.prg by itself and it'd work, but there wouldn't
be any graphic data - just whatever rubbish was already in VERA RAM.

The graphics data is comprised of the .bin files in the `export_cx16` dir.
These are generated from the png files by the `png2sprites` tool.

scumbotron.asm is the code for the final scumbotron.prg. It includes the
compressed game.prg binary and the compressed graphics.
It needs to be compiled with tass. This is due to the unpacking routine
needed to be relocated out of the BASIC RAM before running. The tass
`.logical` psuedo-op does the job nicely.
Would be nicer to use the assembler that comes with llvm-mos, but I'm not
sure if it has an equivalent.

pack.sh is a nasty little script to build the final scumbotron.prg file.
There's a little hoop-jumping because we need to chop up the data into
8KB chunks before compressing it (because we use the banked RAM as a buffer,
and there's only 8KB of that mapped in at once).

