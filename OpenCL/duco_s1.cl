int count_digits(int num){
    if(num < 100000)
        if(num < 1000)
            if(num < 100)
                if(num < 10) return 1;
                else return 2;
            else return 3;
        else
            if(num < 10000) return 4;
            else return 5;
    else
        if(num < 10000000)
            if(num < 1000000) return 6;
            else return 7;
        else
            if(num < 100000000) return 8;
            else
                if(num < 1000000000) return 9;
                else return 10;
}

void lookup_3_digits(char *arr, int num, __constant const char *table){
    for(int i = 0; i < 3; i++) arr[i] = table[i+3*num];
}

int fast_print_int(char *buf, int num, __constant const char *lut){
    char temp_buf[12];
    char *temp_buf_ptr = temp_buf+12;
    int n_digits = 0;
    
    if(num == 0){ // Edge case, never enters for loop if num == 0
        temp_buf_ptr -= 3;
        lookup_3_digits(temp_buf_ptr, 0, lut);
        n_digits = 1;
    }
    else{
        for(int working_num = num; working_num != 0; working_num /= 1000){
            temp_buf_ptr -= 3;
            lookup_3_digits(temp_buf_ptr, working_num % 1000, lut);
        }
        n_digits = count_digits(num);
    }

    for(int i = 0; i < n_digits; i++) buf[i] = temp_buf[i+12-n_digits];
    buf[n_digits] = '\0';
    return n_digits;
}

int compare(outbuf *hash, __constant const outbuf *target)
{
    for(int i = 0; i < 5; i++)
        if(hash->buffer[i] != target->buffer[i]) return 0;
    return 1;
}

__kernel void check_nonce(
    __global int *nonce_int, 
    __constant const char *lut, 
    __constant const char *prefix, 
    __constant const outbuf *target, 
    __global int *correct_nonce, 
    int auto_iterate_size
)
{
    unsigned int idx = get_global_id(0);
    inbuf sha1_input;
    outbuf sha1_output;
    char *buffer_ptr = (char*)sha1_input.buffer;

    for(int i = 0; i < 40; i++) buffer_ptr[i] = prefix[i];
    for(int i = 40; i < 52; i++) buffer_ptr[i] = 0; // Pads buffer with zeroes (sha1 kernel expects this)
    sha1_input.length = 40+fast_print_int(buffer_ptr+40, nonce_int[idx], lut); // But before printing (this way is faster)

    hash_private(sha1_input.buffer, sha1_input.length, sha1_output.buffer);

    if(compare(&sha1_output, target)) *correct_nonce = nonce_int[idx];

    nonce_int[idx] += auto_iterate_size;
}