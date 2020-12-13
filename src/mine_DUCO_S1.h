#include <stdlib.h>
#include <stdio.h>

#include "sha1.h"

#define HASH_SIZE 20

// private functions
long _get_divisor(long x);
unsigned char _char_hex_to_byte(const char hex[2]);

// public functions
SHA1_CTX set_sha1_base(
    const char input_prefix[HASH_SIZE*2]);

SHA1_CTX update_sha1_base(
    SHA1_CTX base_ctx,
    const unsigned char byte);

void get_sha1_deriv(
    SHA1_CTX base_ctx,
    unsigned char hash[HASH_SIZE],
    long nonce);

int compare_hash(
    const char hex_digest[HASH_SIZE*2],
    const unsigned char byte_digest[HASH_SIZE]);

long mine_DUCO_S1(
    const char target_hash[HASH_SIZE*2],
    const unsigned char hash_prefix[HASH_SIZE*2],
    int difficulty);