# Scumbotron

A lost arcade game from the 80s.
Shoot all the things.

## Building

Assumes a unixy system. Should build fine on windows if you've got appropriate tools installed.

Need golang to compile the `png2sprites` tool:

```
$ cd tools
$ go build png2sprites.go
```

Then run the Makefile for whichever platform you want:

```
$ make -f Makefile.sdl2
```

The produced executable/rom/whatever will be in the `build_<PLATFORM>` dir.


## Platform status

### cx16

Build broken (overruns BASIC RAM limit)

### sdl2

Working. (tested on windows and linux)

### megadrive

Working. A little slow. No sound.

### nds

Working and emu and real hw. No sound. Screen layout a little awkward.

### pce

Broken. Suspect llvm-sdk-mos pce implementation needs tweaking.

### f256k

Partial. Trouble running on emulation/IDE (older IDE releases include PGZ loader?).

