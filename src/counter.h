#include <string.h>

struct counter_state{
    unsigned long as_long_integer;
    int lower_digits;
    int middle_digits;
    int upper_digits;
    char buf[12];
    int length;
};

void init_counter_state(struct counter_state *state);
void increment_counter(struct counter_state *state);

int count_digits(long num);
void sprint_4_digits(char *buf, int num);
const char *lookup_4_digits(int num);