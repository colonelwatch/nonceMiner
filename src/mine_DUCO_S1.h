#ifndef MINE_DUCO_S1_H
#define MINE_DUCO_S1_H

#include <stdlib.h>
#include <openssl/sha.h>

#include "utils/counter.h"
#include "utils/opencl.h"

#define DUCO_S1_SIZE 20

// Unsigned char is used here to align with Python UTF-8 encoding, but 
//  char arrays are also okay because letters and numbers are encoded
//  the same between UTF-8 and ASCII

#ifdef __cplusplus  
extern "C" { 
#endif

/*
 *  compare_DUCO_S1 - Compares a byte string and a hex digest, returning 1 if matched
 *  
 *  Compares two hex characters from the hex digest with the corresponding byte in the byte
 *  string for every byte, from left to right. However, it will immediately return 0 if 
 *  there is a difference early on.
 * */
int compare_DUCO_S1(
    const unsigned char hex_digest[DUCO_S1_SIZE*2],
    const unsigned char byte_digest[DUCO_S1_SIZE]);

/*
 *  mine_DUCO_S1 - Returns the nonce that generated target_hexdigest
 *  
 *  input_prefix should point to the beginning of the prefix. Because the prefix can be 
 *  either 40 or 16 characters long, the prefix_length parameter is exposed. Pass in 
 *  whichever is the correct length. target_hexdigest should point to the beginning of the
 *  target hex digest.
 * */
long mine_DUCO_S1(
    const unsigned char *input_prefix,
    int prefix_length,
    const unsigned char target_hexdigest[DUCO_S1_SIZE*2],
    int difficulty);

/*
 *  mine_DUCO_S1_OpenCL - Returns the nonce that generated target_hexdigest
 *  
 *  input_prefix should point to the beginning of the prefix. Because the prefix can be 
 *  either 40 or 16 characters long, the prefix_length parameter is exposed. Pass in 
 *  whichever is the correct length. target_hexdigest should point to the beginning of the
 *  target hex digest.
 * 
 *  Builds and executes an OpenCL kernel on the GPU specified by the ctx.
 * */
long mine_DUCO_S1_OpenCL(
    const unsigned char *input_prefix,
    int prefix_length,
    const unsigned char target_hexdigest[DUCO_S1_SIZE*2],
    int difficulty,
    worker_ctx *ctx);

#ifdef __cplusplus 
} 
#endif 

#endif