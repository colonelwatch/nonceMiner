#ifndef OPENCL_H
#define OPENCL_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <CL/cl.h>

#include "table.h"

#define MAX_SOURCE_SIZE 0x100000

// Input/output formats specified by sha1.cl
typedef struct{
    cl_uint length; // in bytes
    cl_uint buffer[13];
} inbuf;
typedef struct{
    cl_uint buffer[5];
} outbuf;

// OpenCL worker context
typedef struct{
    cl_context context;
    cl_command_queue command_queue;
    cl_program program;
    cl_kernel kernel;
    cl_mem nonce_int_mem, lut_mem, prefix_mem, target_mem, correct_nonce_mem;
    outbuf expected_hash;
    size_t auto_iterate_size;
} worker_ctx;

// internal private functions
unsigned char _hex_to_int(char high_hex, char low_hex);
void _generate_expected(outbuf *expected, const char target_hexdigest[40]);
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
void build_OpenCL_worker_kernel(worker_ctx *ctx, size_t auto_iterate_size);
void init_OpenCL_worker_kernel(worker_ctx *ctx, const char *prefix, const char *target);
void launch_OpenCL_worker_kernel(worker_ctx *ctx);
void dump_OpenCL_worker_kernel(worker_ctx *ctx, int *output);
void deconstruct_OpenCL_worker_kernel(worker_ctx *ctx);

#endif