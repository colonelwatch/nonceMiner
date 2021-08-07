# nonceMiner

A casual project about running the DUCO-S1 algorithm, from revoxhere's [duino-coin](https://github.com/revoxhere/duino-coin) project, as damn fast as possible. Written in C, it leverages OpenSSL and exploits hash midstate caching on the CPU, and it uses OpenCL on the GPU (no hash midstate caching at the moment).

Not familiar with hash midstate caching? The idea is to cache the SHA-1 algorithm's state after it processes the prefix (creating the midstate) then to finish the hash with the guessed nonces repeatedly, but while reusing the midstate, until the right nonce is found. This saves a lot of calculation, and it's already being used in Bitcoin mining.

My laptop (Intel i7-8550U + UHD Graphics 630) averages about 60 MH/s. In other words, it's the fastest open-source DUCO-S1 miner around. Here are some other figures as July 25, 2021:

* Nvidia RTX 3070: 1.65 GH/s
* Nvidia GTX 1060 6GB: 684 MH/s
* Nvidia GTX 770: 343 MH/s
* Ryzen 5 3600: 106 MH/s
* Raspberry Pi 3B (CPU only): 4 MH/s

To try it out yourself, grab a release! There, you can find a pre-compiled Windows binary or instructions to compile for Linux below.

Disclaimer: This project does not represent my personal endorsement of duino-coin, just that their algorithm can be run really, *really* fast.

## Running
The nonceMiner binary is dependent on OpenSSL and OpenCL. The OpenSSL light binary is availible for Windows on [slproweb.com](https://slproweb.com/products/Win32OpenSSL.html), and this usually comes pre-installed on Linux. The OpenCL runtime should come with your graphics driver.

The binary is executable with the following options.

```
Options:
  -h    Print the help message
  -a    Specify the hash algorithm {DUCO_S1, xxhash}
  -i    Job difficulty/intensity {LOW, MEDIUM, NET, EXTREME}
  -o    Node URL of the format <host>:<port>
  -u    Username for mining
  -w    Identifier for mining
  -n    Program name for mining (overrides default name)
  -t    Number of threads
  -l    Hashrate limit per-thread, applies to CPU threads only (MH/s)
  -g    Spawn an OpenCL thread for each GPU detected
```

Please note that the xxhash mode is additionally up to five times faster, but the server imposes a per-thread hashrate limit of 0.9 MH/s. Exceeding this limit will cause shares to be rejected.

## Compiling
I compiled this before in Windows 10 and WSL2 (Ubuntu 20.04 LTS), so results outside these environments may vary.

Prerequisites: `gcc` (MinGW on Windows), `libssl-dev` (at least a development OpenSSL binary in Windows, I used the [slproweb.com](https://slproweb.com/products/Win32OpenSSL.html) copy, *not the "light" binary*), `libOpenCL` from your GPU drivers (applies to both Linux and Windows)

1) Call `make nonceMiner` in the repo directory
  * To compile without OpenCL, call `make nonceMiner CFLAGS='-O3 -Wall -D NO_OPENCL'` in the repo directory
2) Navigate to the bin folder with `cd bin`
3) Execute `./nonceMiner -u <your username here> -o <node URL here>`, or pull the compiled binary AND the OpenCL sources from `bin` (if compiled with OpenCL)

Additionally, the following test programs are available:
* `benchmark` - Evaluate DUCO-S1 and xxhash performance on your machine
* `nonceMiner_minimal` - A minimal implementation of DUCO-S1 with no error handling or multithreading
* `nonceMiner_minimal_xxhash` - A minimal implementation of xxhash with no error handling or multithreading
To compile one of them, just `make` with its program name.

## Attribution
This project uses SHA-1 code from OpenSSL.
```
/* ====================================================================
 * Copyright (c) 1998-2019 The OpenSSL Project.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * 3. All advertising materials mentioning features or use of this
 *    software must display the following acknowledgment:
 *    "This product includes software developed by the OpenSSL Project
 *    for use in the OpenSSL Toolkit. (http://www.openssl.org/)"
 *
 * 4. The names "OpenSSL Toolkit" and "OpenSSL Project" must not be used to
 *    endorse or promote products derived from this software without
 *    prior written permission. For written permission, please contact
 *    openssl-core@openssl.org.
 *
 * 5. Products derived from this software may not be called "OpenSSL"
 *    nor may "OpenSSL" appear in their names without prior written
 *    permission of the OpenSSL Project.
 *
 * 6. Redistributions of any form whatsoever must retain the following
 *    acknowledgment:
 *    "This product includes software developed by the OpenSSL Project
 *    for use in the OpenSSL Toolkit (http://www.openssl.org/)"
 *
 * THIS SOFTWARE IS PROVIDED BY THE OpenSSL PROJECT ``AS IS'' AND ANY
 * EXPRESSED OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE OpenSSL PROJECT OR
 * ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 * ====================================================================
 *
 * This product includes cryptographic software written by Eric Young
 * (eay@cryptsoft.com).  This product includes software written by Tim
 * Hudson (tjh@cryptsoft.com).
 *
 */
```

This project uses xxhash.
```
/*
 * xxHash - Extremely Fast Hash algorithm
 * Copyright (C) 2012-2020 Yann Collet
 *
 * BSD 2-Clause License (https://www.opensource.org/licenses/bsd-license.php)
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above
 *      copyright notice, this list of conditions and the following disclaimer
 *      in the documentation and/or other materials provided with the
 *      distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * You can contact the author at:
 *   - xxHash homepage: https://www.xxhash.com
 *   - xxHash source repository: https://github.com/Cyan4973/xxHash
 */
```

This project uses SHA1 OpenCL code from opencl_brute.
```
MIT License

Copyright (c) 2017 Bjoern Kerler

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
```