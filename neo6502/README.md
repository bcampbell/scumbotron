# issues

`game_render()` reentrancy in pause state screws up code generation and causes
FFFx memory to be blatted.
-> restrict pause to PLAYING state and call render fn directly.

neo6502 font is 6*8

