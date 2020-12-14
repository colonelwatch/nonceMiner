#include <stdlib.h>
#include <stdio.h>

#include "sha1.h"

#define HASH_SIZE 20

// Unsigned char is used here to align with Python UTF-8 encoding, but 
//  char arrays are also okay because letters and numbers are encoded
//  the same between UTF-8 and ASCII

// private functions
long _get_divisor(long x);
unsigned char _char_hex_to_byte(const unsigned char hex[2]);

// public functions
SHA1_CTX set_sha1_base(
    const unsigned char input_prefix[HASH_SIZE*2]);

SHA1_CTX update_sha1_base(
    SHA1_CTX base_ctx,
    const unsigned char byte);

void modify_sha1_ctx(
    SHA1_CTX *ctx_ptr,
    long nonce);

void complete_sha1_hash(
    unsigned char hash[HASH_SIZE],
    SHA1_CTX *ctx_ptr);

int compare_hash(
    const unsigned char hex_digest[HASH_SIZE*2],
    const unsigned char byte_digest[HASH_SIZE]);

long mine_DUCO_S1(
    const unsigned char target_hash[HASH_SIZE*2],
    const unsigned char hash_prefix[HASH_SIZE*2],
    int difficulty);

long mine_DUCO_S1_extend_cache(
    const unsigned char target_hexdigest[HASH_SIZE*2],
    const unsigned char input_prefix[HASH_SIZE*2],
    int difficulty);