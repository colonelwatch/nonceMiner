// three_digit_table, minimum_digits, and powers_of_ten are defined in lookup_tables.cl

int count_digits(long num){
    if(num == 0) return 1;
    int base2 = 8*sizeof(long)-clz(num)-1;
    int digits = minimum_digits[base2]; // Equal to presumed log10(num) + 1
    if(num >= powers_of_ten[digits]) digits++; // If presumed log10(num) is too small, add one
    return digits;
}

void lookup_3_digits(char *arr, int num){
    for(int i = 0; i < 3; i++) arr[i] = three_digit_table[i+3*num];
}

int fast_print_int(char *buf, long num){
    char temp_buf[20];
    char *temp_buf_ptr = temp_buf+20;
    int n_digits = count_digits(num);
    temp_buf[19] = '0'; // Necessary for the zero corner case
    
    for(long working_num = num; working_num != 0; working_num /= 1000){
        temp_buf_ptr -= 3;
        lookup_3_digits(temp_buf_ptr, working_num % 1000);
    }

    for(int i = 0; i < n_digits; i++) buf[i] = temp_buf[i+20-n_digits];
    return n_digits;
}

int compare(unsigned int *hash, __constant const unsigned int *target)
{
    for(int i = 0; i < 5; i++)
        if(hash[i] != target[i]) return 0;
    return 1;
}

__kernel void check_nonce(
    __global long *next_nonce, 
    __constant const char *prefix, 
    __constant const unsigned int *target, 
    __global long *correct_nonce
)
{
    __private unsigned int inbuf[15], outbuf[5];
    unsigned int idx = get_global_id(0);
    long current_nonce = (*next_nonce)+idx;
    char *buffer_ptr = (char*)inbuf;

    for(int i = 0; i < 40; i++) buffer_ptr[i] = prefix[i];
    for(int i = 40; i < 60; i++) buffer_ptr[i] = 0; // Pads buffer with zeroes (sha1 kernel expects this)
    int length = 40+fast_print_int(buffer_ptr+40, current_nonce); // But before printing (this way is faster)

    hash_private(inbuf, length, outbuf);
    
    // Conditional execution (halts other threads)
    if(compare(outbuf, target)) *correct_nonce = current_nonce;
}

__kernel void check_nonce_alt(
    __global long *next_nonce, 
    __constant const char *prefix, 
    __constant const unsigned int *target, 
    __global long *correct_nonce
)
{
    __private unsigned int inbuf[9], outbuf[5];
    unsigned int idx = get_global_id(0);
    long current_nonce = (*next_nonce)+idx;
    char *buffer_ptr = (char*)inbuf;

    for(int i = 0; i < 16; i++) buffer_ptr[i] = prefix[i];
    for(int i = 16; i < 36; i++) buffer_ptr[i] = 0; // Pads buffer with zeroes (sha1 kernel expects this)
    int length = 16+fast_print_int(buffer_ptr+16, current_nonce); // But before printing (this way is faster)

    hash_private(inbuf, length, outbuf);
    
    // Conditional execution (halts other threads)
    if(compare(outbuf, target)) *correct_nonce = current_nonce;
}