#include "mine_DUCO_S1.h"

long _get_divisor(long x){
    if(x == 0) return 1; // In case a single zero digit is recieved
    long temp = 1;
    while(x >= temp) temp *= 10;
    return temp / 10;
}

void set_sha1_base(
    SHA1_CTX *ctx_ptr,
    const unsigned char input_prefix[HASH_SIZE*2])
{
    SHA1Init(ctx_ptr);
    for (int i = 0; i < HASH_SIZE*2; i++)
        SHA1Update(ctx_ptr, (const unsigned char*)&input_prefix[i], 1);
}

void modify_sha1_ctx(SHA1_CTX *ctx_ptr, long nonce){
    for(long divisor = _get_divisor(nonce); divisor != 0; divisor /= 10){
        const unsigned char digit = (unsigned char)((nonce%(divisor*10))/divisor)+'0';
        SHA1Update(ctx_ptr, &digit, 1);
    }
}

void complete_sha1_hash(unsigned char hash[HASH_SIZE], SHA1_CTX *ctx_ptr){
    SHA1Final(hash, ctx_ptr); // Wipes context at ctx_ptr when complete
}

int compare_hash(
    const unsigned char hex_digest[2*HASH_SIZE],
    const unsigned char byte_digest[HASH_SIZE])
{
    for(int i = 0; i < HASH_SIZE; i++){
        char msb = "0123456789abcdef"[byte_digest[i]>>4],
             lsb = "0123456789abcdef"[byte_digest[i]&0x0f];
        if(msb != hex_digest[2*i] || lsb != hex_digest[2*i+1]) return 0;
    }
    return 1;
}

long mine_DUCO_S1(
    const unsigned char target_hexdigest[HASH_SIZE*2],
    const unsigned char input_prefix[HASH_SIZE*2],
    int difficulty)
{
    SHA1_CTX base_ctx;
    set_sha1_base(&base_ctx, input_prefix);
    long maximum = 100*difficulty+1;
    
    for(long i = 0; i < maximum; i++){
        unsigned char temp_hash[HASH_SIZE];
        SHA1_CTX temp_ctx = base_ctx;
        modify_sha1_ctx(&temp_ctx, i);
        complete_sha1_hash(temp_hash, &temp_ctx);
        if(compare_hash(target_hexdigest, temp_hash)) return i;
    }
    return -1;
}

long mine_DUCO_S1_extend_cache(
    const unsigned char target_hexdigest[HASH_SIZE*2],
    const unsigned char input_prefix[HASH_SIZE*2],
    int difficulty)
{
    SHA1_CTX base_ctx;
    set_sha1_base(&base_ctx, input_prefix);
    long maximum = 100*difficulty+1;
    SHA1_CTX *cache_ctx = (SHA1_CTX*)malloc(maximum*sizeof(SHA1_CTX)/10);
    
    for(long i = 0; i < maximum; i++){
        unsigned char temp_hash[HASH_SIZE];
        SHA1_CTX temp_ctx;
        if(i < 10){
            temp_ctx = base_ctx;
            modify_sha1_ctx(&temp_ctx, i);
        }
        else{
            temp_ctx = cache_ctx[i/10];
            modify_sha1_ctx(&temp_ctx, i%10);
        }
        if(i < maximum/10) cache_ctx[i] = temp_ctx;
        complete_sha1_hash(temp_hash, &temp_ctx);
        if(compare_hash(target_hexdigest, temp_hash)){
            free(cache_ctx);
            return i;
        }
    }
    free(cache_ctx);
    return -1;
}