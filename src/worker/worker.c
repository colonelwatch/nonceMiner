#include "worker.h"

unsigned char _hex_to_int(char high_hex, char low_hex){
    int sum = 0;

    if(high_hex >= '0' && high_hex <= '9')
        sum += high_hex - '0';
    else if(high_hex >= 'a' && high_hex <= 'f')
        sum += high_hex - 'a' + 10;
    
    sum *= 16;

    if(low_hex >= '0' && low_hex <= '9')
        sum += low_hex - '0';
    else if(low_hex >= 'a' && low_hex <= 'f')
        sum += low_hex - 'a' + 10;

    return sum;
}

void _generate_expected(cl_uint *expected, const char target_hexdigest[40]){
    for(int j = 0; j < 5; j++){
        unsigned int num = 0;
        for(int k = 3; k >= 0; k--){
            char high_hex = target_hexdigest[j*8+k*2],
                 low_hex = target_hexdigest[j*8+k*2+1];
            num |= _hex_to_int(high_hex, low_hex) << k*8;
        }
        expected[j] = num;
    }
}

void _error_out(char* func_name, cl_int ret){
    fprintf(stderr, "%s failed with error code %d\n", func_name, ret);
    abort();
}

void _replace_string(char *buffer, const char *search, const char *replace){
    char *temp = malloc(MAX_SOURCE_SIZE);
    char *ptr = strstr(buffer, search);
    if(ptr){
        strcpy(temp, ptr+strlen(search));
        strcpy(ptr, replace);
        strcpy(ptr+strlen(replace), temp);
    }
    free(temp);
}

void _read_pinned_mem(worker_ctx *ctx, void *dst, cl_mem src, size_t size){
    cl_int ret;
    char *temp_ptr = (char*)clEnqueueMapBuffer(ctx->command_queue, src, CL_TRUE, CL_MAP_READ, 0, size, 0, NULL, NULL, &ret);
    if(ret != CL_SUCCESS) _error_out("clEnqueueMapBuffer", ret);
    clFlush(ctx->command_queue);
    clFinish(ctx->command_queue);

    memcpy(dst, temp_ptr, size);

    ret = clEnqueueUnmapMemObject(ctx->command_queue, src, temp_ptr, 0, NULL, NULL);
    if(ret != CL_SUCCESS) _error_out("clEnqueueUnmapMemObject", ret);
    clFlush(ctx->command_queue);
    clFinish(ctx->command_queue);
}

void _write_pinned_mem(worker_ctx *ctx, cl_mem dst, const void *src, size_t size){
    cl_int ret;
    char *temp_ptr = (char*)clEnqueueMapBuffer(ctx->command_queue, dst, CL_TRUE, CL_MAP_WRITE, 0, size, 0, NULL, NULL, &ret);
    if(ret != CL_SUCCESS) _error_out("clEnqueueMapBuffer", ret);
    clFlush(ctx->command_queue);
    clFinish(ctx->command_queue);

    memcpy(temp_ptr, src, size);

    ret = clEnqueueUnmapMemObject(ctx->command_queue, dst, temp_ptr, 0, NULL, NULL);
    if(ret != CL_SUCCESS) _error_out("clEnqueueUnmapMemObject", ret);
    clFlush(ctx->command_queue);
    clFinish(ctx->command_queue);
}


