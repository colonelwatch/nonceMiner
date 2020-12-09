# nonceMiner

A quick casual project I wrote to try out applying hash midstate caching on the cryptocurrency project [duino-coin](https://github.com/revoxhere/duino-coin). The idea is to cache the SHA-1 algorithm's state after it processes the prefix (creating the midstate) then to finish the hash with the guessed nonces repeatedly, but while reusing the midstate, until the right nonce is found. This saves a lot of calculation, and it's already being used in Bitcoin mining.

Running Multiprocess_nonceMiner.py with `N_PROCESSES = 128`, my i7-8550U peaked at 16 MH/s. In other words, it's probably the fastest DUCO-S1 miner around, barring GPUs. If you're monitoring your own hash rate, make sure you're getting a command line output every two seconds, or else the rate inaccurately appears high (I've had lag for some reason when I pinned the CPU too hard and for too long).

The "src" folder contains C code that implements the shortcut and also a Cython wrapper, so C/C++ programmers can use those source files. This includes an unmodified copy of [sha1](https://github.com/clibs/sha1). For Python programmers, I also attached a precompiled extension (nonceMiner.cp38-win_amd64.pyd) in the root of this repo—the examples hook into this. It's only built in Windows though, so run `python setup.py build_ext --inplace` if you have another OS.
