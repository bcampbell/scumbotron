# Assumes working dir is main scumbotron dir, not f256
#
# Gathers all the binary files needed by the loader/unpacker
# and splits them into 8KB chunks and compresses them.
# output files are put into build dir ready for building the loader.

set -e 

t=`mktemp -d`
BLD=build_f256k
EXP=export_f256k

# game.bin
# TODO: check size of game.bin and warn if 5 chunks isn't correct!
split -b 8192 -d --additional-suffix .bin $BLD/game.bin $t/game_
lzsa -f2 -r $t/game_00.bin $BLD/game_00.zbin
lzsa -f2 -r $t/game_01.bin $BLD/game_01.zbin
lzsa -f2 -r $t/game_02.bin $BLD/game_02.zbin
lzsa -f2 -r $t/game_03.bin $BLD/game_03.zbin
lzsa -f2 -r $t/game_04.bin $BLD/game_04.zbin

# spr16.bin
split -b 8192 -d --additional-suffix .bin $EXP/spr16.bin $t/spr16_
lzsa -f2 -r $t/spr16_00.bin $BLD/spr16_00.zbin
lzsa -f2 -r $t/spr16_01.bin $BLD/spr16_01.zbin
lzsa -f2 -r $t/spr16_02.bin $BLD/spr16_02.zbin
lzsa -f2 -r $t/spr16_03.bin $BLD/spr16_03.zbin

# spr32.bin
lzsa -f2 -r $EXP/spr32.bin $BLD/spr32.zbin

# spr64x8.bin
lzsa -f2 -r $EXP/spr64x8.bin $BLD/spr64x8.zbin

# chars.bin
lzsa -f2 -r $EXP/chars.bin $BLD/chars.zbin

