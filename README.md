# nonceMiner

A quick casual project I wrote to try out applying hash midstate caching on the cryptocurrency project [duino-coin](https://github.com/revoxhere/duino-coin). The idea is to cache the SHA-1 algorithm's state after it processes the prefix (creating the midstate) then to finish the hash with the guessed nonces repeatedly, but while reusing the midstate, until the right nonce is found. This saves a lot of calculation, and it's already being used in Bitcoin mining.

Aside from server bottlenecks, my i7-8550U peaked at 18 MH/s running mp_miner.py. In other words, it's probably the fastest DUCO-S1 miner around, barring GPUs. To try it out yourself, grab a release! There, you can find a Windows-ready `mp_miner.exe`, or instructions to run on Linux. When prompted about processess, 16 or 8 is recommended. You can also find faster Arduino code (260 H/s) compatible with the latest duino-coin AVR miner.

The "src" folder contains C code that implements the shortcut and also a Cython wrapper, so C/C++ programmers can use those source files. This includes an unmodified copy of [sha1](https://github.com/clibs/sha1). Python programmers can grab the precompiled extension from a release.

## Compiling
On the other hand, cloning this repo will require compiling the program from scratch.

### Windows

Prerequisites: `cython`, `pyinstaller`, and Visual Studio Build Tools 2019
1) Call `make windows` in the nonceMiner directory
2) Call `make executable`
3) Execute `mp_miner.exe` in `./dist/`

### Linux

Prerequisites: `cython`, and `pyinstaller`
1) Call `make linux-gnu` in the nonceMiner directory
2) Call `make executable`
3) Execute `mp_miner` in `./dist/`

### macOS

*Currently not supported, but confirmed working by [@MattLancaster](https://github.com/MattLancaster)*

Prerequisites: `cython`
1) Call `make linux-gnu` in the nonceMiner directory
2) Execute `python3 mp_miner.py`