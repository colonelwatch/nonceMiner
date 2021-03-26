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
long _get_divisor(long x);

// public functions
void set_sha1_base(
    SHA_CTX *ctx_ptr,
    const unsigned char input_prefix[HASH_SIZE*2]);

SHA_CTX update_sha1_base(
    SHA_CTX base_ctx,
    const unsigned char byte);

void modify_sha1_ctx(
    SHA_CTX *ctx_ptr,
    long nonce);

void modify_sha1_ctx_one_digit(
    SHA_CTX *ctx_ptr,
    int nonce);

void complete_sha1_hash(
    unsigned char hash[HASH_SIZE],
    SHA_CTX *ctx_ptr);

int compare_hash(
    const unsigned char hex_digest[HASH_SIZE*2],
    const unsigned char byte_digest[HASH_SIZE]);

long mine_DUCO_S1(
    const unsigned char hash_prefix[HASH_SIZE*2],
    const unsigned char target_hash[HASH_SIZE*2],
    int difficulty);

long mine_DUCO_S1_extend_cache(
    const unsigned char input_prefix[HASH_SIZE*2],
    const unsigned char target_hexdigest[HASH_SIZE*2],
    int difficulty);

#ifdef __cplusplus 
} 
#endif 