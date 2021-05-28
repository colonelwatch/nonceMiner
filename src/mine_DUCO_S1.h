#include <stdlib.h>
#include <openssl/sha.h>

#include "counter.h"

#define HASH_SIZE 20

// Unsigned char is used here to align with Python UTF-8 encoding, but 
//  char arrays are also okay because letters and numbers are encoded
//  the same between UTF-8 and ASCII

#ifdef __cplusplus  
extern "C" { 
#endif

/*
 *  compare_hash - Compares a byte string and a hex digest, returning 1 if matched
 *  
 *  Compares two hex characters from the hex digest with the corresponding byte in the byte
 *  string for every byte, from left to right. However, it will immediately return 0 if 
 *  there is a difference early on.
 * */
int compare_hash(
    const unsigned char hex_digest[HASH_SIZE*2],
    const unsigned char byte_digest[HASH_SIZE]);

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
    const unsigned char target_hexdigest[HASH_SIZE*2],
    int difficulty);

#ifdef __cplusplus 
} 
#endif 