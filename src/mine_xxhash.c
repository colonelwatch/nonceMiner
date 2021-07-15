#include "mine_xxhash.h"

int compare_xxhash(
    const unsigned char hex_digest[2*XXHASH_SIZE],
    XXH64_hash_t byte_digest)
{
    for(int i = 0; i < XXHASH_SIZE; i++){
        // Converts from byte to hexadecimal
        unsigned char byte = (byte_digest >> 8*(XXHASH_SIZE-i-1)) & 0xff;
        char msb = "0123456789abcdef"[byte>>4],
             lsb = "0123456789abcdef"[byte&0x0f];
        if(msb != hex_digest[2*i] || lsb != hex_digest[2*i+1]) return 0;
    }
    return 1;
}

long mine_xxhash(
    const unsigned char *input_prefix,
    int prefix_length,
    const unsigned char target_hexdigest[XXHASH_SIZE*2],
    int difficulty)
{
    long maximum = 100*difficulty+1;
    XXH64_state_t *base_state = XXH64_createState(), *temp_state = XXH64_createState();
    XXH64_reset(base_state, 2811);
    XXH64_update(base_state, input_prefix, prefix_length);
    
    struct counter_state state;
    for(init_counter_state(&state); state.as_long_integer < maximum; increment_counter(&state)){
        XXH64_copyState(temp_state, base_state);
        XXH64_update(temp_state, state.buf+12-state.length, state.length);
        XXH64_hash_t temp_hash = XXH64_digest(temp_state);
        if(compare_xxhash(target_hexdigest, temp_hash)){
            XXH64_freeState(base_state);
            XXH64_freeState(temp_state);
            return state.as_long_integer;
        }
    }

    XXH64_freeState(base_state);
    XXH64_freeState(temp_state);
    return -1;
}