#ifndef OPENCL_H
#define OPENCL_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <CL/cl.h>

#include "table.h"

#define MAX_SOURCE_SIZE 0x100000

typedef struct {
    cl_uint length; // in bytes
    cl_uint buffer[13];
} inbuf;

typedef struct {
    cl_uint buffer[5];
} outbuf;

extern inbuf *in;

// internal private functions
unsigned char _hex_to_int(char high_hex, char low_hex);
void _generate_expected(outbuf *expected, const char target_hexdigest[40]);
void _error_out(char* func_name, cl_int ret);
void _replace_string(char *buffer, const char *search, const char *replace);

// general OpenCL functions
void init_OpenCL();
void build_OpenCL_source(char **source_files, int n_files);
void await_OpenCL();

// check_nonce functions
void build_check_nonce_kernel(size_t num_threads, const char *prefix, const char *target, int auto_iterate);
void feed_check_nonce_kernel(int *input);
void launch_check_nonce_kernel();
void dump_check_nonce_kernel(int *output);
void apply_check_nonce_kernel(int *input, int *output);
void deconstruct_check_nonce_kernel();

#endif