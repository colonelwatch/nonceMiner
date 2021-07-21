#include "mine_DUCO_S1.h"

int compare_DUCO_S1(
    const unsigned char hex_digest[2*DUCO_S1_SIZE],
    const unsigned char byte_digest[DUCO_S1_SIZE])
{
    for(int i = 0; i < DUCO_S1_SIZE; i++){
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
    const unsigned char target_hexdigest[DUCO_S1_SIZE*2],
    int difficulty)
{
    SHA_CTX base_ctx;
    SHA1_Init(&base_ctx);
    SHA1_Update(&base_ctx, input_prefix, prefix_length);
    long maximum = 100*difficulty+1;

    struct counter_state state;
    for(init_counter_state(&state); state.as_long_integer < maximum; increment_counter(&state)){
        unsigned char temp_hash[DUCO_S1_SIZE];
        SHA_CTX temp_ctx = base_ctx;
        SHA1_Update(&temp_ctx, state.buf+12-state.length, state.length);
        SHA1_Final(temp_hash, &temp_ctx);
        if(compare_DUCO_S1(target_hexdigest, temp_hash)) return state.as_long_integer;
    }
    return -1;
}

// mine_DUCO_S1_OpenCL and advance_nonce_arr take majority (we're CPU bound...)
long mine_DUCO_S1_OpenCL(
    const unsigned char *input_prefix,
    int prefix_length,
    const unsigned char target_hexdigest[DUCO_S1_SIZE*2],
    int difficulty)
{
    int maximum = 100*difficulty+1;
    int zeroth_val = 0; // Keeps track of the nonces already tried
    int *nonce_arr = malloc(65536*sizeof(unsigned long));
    int correct_nonce = -1;
    for(int i = 0; i < 65536; i++) nonce_arr[i] = i;

    build_check_nonce_kernel(65536, (const char*)input_prefix, (const char*)target_hexdigest, 1);

    // We stagger GPU and CPU work here to avoid serial execution
    apply_check_nonce_kernel(nonce_arr, &correct_nonce);
    while(zeroth_val < maximum-65536){
        launch_check_nonce_kernel();
        if(correct_nonce != -1){
            await_OpenCL();
            deconstruct_check_nonce_kernel();
            free(nonce_arr);
            return correct_nonce;
        }
        await_OpenCL();
        zeroth_val += 65536;
        dump_check_nonce_kernel(&correct_nonce);
    }

    deconstruct_check_nonce_kernel();
    free(nonce_arr);
    return -1;
}