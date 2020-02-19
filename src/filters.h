#ifndef FILTERS_H
#define FILTERS_H

#include "./bitutils.h"
#include "./model.h"
#include "../include/isaac.h"

// Standard Bloom Filters
typedef struct BF
{
    CounterBitSet bitset;
    unsigned int *hash_codes;
    int K;
    int m;
} BF;

void init_bf(BF *bf, int K, int m);
void insert_bf(BF *bf, void *data, int length);
int test_bf(BF *bf, void *data, int length);
void free_bf(BF *bf);


// Stable Bloom Filters
typedef struct SBF
{
    CounterBitSet counters;
    isaac_ctx isaac;
    unsigned int *hash_codes;
    int P;
    int K;
    int m;
    int bits_per_counter;
} SBF;

void init_sbf(SBF *sbf, int P, int K, int m, int bits_per_counter);
void insert_sbf(SBF *sbf, void *data, int length);
int test_sbf(SBF *sbf, void *data, int length);
void free_sbf(SBF *sbf);


// Learned Bloom Filters
typedef struct LBF
{
    Model model;
    float tau;
    BF bf;
} LBF;


void init_lbf(LBF *lbf, Model *model, int K, int m, float tau);
void insert_lbf(LBF *lbf, Data *data, int length);
int test_lbf(LBF *lbf, Data *data, int length);
void free_lbf(LBF *lbf);


// Single Stable Learned Bloom Filters
typedef struct SSLBF
{
    Model model;
    float tau;
    SBF sbf;
} SSLBF;


void init_sslbf(SSLBF *sslbf, Model *model, int P, int K, int m, int bits_per_counter, float tau);
void insert_sslbf(SSLBF *sslbf, Data *data, int length);
int test_sslbf(SSLBF *sslbf, Data *data, int length);
void free_sslbf(SSLBF *sslbf);

// Grouping Stable Learned Bloom Filters
typedef struct GSLBF
{
    Model model;
    float *tau_array;
    SBF *SBF_array;
    int g;
} GSLBF;

void init_gslbf(GSLBF *gslbf, Model *model, int *P_array, int *K_array, int *m_array, int *bits_per_counter_array, float *tau_array, int g);
void insert_gslbf(GSLBF *gslbf, Data *data, int length);
int test_gslbf(GSLBF *gslbf, Data *data, int length);
void free_gslbf(GSLBF *gslbf);

#endif
