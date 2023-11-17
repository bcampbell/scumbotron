# Scumbotron

A lost arcade game from the 80s.
Shoot all the things.

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

