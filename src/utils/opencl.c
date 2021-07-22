#include "opencl.h"

// global OpenCL context
cl_device_id device_id;
cl_context context;
cl_command_queue command_queue;
cl_program program;

// SHA-1 kernel context
cl_kernel sha1_kernel;
cl_mem sha1_in_mem, sha1_out_mem;
inbuf *sha1_in;
outbuf *sha1_out;
size_t sha1_n_workers;

// DUCO_S1 kernel context
cl_kernel check_nonce_kernel;
cl_mem nonce_int_mem, lut_mem, prefix_mem, target_mem, correct_nonce_mem;
outbuf check_nonce_expected_hash;
int *nonce_int;
int *correct_nonce;
size_t check_nonce_n_workers;
int iterate_size;


unsigned char _hex_to_int(char high_hex, char low_hex){
    int sum = 0;
    switch(low_hex){
        case '0': sum += 0; break;
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
    switch(high_hex){
        case '0': sum += 0*16; break;
        case '1': sum += 1*16; break;
        case '2': sum += 2*16; break;
        case '3': sum += 3*16; break;
        case '4': sum += 4*16; break;
        case '5': sum += 5*16; break;
        case '6': sum += 6*16; break;
        case '7': sum += 7*16; break;
        case '8': sum += 8*16; break;
        case '9': sum += 9*16; break;
        case 'a': sum += 10*16; break;
        case 'b': sum += 11*16; break;
        case 'c': sum += 12*16; break;
        case 'd': sum += 13*16; break;
        case 'e': sum += 14*16; break;
        case 'f': sum += 15*16; break;
    }
    return sum;
}

void _generate_expected(outbuf *expected, const char target_hexdigest[40]){
    for(int j = 0; j < 5; j++){
        unsigned int num = 0;
        for(int k = 3; k >= 0; k--){
            char high_hex = target_hexdigest[j*8+k*2],
                 low_hex = target_hexdigest[j*8+k*2+1];
            num |= _hex_to_int(high_hex, low_hex) << k*8;
        }
        expected->buffer[j] = num;
    }
}

void _error_out(char* func_name, cl_int ret){
    fprintf(stderr, "%s failed with error code %d\n", func_name, ret);
    abort();
}

void _replace_string(char *buffer, const char *search, const char *replace){
    char *ptr = strstr(buffer, search);
    if(ptr){
        char *temp = malloc(MAX_SOURCE_SIZE);
        strcpy(temp, ptr+strlen(search));
        strcpy(ptr, replace);
        strcpy(ptr+strlen(replace), temp);
        free(temp);
    }
}


void init_OpenCL(){
    // Get platform and device information
    cl_platform_id platform_id;
    cl_int ret;
    ret = clGetPlatformIDs(1, &platform_id, NULL);
    if(ret != CL_SUCCESS) _error_out("clGetPlatformIDs", ret);
    ret = clGetDeviceIDs(platform_id, CL_DEVICE_TYPE_GPU, 1, &device_id, NULL);
    if(ret != CL_SUCCESS) _error_out("clGetDeviceIDs", ret);

    // Create an OpenCL context
    context = clCreateContext(NULL, 1, &device_id, NULL, NULL, &ret);
    if(ret != CL_SUCCESS) _error_out("clCreateContext", ret);

    // Create a command queue
    command_queue = clCreateCommandQueue(context, device_id, 0, &ret);
    if(ret != CL_SUCCESS) _error_out("clCreateCommandQueue", ret);
}

void build_OpenCL_source(char **source_files, int n_files){
    // Load the buffer_structs_template.cl into source_str
    FILE *fp;
    char *source_str = (char*)malloc(MAX_SOURCE_SIZE),
         *temp_str = (char*)malloc(MAX_SOURCE_SIZE);
    size_t source_len = 0, temp_len;

    source_str[0] = '\0'; // Set the source_str to be empty
    for(int i = 0; i < n_files; i++){
        fp = fopen(source_files[i], "r");
        if(fp == NULL){
            fprintf(stderr, "Could not open file %s\n", source_files[i]);
            abort();
        }
        temp_len = fread(temp_str, 1, MAX_SOURCE_SIZE, fp);
        fclose(fp);

        temp_str[temp_len] = '\0';
        source_len += temp_len;
        strcat(source_str, temp_str);
    }

    // Configure the buffers by replacing placeholders
    _replace_string(source_str, "<word_size>", "4");
    _replace_string(source_str, "<inBufferSize_bytes>", "50");
    _replace_string(source_str, "<outBufferSize_bytes>", "20");
    _replace_string(source_str, "<saltBufferSize_bytes>", "0");
    _replace_string(source_str, "<ctBufferSize_bytes>", "0");
    _replace_string(source_str, "<hashBlockSize_bits>", "512");
    _replace_string(source_str, "<hashDigestSize_bits>", "160");
    _replace_string(source_str, "mod(j,wordSize)", "j\%wordSize");
    _replace_string(source_str, "\r\n", "\n"); // convert out of Windows format

    source_len = strlen(source_str); // deal with how _replace_string doesn't update length

    // Create an OpenCL program from the source string
    cl_int ret;
    program = clCreateProgramWithSource(context, 1, (const char**)&source_str, (const size_t*)&source_len, &ret);
    if(ret != CL_SUCCESS) _error_out("clCreateProgramWithSource", ret);
    ret = clBuildProgram(program, 1, &device_id, NULL, NULL, NULL);

    if (ret == CL_BUILD_PROGRAM_FAILURE) {
        // Determine the size of the log
        size_t log_size;
        clGetProgramBuildInfo(program, device_id, CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);
        // Allocate memory for the log
        char *log = (char *) malloc(log_size);
        // Get the log
        clGetProgramBuildInfo(program, device_id, CL_PROGRAM_BUILD_LOG, log_size, log, NULL);
        // Print the log
        printf("%s\n", log);
    }

    if(ret != CL_SUCCESS) _error_out("clBuildProgram", ret);

    free(source_str);
    free(temp_str);
}

void await_OpenCL(){
    clFinish(command_queue);
}


void build_check_nonce_kernel(size_t num_threads, const char *prefix,  const char *target, int auto_iterate){
    // Create the OpenCL kernel
    cl_int ret;
    check_nonce_kernel = clCreateKernel(program, "check_nonce", &ret);
    if(ret != CL_SUCCESS) _error_out("clCreateKernel", ret);

    // Set the kernel arguments
    check_nonce_n_workers = num_threads;
    if(auto_iterate) iterate_size = num_threads;
    else iterate_size = 0;
    _generate_expected(&check_nonce_expected_hash, target);

    nonce_int_mem = clCreateBuffer(context, CL_MEM_ALLOC_HOST_PTR, check_nonce_n_workers*sizeof(int), NULL, &ret);
    if(ret != CL_SUCCESS) _error_out("clCreateBuffer", ret);
    lut_mem = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(three_digit_table), (void*)three_digit_table, &ret);
    if(ret != CL_SUCCESS) _error_out("clCreateBuffer", ret);
    prefix_mem = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, 40*sizeof(char), (void*)prefix, &ret);
    if(ret != CL_SUCCESS) _error_out("clCreateBuffer", ret);
    target_mem = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(outbuf), (void*)&check_nonce_expected_hash, &ret);
    if(ret != CL_SUCCESS) _error_out("clCreateBuffer", ret);
    correct_nonce_mem = clCreateBuffer(context, CL_MEM_WRITE_ONLY | CL_MEM_ALLOC_HOST_PTR, sizeof(int), NULL, &ret);
    if(ret != CL_SUCCESS) _error_out("clCreateBuffer", ret);
    clFlush(command_queue);
    clFinish(command_queue);

    // Initialize correct_nonce_mem with -1
    correct_nonce = (int*)clEnqueueMapBuffer(command_queue, correct_nonce_mem, CL_TRUE, CL_MAP_WRITE, 0, sizeof(int), 0, NULL, NULL, &ret);
    if(ret != CL_SUCCESS) _error_out("clEnqueueMapBuffer", ret);
    clFlush(command_queue);
    clFinish(command_queue);
    *correct_nonce = -1;
    ret = clEnqueueUnmapMemObject(command_queue, correct_nonce_mem, correct_nonce, 0, NULL, NULL);
    if(ret != CL_SUCCESS) _error_out("clEnqueueUnmapMemObject", ret);
    clFlush(command_queue);
    clFinish(command_queue);

    clSetKernelArg(check_nonce_kernel, 0, sizeof(cl_mem), &nonce_int_mem);
    clSetKernelArg(check_nonce_kernel, 1, sizeof(cl_mem), &lut_mem);
    clSetKernelArg(check_nonce_kernel, 2, sizeof(cl_mem), &prefix_mem);
    clSetKernelArg(check_nonce_kernel, 3, sizeof(cl_mem), &target_mem);
    clSetKernelArg(check_nonce_kernel, 4, sizeof(cl_mem), &correct_nonce_mem);
    clSetKernelArg(check_nonce_kernel, 5, sizeof(int), &iterate_size);
}

