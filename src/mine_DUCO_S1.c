#include "mine_DUCO_S1.h"

long _get_divisor(long x){
    if(x == 0) return 1; // In case a single zero digit is received
    long temp = 1;
    while(x >= temp) temp *= 10;
    return temp / 10;
}

void set_sha1_base(
    SHA_CTX *ctx_ptr,
    const unsigned char *input_prefix,
    int prefix_length)
{
    SHA1_Init(ctx_ptr);
    SHA1_Update(ctx_ptr, input_prefix, prefix_length);
}

void modify_sha1_ctx(SHA_CTX *ctx_ptr, long nonce){
    for(long divisor = _get_divisor(nonce); divisor != 0; divisor /= 10){
        const unsigned char digit = (unsigned char)((nonce%(divisor*10))/divisor)+'0';
        SHA1_Update(ctx_ptr, &digit, 1);
    }
}

void modify_sha1_ctx_one_digit(SHA_CTX *ctx_ptr, int nonce){
    const unsigned char digit = nonce+'0';
    SHA1_Update(ctx_ptr, &digit, 1);
}

void modify_sha1_ctx_two_digits(SHA_CTX *ctx_ptr, int nonce){
    const unsigned char digits[2] = {
        nonce/10 + '0',
        nonce%10 + '0'
    };
    SHA1_Update(ctx_ptr, digits, 2);
}

void complete_sha1_hash(unsigned char hash[HASH_SIZE], SHA_CTX *ctx_ptr){
    SHA1_Final(hash, ctx_ptr); // Wipes context at ctx_ptr when complete
}

int compare_hash(
    const unsigned char hex_digest[2*HASH_SIZE],
    const unsigned char byte_digest[HASH_SIZE])
{
    for(int i = 0; i < HASH_SIZE; i++){
        // Converts from byte to hexadecimal
        char msb = "0123456789abcdef"[byte_digest[i]>>4],
             lsb = "0123456789abcdef"[byte_digest[i]&0x0f];
        if(msb != hex_digest[2*i] || lsb != hex_digest[2*i+1]) return 0;
    }
    return 1;
}

long mine_DUCO_S1(
    const unsigned char *input_prefix,
    int prefix_length,
    const unsigned char target_hexdigest[HASH_SIZE*2],
    int difficulty)
{
    SHA_CTX base_ctx;
    set_sha1_base(&base_ctx, input_prefix, prefix_length);
    long maximum = 100*difficulty+1;
    
    for(long i = 0; i < maximum; i++){
        unsigned char temp_hash[HASH_SIZE];
        SHA_CTX temp_ctx = base_ctx;
        modify_sha1_ctx(&temp_ctx, i);
        complete_sha1_hash(temp_hash, &temp_ctx);
        if(compare_hash(target_hexdigest, temp_hash)) return i;
    }
    return -1;
}

long mine_DUCO_S1_lookup(
    const unsigned char *input_prefix,
    int prefix_length,
    const unsigned char target_hexdigest[HASH_SIZE*2],
    int difficulty)
{
    SHA_CTX base_ctx;
    set_sha1_base(&base_ctx, input_prefix, prefix_length);
    long maximum = 100*difficulty+1;

    struct counter_state state;
    init_counter_state(&state);
    for(long i = 0; i < maximum; i++){
        unsigned char temp_hash[HASH_SIZE];
        SHA_CTX temp_ctx = base_ctx;
        SHA1_Update(&temp_ctx, state.buf+12-state.length, state.length);
        complete_sha1_hash(temp_hash, &temp_ctx);
        if(compare_hash(target_hexdigest, temp_hash)) return i;
        increment_counter(&state);
    }
    return -1;
}

long mine_DUCO_S1_extend_cache(
    const unsigned char *input_prefix,
    int prefix_length,
    const unsigned char target_hexdigest[HASH_SIZE*2],
    int difficulty)
{
    SHA_CTX base_ctx;
    set_sha1_base(&base_ctx, input_prefix, prefix_length);
    long maximum = 100*difficulty+1;
    long cache_size = maximum/100;
    SHA_CTX *cache_ctx = (SHA_CTX*)malloc(cache_size*sizeof(SHA_CTX));
    
    for(long i = 0; i < maximum; i++){
        unsigned char temp_hash[HASH_SIZE];
        SHA_CTX temp_ctx;
        if(i < 10){ // If nothing but the context with prefix is cached yet...
            temp_ctx = base_ctx;
            modify_sha1_ctx_one_digit(&temp_ctx, i);
        }
        else if(i < 10*cache_size){ // If context with nonce upper digits (tens and beyond) is cached...
            temp_ctx = cache_ctx[i/10];
            modify_sha1_ctx_one_digit(&temp_ctx, i%10);
        }
        else{ // If context with nonce upper digits (hundreds and beyond) is cached...
            temp_ctx = cache_ctx[i/100];
            modify_sha1_ctx_two_digits(&temp_ctx, i%100);
        }
        if(i < cache_size) cache_ctx[i] = temp_ctx; // Cache the SHA1 context
        complete_sha1_hash(temp_hash, &temp_ctx);
        if(compare_hash(target_hexdigest, temp_hash)){
            free(cache_ctx);
            return i;
        }
    }
    free(cache_ctx);
    return -1;
}
