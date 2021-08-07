#ifndef OPENCL_H
#define OPENCL_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef __APPLE__
    #include <OpenCL/opencl.h>
#else
    #include <CL/cl.h>
#endif

#define MAX_SOURCE_SIZE 0x100000

// OpenCL worker context
typedef struct{
    cl_context context;
    cl_command_queue command_queue;
    cl_program program;
    cl_kernel kernel;
    cl_mem nonce_int_mem, prefix_mem, target_mem, correct_nonce_mem;
    size_t prefix_size;
    cl_uint expected_hash[5];
    size_t auto_iterate_size;
    cl_long current_nonce;
} worker_ctx;

// internal private functions
unsigned char _hex_to_int(char high_hex, char low_hex);
void _generate_expected(cl_uint *expected, const char target_hexdigest[40]);
void _error_out(char* func_name, cl_int ret);
void _replace_string(char *buffer, const char *search, const char *replace);
void _read_pinned_mem(worker_ctx *ctx, void *dst, cl_mem src, size_t size);
void _write_pinned_mem(worker_ctx *ctx, cl_mem dst, const void *src, size_t size);

// OpenCL worker functions
int count_OpenCL_devices(cl_device_type device_type);
void get_OpenCL_devices(cl_device_id *devices, int n_devices, cl_device_type device_type);
void init_OpenCL_workers(worker_ctx *ctx_arr, cl_device_id *devices, int n_devices);
void build_OpenCL_worker_source(worker_ctx *ctx, cl_device_id device_id, char **source_files, int n_files);
void await_OpenCL_worker(worker_ctx *ctx);

// OpenCL kernel functions
void build_OpenCL_worker_kernel(worker_ctx *ctx, size_t auto_iterate_size, int alternate_mode);
void init_OpenCL_worker_kernel(worker_ctx *ctx, const char *prefix, const char *target);
void launch_OpenCL_worker_kernel(worker_ctx *ctx);
void dump_OpenCL_worker_kernel(worker_ctx *ctx, int64_t *output);
void increment_OpenCL_worker_kernel(worker_ctx *ctx);
void deconstruct_OpenCL_worker_kernel(worker_ctx *ctx);

#endif