void feed_check_nonce_kernel(int *input){
    int ret;
    nonce_int = (int*)clEnqueueMapBuffer(command_queue, nonce_int_mem, CL_TRUE, CL_MAP_WRITE, 0, check_nonce_n_workers*sizeof(int), 0, NULL, NULL, &ret);
    if(ret != CL_SUCCESS) _error_out("clEnqueueMapBuffer", ret);
    clFlush(command_queue);
    clFinish(command_queue);

    memcpy(nonce_int, input, check_nonce_n_workers*sizeof(int));

    ret = clEnqueueUnmapMemObject(command_queue, nonce_int_mem, nonce_int, 0, NULL, NULL);
    if(ret != CL_SUCCESS) _error_out("clEnqueueUnmapMemObject", ret);
    clFlush(command_queue);
    clFinish(command_queue);
}

void launch_check_nonce_kernel(){
    cl_int ret;
    ret = clEnqueueNDRangeKernel(command_queue, check_nonce_kernel, 1, NULL, &check_nonce_n_workers, NULL, 0, NULL, NULL);
    if(ret != CL_SUCCESS) _error_out("clEnqueueNDRangeKernel", ret);
    clFlush(command_queue);
}

void dump_check_nonce_kernel(int *output){
    int ret;
    correct_nonce = (int*)clEnqueueMapBuffer(command_queue, correct_nonce_mem, CL_TRUE, CL_MAP_READ, 0, sizeof(int), 0, NULL, NULL, &ret);
    if(ret != CL_SUCCESS) _error_out("clEnqueueMapBuffer", ret);
    clFlush(command_queue);
    clFinish(command_queue);

    memcpy(output, correct_nonce, sizeof(int));

    ret = clEnqueueUnmapMemObject(command_queue, correct_nonce_mem, correct_nonce, 0, NULL, NULL);
    if(ret != CL_SUCCESS) _error_out("clEnqueueUnmapMemObject", ret);
    clFlush(command_queue);
    clFinish(command_queue);
}

