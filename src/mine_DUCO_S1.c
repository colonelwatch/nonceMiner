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

#ifndef NO_OPENCL
int64_t mine_DUCO_S1_OpenCL(
    const unsigned char *input_prefix,
    int prefix_length,
    const unsigned char target_hexdigest[DUCO_S1_SIZE*2],
    int difficulty,
    worker_ctx *ctx)
{
    int64_t maximum = 100*(int64_t)difficulty+1;
    int64_t correct_nonce;
    init_OpenCL_worker_kernel(ctx, (const char*)input_prefix, (const char*)target_hexdigest);

    // We stagger GPU and CPU work here to avoid serial execution, and the GPU starts first
    launch_OpenCL_worker_kernel(ctx);
    await_OpenCL_worker(ctx);
    dump_OpenCL_worker_kernel(ctx, &correct_nonce);
    increment_OpenCL_worker_kernel(ctx);
    while(ctx->current_nonce < maximum){
        launch_OpenCL_worker_kernel(ctx);
        if(correct_nonce != -1){
            await_OpenCL_worker(ctx);
            return correct_nonce;
        }
        await_OpenCL_worker(ctx);
        dump_OpenCL_worker_kernel(ctx, &correct_nonce);
        increment_OpenCL_worker_kernel(ctx);
    }

    await_OpenCL_worker(ctx);
    return -1;
}
#endif