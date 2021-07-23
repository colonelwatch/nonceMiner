#include "opencl.h"

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

int count_OpenCL_devices(cl_device_type device_type){
    int ret;
    cl_uint n_platforms;
    ret = clGetPlatformIDs(0, NULL, &n_platforms);
    if(ret != CL_SUCCESS) _error_out("clGetPlatformIDs", ret);

    cl_platform_id *platforms = malloc(n_platforms*sizeof(cl_platform_id));
    ret = clGetPlatformIDs(n_platforms, platforms, NULL);
    if(ret != CL_SUCCESS) _error_out("clGetPlatformIDs", ret);

    int n_devices = 0;
    for(int i = 0; i < n_platforms; i++){
        cl_uint n_devices_in_platform = 0;
        ret = clGetDeviceIDs(platforms[i], device_type, 0, NULL, &n_devices_in_platform);
        if(ret == CL_DEVICE_NOT_FOUND) continue;
        if(ret != CL_SUCCESS) _error_out("clGetDeviceIDs", ret);
        n_devices += n_devices_in_platform;
    }

    free(platforms);
    return n_devices;
}

void get_OpenCL_devices(cl_device_id *devices, int n_devices, cl_device_type device_type){
    int ret;
    cl_uint n_platforms;
    ret = clGetPlatformIDs(0, NULL, &n_platforms);
    if(ret != CL_SUCCESS) _error_out("clGetPlatformIDs", ret);

    cl_platform_id *platforms = malloc(n_platforms*sizeof(cl_platform_id));
    ret = clGetPlatformIDs(n_platforms, platforms, NULL);
    if(ret != CL_SUCCESS) _error_out("clGetPlatformIDs", ret);

    int device_idx = 0;
    for(int i = 0; i < n_platforms; i++){
        cl_uint n_devices_on_platform;
        ret = clGetDeviceIDs(platforms[i], device_type, 0, NULL, &n_devices_on_platform);
        if(ret == CL_DEVICE_NOT_FOUND) continue;
        else if(ret != CL_SUCCESS) _error_out("clGetDeviceIDs", ret);

        cl_device_id *devices_on_platform = malloc(n_devices_on_platform*sizeof(cl_device_id));
        ret = clGetDeviceIDs(platforms[i], device_type, n_devices_on_platform, devices_on_platform, NULL);
        if(ret != CL_SUCCESS) _error_out("clGetDeviceIDs", ret);
        for(int j = 0; j < n_devices_on_platform; j++){
            devices[device_idx] = devices_on_platform[j];
            device_idx++;
        }

        free(devices_on_platform);
    }

    free(platforms);
}

// written with a multiple contexts, multiple device paradigm in mind
void init_OpenCL_devices(check_nonce_ctx *devices_ctx, cl_device_id *devices, int n_devices){
    int ret;
    for(int i = 0; i < n_devices; i++){
        // Create an OpenCL context
        devices_ctx[i].context = clCreateContext(NULL, n_devices, devices, NULL, NULL, &ret);
        if(ret != CL_SUCCESS) _error_out("clCreateContext", ret);

        // Create a command queue for each device
        devices_ctx[i].command_queue = clCreateCommandQueue(devices_ctx[i].context, devices[i], 0, &ret);
        if(ret != CL_SUCCESS) _error_out("clCreateCommandQueue", ret);
    }
}

void build_OpenCL_source(check_nonce_ctx *device_ctx, cl_device_id device_id, char **source_files, int n_files){
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
    device_ctx->program = clCreateProgramWithSource(device_ctx->context, 1, (const char**)&source_str, (const size_t*)&source_len, &ret);
    if(ret != CL_SUCCESS) _error_out("clCreateProgramWithSource", ret);
    ret = clBuildProgram(device_ctx->program, 1, &device_id, NULL, NULL, NULL);

    if (ret == CL_BUILD_PROGRAM_FAILURE) {
        // Determine the size of the log
        size_t log_size;
        clGetProgramBuildInfo(device_ctx->program, device_id, CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);
        // Allocate memory for the log
        char *log = (char *) malloc(log_size);
        // Get the log
        clGetProgramBuildInfo(device_ctx->program, device_id, CL_PROGRAM_BUILD_LOG, log_size, log, NULL);
        // Print the log
        printf("%s\n", log);
    }

    if(ret != CL_SUCCESS) _error_out("clBuildProgram", ret);

    free(source_str);
    free(temp_str);
}

void read_pinned_mem(check_nonce_ctx *device_ctx, void *dst, cl_mem src, size_t size){
    int ret;
    char *temp_ptr = (char*)clEnqueueMapBuffer(device_ctx->command_queue, src, CL_TRUE, CL_MAP_READ, 0, size, 0, NULL, NULL, &ret);
    if(ret != CL_SUCCESS) _error_out("clEnqueueMapBuffer", ret);
    clFlush(device_ctx->command_queue);
    clFinish(device_ctx->command_queue);

    memcpy(dst, temp_ptr, size);

    ret = clEnqueueUnmapMemObject(device_ctx->command_queue, src, temp_ptr, 0, NULL, NULL);
    if(ret != CL_SUCCESS) _error_out("clEnqueueUnmapMemObject", ret);
    clFlush(device_ctx->command_queue);
    clFinish(device_ctx->command_queue);
}

