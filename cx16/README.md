This directory contains all the cx16-specific code.
The aim is to fit the whole game into a single .prg file.
These load into BASIC RAM, a space of about 38KB, from
$0800 to $9EFF.

## cx16 build requirements

- llvm-mos-sdk
- golang (to compile the `png2sprites` tool)
- lzsa (compression tool)
- tass assembler
- x16emu (optional, for running the game)

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

