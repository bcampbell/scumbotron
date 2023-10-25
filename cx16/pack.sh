# Script to pack up game.prg and graphics data and unpack routine
# into final scumbotron.prg.
# Assumes working dir is main scumbotron dir, not cx16
set -e 

t=`mktemp -d`
BUILD=build_cx16
EXPORT=export_cx16

echo $t

# with game.prg:
# - strip off the first 2 bytes (the .prg format load address)
# - split into 8KB chunks
# - compress each chunk
tail -c +3 $BUILD/game.prg | split -b 8192 -d - $t/game
for chunk in $t/game0*
do
    lzsa -f2 -r $chunk $chunk.zbin
done

# lump up and compress gfx data
cat $EXPORT/spr16.bin $EXPORT/spr32.bin $EXPORT/spr64x8.bin >$t/gfx.bin
lzsa -f2 -r $t/gfx.bin $t/gfx.zbin

64tass -I $t -o $BUILD/scumbotron.prg cx16/scumbotron.asm

