
game.bin is just the game code - it assumes all the graphics data has been
loaded into upper ram banks, ready to go.

scumbotron.asm is the unpacker - it contains compressed versions of game.bin
and the graphics data and unpacks them on startup then starts game.bin.