int count_OpenCL_devices(cl_device_type device_type){
    // Find all platforms on host, generally one per manafacturer (AMD, Intel, NVIDIA, etc.)
    cl_int ret;
    cl_uint n_platforms;
    ret = clGetPlatformIDs(0, NULL, &n_platforms);
    if(ret != CL_SUCCESS) _error_out("clGetPlatformIDs", ret);
    cl_platform_id *platforms = malloc(n_platforms*sizeof(cl_platform_id));
    ret = clGetPlatformIDs(n_platforms, platforms, NULL);
    if(ret != CL_SUCCESS) _error_out("clGetPlatformIDs", ret);

    // Find all devices on each platform
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
    // We have to redo part of the above function
    cl_int ret;
    cl_uint n_platforms;
    ret = clGetPlatformIDs(0, NULL, &n_platforms);
    if(ret != CL_SUCCESS) _error_out("clGetPlatformIDs", ret);
    cl_platform_id *platforms = malloc(n_platforms*sizeof(cl_platform_id));
    ret = clGetPlatformIDs(n_platforms, platforms, NULL);
    if(ret != CL_SUCCESS) _error_out("clGetPlatformIDs", ret);

    // Load the devices of each platform onto a flat list
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
void init_OpenCL_workers(worker_ctx *ctx_arr, cl_device_id *devices, int n_devices){
    cl_int ret;
    for(int i = 0; i < n_devices; i++){
        // Create an OpenCL context (platform is assumed from the device_id)
        ctx_arr[i].context = clCreateContext(NULL, 1, &devices[i], NULL, NULL, &ret);
        if(ret != CL_SUCCESS) _error_out("clCreateContext", ret);

        // Create a command queue for each device
        ctx_arr[i].command_queue = clCreateCommandQueue(ctx_arr[i].context, devices[i], 0, &ret);
        if(ret != CL_SUCCESS) _error_out("clCreateCommandQueue", ret);
    }
}

void build_OpenCL_worker_source(worker_ctx *ctx, cl_device_id device_id, char **source_files, int n_files){
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

        temp_str[temp_len] = '\n'; // Add a newline in case it is missing
        temp_str[temp_len+1] = '\0';
        source_len += temp_len+1;
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
    ctx->program = clCreateProgramWithSource(ctx->context, 1, (const char**)&source_str, (const size_t*)&source_len, &ret);
    if(ret != CL_SUCCESS) _error_out("clCreateProgramWithSource", ret);
    ret = clBuildProgram(ctx->program, 1, &device_id, NULL, NULL, NULL);

    // If there is a compile error, print it
    if(ret == CL_BUILD_PROGRAM_FAILURE){
        size_t log_size;
        clGetProgramBuildInfo(ctx->program, device_id, CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);
        char *log = (char*)malloc(log_size*sizeof(char));
        clGetProgramBuildInfo(ctx->program, device_id, CL_PROGRAM_BUILD_LOG, log_size, log, NULL);
        puts(log);
        free(log);
    }
    if(ret != CL_SUCCESS) _error_out("clBuildProgram", ret);

    free(source_str);
    free(temp_str);
}

void await_OpenCL_worker(worker_ctx *ctx){
    clFinish(ctx->command_queue);
}


void build_OpenCL_worker_kernel(worker_ctx *ctx, size_t auto_iterate_size){
    // Create the OpenCL kernel
    cl_int ret;
    ctx->kernel = clCreateKernel(ctx->program, "check_nonce", &ret);
    if(ret != CL_SUCCESS) _error_out("clCreateKernel", ret);

    ctx->auto_iterate_size = auto_iterate_size;

    // Build the kernel buffers
    ctx->nonce_int_mem = clCreateBuffer(ctx->context, CL_MEM_ALLOC_HOST_PTR, sizeof(cl_long), NULL, &ret);
    if(ret != CL_SUCCESS) _error_out("clCreateBuffer", ret);
    ctx->prefix_mem = clCreateBuffer(ctx->context, CL_MEM_READ_ONLY | CL_MEM_ALLOC_HOST_PTR, 40*sizeof(char), NULL, &ret);
    if(ret != CL_SUCCESS) _error_out("clCreateBuffer", ret);
    ctx->target_mem = clCreateBuffer(ctx->context, CL_MEM_READ_ONLY | CL_MEM_ALLOC_HOST_PTR, 5*sizeof(cl_uint), NULL, &ret);
    if(ret != CL_SUCCESS) _error_out("clCreateBuffer", ret);
    ctx->correct_nonce_mem = clCreateBuffer(ctx->context, CL_MEM_WRITE_ONLY | CL_MEM_ALLOC_HOST_PTR, sizeof(cl_long), NULL, &ret);
    if(ret != CL_SUCCESS) _error_out("clCreateBuffer", ret);
    clFlush(ctx->command_queue);
    clFinish(ctx->command_queue);
}

// Needs to be called before the first use of the context
void init_OpenCL_worker_kernel(worker_ctx *ctx, const char *prefix, const char *target){
    // Generate some of the parameters to copy
    cl_long temp_neg1 = -1;
    ctx->current_nonce = 0;
    _generate_expected(ctx->expected_hash, target);

    // Copy parameters into device memory
    _write_pinned_mem(ctx, ctx->nonce_int_mem, &(ctx->current_nonce), sizeof(cl_long));
    _write_pinned_mem(ctx, ctx->prefix_mem, prefix, 40*sizeof(char));
    _write_pinned_mem(ctx, ctx->target_mem, ctx->expected_hash, 5*sizeof(cl_uint));
    _write_pinned_mem(ctx, ctx->correct_nonce_mem, &temp_neg1, sizeof(cl_long));

    // Set the kernel arguments
    clSetKernelArg(ctx->kernel, 0, sizeof(cl_mem), &(ctx->nonce_int_mem));
    clSetKernelArg(ctx->kernel, 1, sizeof(cl_mem), &(ctx->prefix_mem));
    clSetKernelArg(ctx->kernel, 2, sizeof(cl_mem), &(ctx->target_mem));
    clSetKernelArg(ctx->kernel, 3, sizeof(cl_mem), &(ctx->correct_nonce_mem));
}

void launch_OpenCL_worker_kernel(worker_ctx *ctx){
    cl_int ret;
    ret = clEnqueueNDRangeKernel(ctx->command_queue, ctx->kernel, 1, NULL, &(ctx->auto_iterate_size), NULL, 0, NULL, NULL);
    if(ret != CL_SUCCESS) _error_out("clEnqueueNDRangeKernel", ret);
    clFlush(ctx->command_queue);
}

void dump_OpenCL_worker_kernel(worker_ctx *ctx, int64_t *output){
    _read_pinned_mem(ctx, output, ctx->correct_nonce_mem, sizeof(cl_long)); // cl_long and int64_t should be equiv
}

void increment_OpenCL_worker_kernel(worker_ctx *ctx){
    ctx->current_nonce += ctx->auto_iterate_size;
    _write_pinned_mem(ctx, ctx->nonce_int_mem, &(ctx->current_nonce), sizeof(cl_long));
    clSetKernelArg(ctx->kernel, 0, sizeof(cl_mem), &(ctx->nonce_int_mem));
}

void deconstruct_OpenCL_worker_kernel(worker_ctx *ctx){
    clReleaseMemObject(ctx->nonce_int_mem);
    clReleaseMemObject(ctx->prefix_mem);
    clReleaseMemObject(ctx->target_mem);
    clReleaseMemObject(ctx->correct_nonce_mem);
    clReleaseKernel(ctx->kernel);
}