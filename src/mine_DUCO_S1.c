#include "mine_DUCO_S1.h"

long _get_divisor(long x){
    if(x == 0) return 1; // In case a single zero digit is recieved
    long temp = 1;
    while(x >= temp) temp *= 10;
    return temp / 10;
}

// 0 index holds upper bits and 1 index holds lower bits
unsigned char _char_hex_to_byte(const unsigned char hex[2]){
    unsigned char sum = 0;
    switch(hex[0]){
        case '1': sum += 1<<4; break;
        case '2': sum += 2<<4; break;
        case '3': sum += 3<<4; break;
        case '4': sum += 4<<4; break;
        case '5': sum += 5<<4; break;
        case '6': sum += 6<<4; break;
        case '7': sum += 7<<4; break;
        case '8': sum += 8<<4; break;
        case '9': sum += 9<<4; break;
        case 'a': sum += 10<<4; break;
        case 'b': sum += 11<<4; break;
        case 'c': sum += 12<<4; break;
        case 'd': sum += 13<<4; break;
        case 'e': sum += 14<<4; break;
        case 'f': sum += 15<<4; break;
    }
    switch(hex[1]){
        case '1': sum += 1; break;
        case '2': sum += 2; break;
        case '3': sum += 3; break;
        case '4': sum += 4; break;
        case '5': sum += 5; break;
        case '6': sum += 6; break;
        case '7': sum += 7; break;
        case '8': sum += 8; break;
        case '9': sum += 9; break;
        case 'a': sum += 10; break;
        case 'b': sum += 11; break;
        case 'c': sum += 12; break;
        case 'd': sum += 13; break;
        case 'e': sum += 14; break;
        case 'f': sum += 15; break;
    }
    return sum;
}

SHA1_CTX set_sha1_base(
    const unsigned char input_prefix[HASH_SIZE*2])
{
    SHA1_CTX base_ctx;
    SHA1Init(&base_ctx);
    for (int i = 0; i < HASH_SIZE*2; i++)
        SHA1Update(&base_ctx, (const unsigned char*)&input_prefix[i], 1);
    return base_ctx;
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
        unsigned char byte_val = _char_hex_to_byte(&hex_digest[2*i]);
        if(byte_val != byte_digest[i]) return 0;
    }
    return 1;
}

long mine_DUCO_S1(
    const unsigned char target_hexdigest[HASH_SIZE*2],
    const unsigned char input_prefix[HASH_SIZE*2],
    int difficulty)
{
    SHA1_CTX base_ctx = set_sha1_base(input_prefix);
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
    SHA1_CTX base_ctx = set_sha1_base(input_prefix);
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