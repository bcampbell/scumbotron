# Agfa build

To build:

$ make -f Makefile.agfa

Currently builds fine on Linux (and probably on windows and mac) with gcc.
But don't try to run - it'll all end in tears when it tries to hit the VERA registers :-)

## Toolchain

Not sure on intended C toolchain, but the makefile will need fiddling to set compiler and linker details.
Amiga and megadrive are working 68000 targets, which might provide useful examples.

## Graphics data

The makefile invokes png2sprites to export the graphics from png as part of the build.
png2sprites is written in golang. But I'll add the exported data files to git.
See dir `export_agfa/`.
The data files are just .c files which are compiled directly into the binary.

## Sound

The vera-based sound _should_ just work out of the box...

## Missing bits

You'll need a vblank timer (and a waitvbl() function).
I assume there'll be some interrupt handler. Can't remember if the VERA is wired in for interrupts or not...
The `tick` variable needs to increment each frame - either in the IRQ handler or in the main loop.
(I often just increment it in the IRQ, and have waitvbl() just busy-wait until it changes).

The input code is all stubbed out at the moment.
But even in that state, it _should_ just cycle through the title screen and
intro/story sequence, so if you see that OK it's all probably working fine.

The HUD will have the wrong character for the lives counter.
On the X16 I used the built-in petscii font, which has a heart character.
But my custom charset (chars.png) doesn't have it (yet). On the other
platforms I use a `*`, instead, but `gfx_vera.c` is still the x16 code.
(see the `plat_hud()` function).


 