void write_pinned_mem(check_nonce_ctx *device_ctx, cl_mem dst, const void *src, size_t size){
    int ret;
    char *temp_ptr = (char*)clEnqueueMapBuffer(device_ctx->command_queue, dst, CL_TRUE, CL_MAP_WRITE, 0, size, 0, NULL, NULL, &ret);
    if(ret != CL_SUCCESS) _error_out("clEnqueueMapBuffer", ret);
    clFlush(device_ctx->command_queue);
    clFinish(device_ctx->command_queue);

    memcpy(temp_ptr, src, size);

    ret = clEnqueueUnmapMemObject(device_ctx->command_queue, dst, temp_ptr, 0, NULL, NULL);
    if(ret != CL_SUCCESS) _error_out("clEnqueueUnmapMemObject", ret);
    clFlush(device_ctx->command_queue);
    clFinish(device_ctx->command_queue);
}

void await_OpenCL(check_nonce_ctx *ctx){
    clFinish(ctx->command_queue);
}


void build_check_nonce_kernel(check_nonce_ctx *ctx, size_t num_threads){
    // Create the OpenCL kernel
    cl_int ret;
    ctx->kernel = clCreateKernel(ctx->program, "check_nonce", &ret);
    if(ret != CL_SUCCESS) _error_out("clCreateKernel", ret);

    // Set the kernel arguments
    ctx->n_workers = num_threads;

    ctx->nonce_int_mem = clCreateBuffer(ctx->context, CL_MEM_ALLOC_HOST_PTR, ctx->n_workers*sizeof(int), NULL, &ret);
    if(ret != CL_SUCCESS) _error_out("clCreateBuffer", ret);
    ctx->lut_mem = clCreateBuffer(ctx->context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(three_digit_table), (void*)three_digit_table, &ret);
    if(ret != CL_SUCCESS) _error_out("clCreateBuffer", ret);
    ctx->prefix_mem = clCreateBuffer(ctx->context, CL_MEM_READ_ONLY | CL_MEM_ALLOC_HOST_PTR, 40*sizeof(char), NULL, &ret);
    if(ret != CL_SUCCESS) _error_out("clCreateBuffer", ret);
    ctx->target_mem = clCreateBuffer(ctx->context, CL_MEM_READ_ONLY | CL_MEM_ALLOC_HOST_PTR, sizeof(outbuf), NULL, &ret);
    if(ret != CL_SUCCESS) _error_out("clCreateBuffer", ret);
    ctx->correct_nonce_mem = clCreateBuffer(ctx->context, CL_MEM_WRITE_ONLY | CL_MEM_ALLOC_HOST_PTR, sizeof(int), NULL, &ret);
    if(ret != CL_SUCCESS) _error_out("clCreateBuffer", ret);
    clFlush(ctx->command_queue);
    clFinish(ctx->command_queue);
}

// Needs to be called before the first use of the context
void init_check_nonce_kernel(check_nonce_ctx *ctx, const char *prefix, const char *target){
    // Generate some of the parameters to copy
    int temp = -1;
    _generate_expected(&(ctx->expected_hash), target);
    int *nonce_arr = malloc(ctx->n_workers*sizeof(unsigned long));
    for(int i = 0; i < ctx->n_workers; i++) nonce_arr[i] = i;

    // Copy parameters into device memory
    write_pinned_mem(ctx, ctx->nonce_int_mem, nonce_arr, ctx->n_workers*sizeof(int));
    write_pinned_mem(ctx, ctx->prefix_mem, prefix, 40*sizeof(char));
    write_pinned_mem(ctx, ctx->target_mem, &(ctx->expected_hash), sizeof(outbuf));
    write_pinned_mem(ctx, ctx->correct_nonce_mem, &temp, sizeof(int));

    clSetKernelArg(ctx->kernel, 0, sizeof(cl_mem), &(ctx->nonce_int_mem));
    clSetKernelArg(ctx->kernel, 1, sizeof(cl_mem), &(ctx->lut_mem));
    clSetKernelArg(ctx->kernel, 2, sizeof(cl_mem), &(ctx->prefix_mem));
    clSetKernelArg(ctx->kernel, 3, sizeof(cl_mem), &(ctx->target_mem));
    clSetKernelArg(ctx->kernel, 4, sizeof(cl_mem), &(ctx->correct_nonce_mem));
    clSetKernelArg(ctx->kernel, 5, sizeof(int), &(ctx->n_workers));

    free(nonce_arr);
}

void launch_check_nonce_kernel(check_nonce_ctx *ctx){
    cl_int ret;
    ret = clEnqueueNDRangeKernel(ctx->command_queue, ctx->kernel, 1, NULL, &(ctx->n_workers), NULL, 0, NULL, NULL);
    if(ret != CL_SUCCESS) _error_out("clEnqueueNDRangeKernel", ret);
    clFlush(ctx->command_queue);
}

void dump_check_nonce_kernel(check_nonce_ctx *ctx, int *output){
    read_pinned_mem(ctx, output, ctx->correct_nonce_mem, sizeof(int));
}

void deconstruct_check_nonce_kernel(check_nonce_ctx *ctx){
    clReleaseMemObject(ctx->nonce_int_mem);
    clReleaseMemObject(ctx->lut_mem);
    clReleaseMemObject(ctx->prefix_mem);
    clReleaseMemObject(ctx->target_mem);
    clReleaseMemObject(ctx->correct_nonce_mem);
    clReleaseKernel(ctx->kernel);
}