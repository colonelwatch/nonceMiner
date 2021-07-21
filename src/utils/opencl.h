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

void init_OpenCL();
void build_source(char **source_files, int n_files);

// SHA-1 kernel functions
void build_sha1_kernel(size_t num_threads);
void feed_sha1_kernel(inbuf *input);
void launch_sha1_kernel();
void dump_sha1_kernel(outbuf *output);
void apply_sha1_kernel(inbuf *input, outbuf *output);

// check_nonce functions
void build_check_nonce_kernel(size_t num_threads, const char *prefix, const char *target, int auto_iterate);
void feed_check_nonce_kernel(int *input);
void launch_check_nonce_kernel();
void dump_check_nonce_kernel(int *output);
void apply_check_nonce_kernel(int *input, int *output);
void deconstruct_check_nonce_kernel();

void await_OpenCL();

#endif