void apply_check_nonce_kernel(int *input, int *output){
    feed_check_nonce_kernel(input);
    launch_check_nonce_kernel();
    await_OpenCL();
    dump_check_nonce_kernel(output);
}

void deconstruct_check_nonce_kernel(){
    clReleaseMemObject(nonce_int_mem);
    clReleaseMemObject(lut_mem);
    clReleaseMemObject(prefix_mem);
    clReleaseMemObject(target_mem);
    clReleaseMemObject(correct_nonce_mem);
    clReleaseKernel(check_nonce_kernel);
}


void build_sha1_kernel(size_t num_threads){
    // Create the OpenCL kernel
    cl_int ret;
    sha1_kernel = clCreateKernel(program, "hash_main", &ret);
    if(ret != CL_SUCCESS) _error_out("clCreateKernel", ret);

    sha1_in_mem = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_ALLOC_HOST_PTR, num_threads*sizeof(inbuf), NULL, &ret);
    if(ret != CL_SUCCESS) _error_out("clCreateBuffer", ret);
    sha1_out_mem = clCreateBuffer(context, CL_MEM_WRITE_ONLY | CL_MEM_ALLOC_HOST_PTR, num_threads*sizeof(outbuf), NULL, &ret);
    if(ret != CL_SUCCESS) _error_out("clCreateBuffer", ret);
    clFlush(command_queue);
    clFinish(command_queue);

    ret = clSetKernelArg(sha1_kernel, 0, sizeof(cl_mem), &sha1_in_mem);
    ret = clSetKernelArg(sha1_kernel, 1, sizeof(cl_mem), &sha1_out_mem);

    sha1_n_workers = num_threads;
}

void feed_sha1_kernel(inbuf *input){
    int ret;
    sha1_in = (inbuf*)clEnqueueMapBuffer(command_queue, sha1_in_mem, CL_TRUE, CL_MAP_WRITE, 0, sha1_n_workers*sizeof(inbuf), 0, NULL, NULL, &ret);
    if(ret != CL_SUCCESS) _error_out("clEnqueueMapBuffer", ret);
    clFlush(command_queue);
    clFinish(command_queue);

    memcpy(sha1_in, input, sha1_n_workers*sizeof(inbuf));

    ret = clEnqueueUnmapMemObject(command_queue, sha1_in_mem, sha1_in, 0, NULL, NULL);
    if(ret != CL_SUCCESS) _error_out("clEnqueueUnmapMemObject", ret);
    clFlush(command_queue);
    clFinish(command_queue);
}

void launch_sha1_kernel(){
    cl_int ret;
    ret = clEnqueueNDRangeKernel(command_queue, sha1_kernel, 1, NULL, &sha1_n_workers, NULL, 0, NULL, NULL);
    if(ret != CL_SUCCESS) _error_out("clEnqueueNDRangeKernel", ret);
    clFlush(command_queue);
}

void dump_sha1_kernel(outbuf *output){
    int ret;
    sha1_out = (outbuf*)clEnqueueMapBuffer(command_queue, sha1_out_mem, CL_TRUE, CL_MAP_READ, 0, sha1_n_workers*sizeof(outbuf), 0, NULL, NULL, &ret);
    if(ret != CL_SUCCESS) _error_out("clEnqueueMapBuffer", ret);
    clFlush(command_queue);
    clFinish(command_queue);

    memcpy(output, sha1_out, sha1_n_workers*sizeof(outbuf));

    ret = clEnqueueUnmapMemObject(command_queue, sha1_out_mem, sha1_out, 0, NULL, NULL);
    if(ret != CL_SUCCESS) _error_out("clEnqueueUnmapMemObject", ret);
    clFlush(command_queue);
    clFinish(command_queue);
}

void apply_sha1_kernel(inbuf *input, outbuf *output){
    feed_sha1_kernel(input);
    launch_sha1_kernel();
    await_OpenCL();
    dump_sha1_kernel(output);
}