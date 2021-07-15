#define XXH_INLINE_ALL
#include "utils/xxhash.h"
#include "utils/counter.h"

#define XXHASH_SIZE 8

// Unsigned char is used here to align with Python UTF-8 encoding, but 
//  char arrays are also okay because letters and numbers are encoded
//  the same between UTF-8 and ASCII

#ifdef __cplusplus  
extern "C" { 
#endif

/*
 *  compare_xxhash - Compares a 64-bit integer and a hex digest, returning 1 if matched
 *  
 *  Compares two hex characters from the hex digest with the corresponding byte in the byte
 *  string for every byte, from left to right. However, it will immediately return 0 if 
 *  there is a difference early on.
 * */
int compare_xxhash(
    const unsigned char hex_digest[XXHASH_SIZE*2],
    XXH64_hash_t byte_digest);

/*
 *  mine_xxhash - Returns the nonce that generated target_hexdigest
 *  
 *  input_prefix should point to the beginning of the prefix. Because the prefix can be 
 *  either 40 or 16 characters long, the prefix_length parameter is exposed. Pass in 
 *  whichever is the correct length. target_hexdigest should point to the beginning of the
 *  target hex digest.
 * */
long mine_xxhash(
    const unsigned char *input_prefix,
    int prefix_length,
    const unsigned char target_hexdigest[XXHASH_SIZE*2],
    int difficulty);

#ifdef __cplusplus 
} 
#endif 