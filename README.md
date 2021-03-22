# nonceMiner

A quick casual project I wrote to try out applying hash midstate caching on the cryptocurrency project [duino-coin](https://github.com/revoxhere/duino-coin). The idea is to cache the SHA-1 algorithm's state after it processes the prefix (creating the midstate) then to finish the hash with the guessed nonces repeatedly, but while reusing the midstate, until the right nonce is found. This saves a lot of calculation, and it's already being used in Bitcoin mining.

Aside from server bottlenecks, my i7-8550U averaged 22 to 25MH/s. In other words, it's probably the fastest DUCO-S1 miner around, barring GPUs. To try it out yourself, grab a release! There, you can find a pre-compiled Windows binary and instructions to compile for Linux below. When prompted about processess, 16 or 8 is recommended.

This program used to be written in Python with some C wrapped in Cython, but I have since rewritten it in pure C with native cross-platform compatibility.

## Running
The binary is dependent on OpenSSL. The light binary is availible for Windows on [slproweb.com](https://slproweb.com/products/Win32OpenSSL.html), and this usually comes pre-installed on Linux.

## Compiling
I compiled this before in Windows 10 and WSL2 (Ubuntu 20.04 LTS), so results outside these environments may vary.

Prerequisites: `gcc` (MinGW on Windows), `libssl-dev` (at least a development OpenSSL binary in Windows, I used the [slproweb.com](https://slproweb.com/products/Win32OpenSSL.html) copy, *not the "light" binary*)

1) Call `make nonceMiner` in the repo directory
2) Execute `./bin/nonceMiner` in the repo directory, or pull the compiled binary from `bin`

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
