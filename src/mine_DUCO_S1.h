#include <stdlib.h>
#include <openssl/sha.h>

#define HASH_SIZE 20

// Unsigned char is used here to align with Python UTF-8 encoding, but 
//  char arrays are also okay because letters and numbers are encoded
//  the same between UTF-8 and ASCII

#ifdef __cplusplus  
extern "C" { 
#endif

// private functions
/*
 *  _get_divisor - returns the largest divisor of x that is an order of 10
 *  
 *  Useful for splitting a number into digits from left to right. Only used in the basic
 *  mine_DUCO_S1, not mine_DUCO_S1_extend_cache.
 * */
long _get_divisor(long x);

// public functions
/*
 *  set_sha1_base - Initializes SHA1 context with the prefix
 *  
 *  input_prefix should point to the beginning of the prefix. Because the prefix can be 
 *  either 40 or 16 characters long, the prefix_length parameter is exposed. Pass in 
 *  whichever is the correct length.
 * */
void set_sha1_base(
    SHA_CTX *ctx_ptr,
    const unsigned char *input_prefix,
    int prefix_length);

/*
 *  modify_sha1_ctx - Updates SHA1 context with the full nonce
 *  
 *  Encodes each digit in ASCII then updates the context with that digit. Only used in the 
 *  basic mine_DUCO_S1, not mine_DUCO_S1_extend_cache.
 * */
void modify_sha1_ctx(
    SHA_CTX *ctx_ptr,
    long nonce);

/*
 *  modify_sha1_ctx_one_digit - Updates SHA1 context with a single digit
 *  
 *  Encodes passed digit in an UTF-8 char then updates the context with that digit.
 * */
void modify_sha1_ctx_one_digit(
    SHA_CTX *ctx_ptr,
    int nonce);

/*
 *  modify_sha1_ctx_two_digits - Updates SHA1 context with two digits
 *  
 *  Splits a two-digit number into two digits, encodes both in UTF-8 into a array, then 
 *  updates the context with that array.
 * */
void modify_sha1_ctx_two_digits(
    SHA_CTX *ctx_ptr,
    int nonce);

/*
 *  complete_sha1_hash - Completes the SHA1 hash, putting a byte string into hash[HASH_SIZE]
 *  
 *  A wrapper around SHA1_Final. Note: A byte-string is NOT an ordinary hex digest.
 * */
void complete_sha1_hash(
    unsigned char hash[HASH_SIZE],
    SHA_CTX *ctx_ptr);

/*
 *  complete_sha1_hash - Compares a byte string and a hex digest, returning 1 if matched
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
 *  This is the basic version. It uses a very small amount of memory but gives up a 
 *  significant amount of speed compared to mine_DUCO_S1_extend_cache. Still, it is
 *  faster compared to stock.
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

/*
 *  mine_DUCO_S1_extend_cache - Returns the nonce that generated target_hexdigest
 *  
 *  This is the fastest availible. In addition to caching the midstate initialized with 
 *  the prefix, it also caches midstates with the upper digits of the nonce, saving even 
 *  more work. Per thread running this function, it uses roughly 100MB of memory on the 
 *  EXTREME difficulty, but this can be higher or lower depending on job difficulty the 
 *  server sends.
 *  
 *  input_prefix should point to the beginning of the prefix. Because the prefix can be 
 *  either 40 or 16 characters long, the prefix_length parameter is exposed. Pass in 
 *  whichever is the correct length. target_hexdigest should point to the beginning of the
 *  target hex digest.
 * */
long mine_DUCO_S1_extend_cache(
    const unsigned char *input_prefix,
    int prefix_length,
    const unsigned char target_hexdigest[HASH_SIZE*2],
    int difficulty);

#ifdef __cplusplus 
} 
#endif 