#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "./bitutils.h"

#define BIN_BITS 32
#define MAX_BITS_PER_COUNTER 32

#define GEN_BITS_RANGE(l ,r) (((1UL << ((l) - 1)) - 1) ^ ((1UL << (r)) - 1))

/**
 * Init a CounterBitSet.
 * 
 * counters: pointer to a CounterBitSet
 * size: number of counters
 * bits_per_counter: value from 1 -- 32
*/
void init_counters(CounterBitSet *counters, int size, int bits_per_counter)
{
    if (bits_per_counter > MAX_BITS_PER_COUNTER)
    {
        printf("The maximum bits per counter is 32, %d provided.\n", bits_per_counter);
        exit(1);
    }

    int bins = (size * bits_per_counter + BIN_BITS - 1) / BIN_BITS;
    uint32 *data = (uint32 *) malloc(bins * sizeof(uint32));
    memset(data, 0, bins * sizeof(uint32));
    if (data == NULL)
    {
        printf("Memory allocation fail for %d bytes.\n", bins);
        exit(1);
    }
    counters->raw_bits = data;
    counters->size = size;
    counters->bits_per_counter = bits_per_counter;
    counters->num_bins = bins;
}

/**
 * Locate bin idx and bit idx given i-th counter.
*/
void get_bin_range(CounterBitSet *counters, int idx, int *bin_start, int *bin_end, int *bit_start, int *bit_end)
{
    int s = counters->bits_per_counter * idx;
    *bin_start = s >> 5;
    *bin_end = (s + counters->bits_per_counter - 1) >> 5;
    *bit_start = BIN_BITS - s % BIN_BITS;
    *bit_end = BIN_BITS - (s + counters->bits_per_counter - 1) % BIN_BITS;
}

/**
 * Test whether i-th counter > 0.
 * 
 * counters: pointer to CounterBitSet
 * idx: index of a queried counter
*/
int test_counter(CounterBitSet *counters, int idx)
{
    int bin_start = 0, bin_end = 0, bit_start = 0, bit_end = 0;
    get_bin_range(counters, idx, &bin_start, &bin_end, &bit_start, &bit_end);

    if (bin_start == bin_end)
    {
        return counters->raw_bits[bin_start] & GEN_BITS_RANGE(bit_end, bit_start);
    }
    else
    {
        return (counters->raw_bits[bin_start] & GEN_BITS_RANGE(1, bit_start)) || \
               (counters->raw_bits[bin_end] & GEN_BITS_RANGE(bit_end, BIN_BITS));
    }
}

int get_counter(CounterBitSet *counters, int idx)
{
    int bin_start = 0, bin_end = 0, bit_start = 0, bit_end = 0;
    get_bin_range(counters, idx, &bin_start, &bin_end, &bit_start, &bit_end);

    if (bin_start == bin_end)
    {
        return (counters->raw_bits[bin_start] & GEN_BITS_RANGE(bit_end, bit_start)) >> (bit_end - 1);
    }
    else
    {
        return ((counters->raw_bits[bin_start] & GEN_BITS_RANGE(1, bit_start))) << (BIN_BITS - bit_end + 1) | \
               (counters->raw_bits[bin_end] & GEN_BITS_RANGE(bit_end, BIN_BITS)) >> (bit_end - 1);
    }
}


/**
 * [REQUIRE FURTHER OPTIMIZED] Decrement i-th counter by 1.
 * 
 * counters: pointer to CounterBitSet
 * idx: index of a counter to be decremented
*/
void decrement(CounterBitSet *counters, int idx)
{
    int bin_start = 0, bin_end = 0, bit_start = 0, bit_end = 0;
    get_bin_range(counters, idx, &bin_start, &bin_end, &bit_start, &bit_end);

    // decrement the counter if is non-zero
    if (test_counter(counters, idx))
    {
        int temp = get_counter(counters, idx) - 1;
        if (bin_start == bin_end)
        {
            counters->raw_bits[bin_start] &= ~GEN_BITS_RANGE(bit_end, bit_start);
            counters->raw_bits[bin_start] |= (temp << (bit_end - 1));
        }
        else
        {
            counters->raw_bits[bin_start] &= ~GEN_BITS_RANGE(1, bit_start);
            counters->raw_bits[bin_start] |= (temp >> (BIN_BITS - bit_end + 1));
            counters->raw_bits[bin_end] &= ~GEN_BITS_RANGE(bit_end, BIN_BITS);
            counters->raw_bits[bin_end] |= (temp << (bit_end - 1));
        }
    }
}



/**
 * Set i-th counter to Max value, i.e., 2^bits - 1.
 * 
 * counters: pointer to CounterBitSet
 * idx: index of a counter to be set
*/
void set_to_max(CounterBitSet *counters, int idx)
{
    int bin_start = 0, bin_end = 0, bit_start = 0, bit_end = 0;
    get_bin_range(counters, idx, &bin_start, &bin_end, &bit_start, &bit_end);

    // printf("insert to %d, bin_start: %d, bin_end: %d, bit_start: %d, bit_end: %d\n", idx, bin_start, bin_end, bit_start, bit_end);

    if (bin_start == bin_end)
    {
        counters->raw_bits[bin_start] |= GEN_BITS_RANGE(bit_end, bit_start);
    }
    else
    {
        counters->raw_bits[bin_start] |= GEN_BITS_RANGE(1, bit_start);
        counters->raw_bits[bin_end] |= GEN_BITS_RANGE(bit_end, BIN_BITS);
    }
}


/**
 * Release memory.
 * 
 * counters: pointer to CounterBitSet
*/
void free_counters(CounterBitSet *counters)
{
    free(counters->raw_bits);
    counters->raw_bits = NULL;
}

/**
 * [DEBUG USAGE] Print values from i-th counter to j-th counter.
 * 
 * counters: pointer to CounterBitSet
 * start_idx: start index of counter to be printed
 * end_idx: end index of counter to be printed
*/
void print_counters(CounterBitSet *counters, int start_idx, int end_idx)
{
    printf("num of counters: %d\n", counters->size);
    printf("bits per counter: %d\n", counters->bits_per_counter);
    printf("num of bins: %d\n", counters->num_bins);
    
    printf("%d--%d bins are: ", start_idx, end_idx - 1);
    for (int i=start_idx; i<end_idx; ++i)
    {
        printf("%08x ", counters->raw_bits[i]);
    }
    printf("\n");
    printf("======================\n");
}
