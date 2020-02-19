#ifndef BITUTILS_H
#define BITUTILS_H

typedef unsigned int uint32;

typedef struct CounterBitSet
{
    uint32 *raw_bits;
    int size; // num of counter
    int bits_per_counter;
    int num_bins;
} CounterBitSet;

void init_counters(CounterBitSet *counters, int size, int bits_per_counter);
void decrement(CounterBitSet *counters, int idx);
void set_to_max(CounterBitSet *counters, int idx);
int test_counter(CounterBitSet *counters, int idx);
void free_counters(CounterBitSet *counters);
void print_counters(CounterBitSet *counters, int start_idx, int end_idx);
int get_counter(CounterBitSet *counters, int idx);

#endif
