#include "counter.h"

void init_counter_state(struct counter_state *state){
    state->as_long_integer = 0;

    state->lower_digits = 0;
    state->middle_digits = 0;
    state->upper_digits = 0;

    memcpy(state->buf, "000000000000", 12);
    state->length = 1;
}

void increment_counter(struct counter_state *state){
    if(state->middle_digits == 9999 && state->lower_digits == 9999){
        state->upper_digits += 1;
        state->middle_digits = 0;
        state->lower_digits = 0;
        sprint_4_digits(state->buf, state->upper_digits);
        sprint_4_digits(state->buf+4, 0);
        sprint_4_digits(state->buf+8, 0);
    }
    else if(state->lower_digits == 9999){
        state->middle_digits += 1;
        state->lower_digits = 0;
        sprint_4_digits(state->buf+4, state->middle_digits);
        sprint_4_digits(state->buf+8, 0);
    }
    else{
        state->lower_digits += 1;
        sprint_4_digits(state->buf+8, state->lower_digits);
    }
    
    state->as_long_integer += 1;
    state->length = count_digits(state->as_long_integer);
}

int count_digits(long num){
    // Balanced search tree
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

void sprint_4_digits(char *buf, int num){
    memcpy(buf, lookup_4_digits(num), 4);
}

const char* lookup_4_digits(int num){
    return table+4*num; // table is defined in table.c
}