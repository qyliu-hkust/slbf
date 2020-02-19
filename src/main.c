#include "stdio.h"
#include "string.h"
#include "stdlib.h"
#include "math.h"
#include "time.h"

#ifdef WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

#include "./filters.h"
#include "../include/isaac.h"

#define PI 3.14159265358979

static const unsigned char *ISAAC_SEED = (unsigned char*) "22333322";

/**
 * Generate Gaussian distributed random numbers.
*/
static float gauss_rand(isaac_ctx *isaac)
{
    float u = isaac_next_float(isaac);
    float v = isaac_next_float(isaac);
    return cosf(2 * PI * v) * sqrtf(-2 * logf(u));
}

/**
 * Test standard Bloom filter performance.
*/
static void exp_bf()
{
    // setting of expected 10000000 elements with 0.01 false positive rate
    int max_range = 10000000;
    int m = max_range * 9.584;
    int k = 6;

    BF bf;
    init_bf(&bf, k, m);

    clock_t start, end;
    
    start = clock();
    for (int i=0; i<max_range; ++i)
    {
        insert_bf(&bf, &i, sizeof(int));
    }
    end = clock();
    printf("Inserting %d items using time: %.3f sec.\n", max_range, (end - start)/(float)CLOCKS_PER_SEC);

    // test query
    start = clock();
    for (int i=0; i<max_range; ++i)
    {
        if (! test_bf(&bf, &i, sizeof(int)))
        {
            printf("counters: ");
            for (int ii=0; ii<k; ++ii)
            {
                printf("%d ", get_counter(&(bf.bitset), bf.hash_codes[ii]));
            }
            printf("\n");
            printf("false negative %d\n", i);
        }
    }
    end = clock();
    printf("pass false negative test.\n");
    printf("Querying %d items using time: %.3f sec.\n", max_range, (end - start)/(float)CLOCKS_PER_SEC);

    // false positive rate
    int wrong = 0;
    int total = 0;
    for (int i=max_range; i<max_range * 2; ++i)
    {
        if (test_bf(&bf, &i, sizeof(int)))
        {
            wrong++;
        }
        total++;
    }
    
    printf("false positive rate is: %.5f\n", (float)wrong / total);
    free_bf(&bf);
}

/**
 * Calculate the ratio of counters with zero in an SBF.
*/
static float get_zero_ratio(SBF *sbf)
{
    int zero_count = 0;
    for (int i=0; i<sbf->m; i++)
    {
        if (! test_counter(&(sbf->counters), i))
        {
            zero_count ++;
        }
    }
    return (float)zero_count / sbf->m;
}

/**
 * Test SBF performance.
*/
static void exp_sbf()
{
    int max_range = 100000;
    int m = 10000;
    int K = 6;
    int P = 6;
    int bits_per_counter = 3;

    SBF sbf;
    init_sbf(&sbf, P, K, m, bits_per_counter);

    // insert
    for (int i=0; i<max_range; ++i)
    {
        insert_sbf(&sbf, &i, sizeof(int));
        if (! ((i+1) % 1000))
        {
            printf("%d iteration: zero rate is \033[40;31m%.5f\%\033[0m.\n", i+1, 100 * get_zero_ratio(&sbf));
        }
    }

    free_sbf(&sbf);
}

int main(int argc, char const *argv[])
{
    // parse command line arguments

    // exp_bf();
    exp_sbf();

    return 0;
}
