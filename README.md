# Scumbotron

A lost arcade game from the 80s.
Shoot all the things.

[Play it online here](https://cx16forum.com/webemu/x16emu.html?manifest=/forum/download/file.php?id=2421) (Commander X16 version). 

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

- cx16: working
- sdl2: working
- megadrive: working, but has slowdowns
- nds: working, no sound
- gba: working, no sound
- amiga: working, no sound, a bit slow (Amiga 3000 is fine)
- neo6502: playable, incomplete, no sound
- pce: Broken. Suspect llvm-sdk-mos pce implementation needs tweaking.
- f256k: partial

