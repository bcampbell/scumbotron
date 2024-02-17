Using the toolchain from the vscode-amiga-debug extension:
https://github.com/BartmanAbyss/vscode-amiga-debug

Install vscode and extension, but we won't be running through vscode at the moment.
But the debugging environment and profiling provided by the extension looks fantastic, so well worth exploring!

To run makefile, use env.sh to add the vscode extension toolchain to the path:

```
$ . amiga/env.sh
$ make -f Makefile.amiga
```

Using fs-uae emulator to test.
There's a minimal harddrive image in dh0.
See Makefile.amiga run target for details.


