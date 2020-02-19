#include "../include/xxhash.h"
#include "../include/isaac.h"
#include "../include/ilog.h"
#include "./filters.h"
#include "./bitutils.h"

#include "stdio.h"
#include "stdlib.h"


#define RANDOM_SEED1 123456789
#define RANDOM_SEED2 987654321

static const unsigned char *ISAAC_SEED = (unsigned char*)"22333322";

/**
 * Generate k independent uniformly distributed hash values in range 0...m-1.
 * For the theory, see https://www.eecs.harvard.edu/~michaelm/postscripts/rsa2008.pdf
 * 
 * data: pointer to the data
 * length: size of data to calculate hash codes (num of bytes)
 * k: number of hash functions
 * m: range of hash codes
 * hash_codes: pointer to the result to be stored
*/
void gen_k_hash32(const void *data, int length, int k, int m, unsigned int *hash_codes)
{
    unsigned int h1 = XXH32(data, length, RANDOM_SEED1);
    unsigned int h2 = XXH32(data, length, RANDOM_SEED2);

    for (int i=0; i<k; ++i)
    {
        hash_codes[i] = (h1 + i * h2) % m;
    }
}


/**
 * Init a standard Bloom filter.
 * 
 * bf: pointer to a BF
 * K: number of hash functions
 * m: number of bits
*/
void init_bf(BF *bf, int K, int m)
{
    CounterBitSet bitset;
    init_counters(&bitset, m, 1);
    
    bf->bitset = bitset;
    bf->K = K;
    bf->m = m;

    bf->hash_codes = (unsigned int *)malloc(K * sizeof(unsigned int));
}

/**
 * Insert an element to Bloom filter.
 * 
 * bf: pointer to a BF
 * data: pointer to the element to be inserted
 * length: length of data (number of bytes used to calculate hash values)
*/
void insert_bf(BF *bf, void *data, int length)
{
    gen_k_hash32(data, length, bf->K, bf->m, bf->hash_codes);
    for (int i=0; i<bf->K; ++i)
    {
        set_to_max(&(bf->bitset), bf->hash_codes[i]);
    }
}

/**
 * Membership query processing.
 * 
 * bf: pointer to a BF
 * data: pointer to the queried element
 * length: length of data (number of bytes used to calculate hash values)
*/
int test_bf(BF *bf, void *data, int length)
{
    gen_k_hash32(data, length, bf->K, bf->m, bf->hash_codes);
    for (int i=0; i<bf->K; ++i)
    {
        if (! test_counter(&(bf->bitset), bf->hash_codes[i]))
        {
            return 0;
        }
    }
    return 1;
}

/**
 * Release memory allocated to BF.
 * 
 * bf: pointer to a BF
*/
void free_bf(BF *bf)
{
    free_counters(&(bf->bitset));
    free(bf->hash_codes);
}

/**
 * Init a stable Bloom filter.
 * 
 * sbf: pointer to an SBF
 * P: number of counters to be decremented
 * K: number of counters to be set
 * m: number of counters
 * bits_per_counter: bits used per counter
*/
void init_sbf(SBF *sbf, int P, int K, int m, int bits_per_counter)
{
    CounterBitSet counters;
    init_counters(&counters, m, bits_per_counter);

    sbf->counters = counters;
    sbf->P = P;
    sbf->K = K;
    sbf->m = m;
    
    sbf->hash_codes = (unsigned int *)malloc(K * sizeof(unsigned int));

    isaac_ctx isaac;
    isaac_init(&isaac, ISAAC_SEED, sizeof(ISAAC_SEED));
    sbf->isaac = isaac;
}

/**
 * Insert an element to an SBF.
 * 
 * sbf: pointer to an SBF
 * data: pointer to element to be inserted
 * length: length of data (number of bytes used to calculate hash values)
*/
void insert_sbf(SBF *sbf, void *data, int length)
{
    // first decrement P counters
    for (int i=0; i<sbf->P; ++i)
    {
        decrement(&(sbf->counters), isaac_next_uint(&(sbf->isaac), sbf->m));
    }
    // then set K counters to Max
    gen_k_hash32(data, length, sbf->K, sbf->m, sbf->hash_codes);
    for (int i=0; i<sbf->K; ++i)
    {
        set_to_max(&(sbf->counters), sbf->hash_codes[i]);
    }
}

/**
 * Membership query processing.
 * 
 * sbf: pointer to an SBF
 * data: pointer to element to be inserted
 * length: length of data (number of bytes used to calculate hash values)
*/
int test_sbf(SBF *sbf, void *data, int length)
{
    gen_k_hash32(data, length, sbf->K, sbf->m, sbf->hash_codes);
    for (int i=0; i<sbf->K; ++i)
    {
        if (! test_counter(&(sbf->counters), sbf->hash_codes[i]))
        {
            return 0;
        }
    }
    return 1;
}

/**
 * Release memory allocated to sbf.
 * 
 * sbf: pointer to an SBF
*/
void free_sbf(SBF *sbf)
{
    free_counters(&(sbf->counters));
    free(sbf->hash_codes);
}


/**
 * Init a Learned Bloom filter.
 * 
 * lbf: pointer to an LBF
 * model: pointer to a Model
 * K: number of hash functions used in the backup filter
 * m: number of bits used in the backup filter
 * tau: decision threshold of the model
*/
void init_lbf(LBF *lbf, Model *model, int K, int m, float tau)
{
    BF bf;
    init_bf(&bf, K, m);

    lbf->bf = bf;
    lbf->tau = tau;
    lbf->model = *model;
}


/**
 * Insert an element to the learned Bloom filter.
 * 
 * lbf: pointer to an LBF
 * data: pointer to the Data object to be inserted
 * length: number of bytes to be used to calculate hash codes
*/
void insert_lbf(LBF *lbf, Data *data, int length)
{
    if (predict(&(lbf->model), data) < lbf->tau)
    {
        insert_bf(&(lbf->bf), &(data->id), length);
    }
}


/**
 * Membership test query processing.
 * 
 * lbf: pointer to an LBF
 * data: pointer to the Data object to be inserted
 * length: number of bytes to be used to calculate hash codes
*/
int test_lbf(LBF *lbf, Data *data, int length)
{
    if (predict(&(lbf->model), data) > lbf->tau)
    {
        return 1;
    }
    else
    {
        return test_bf(&(lbf->bf), &(data->id), length);
    }
}

/**
 * Release memory allocated to lbf.
 * 
 * lbf: pointer to an LBF
*/
void free_lbf(LBF *lbf)
{
    free_bf(&(lbf->bf));
    if (lbf->model.catboost_model_handle)
    {
        ModelCalcerDelete(lbf->model.catboost_model_handle);
    }
}

/**
 * Init a Single Stable Learned Bloom filter.
 * 
 * sslbf: pointer to an SSLBF
 * model: pointer to a Model
 * P: number of counters to be decremented
 * K: number of counters to be set
 * m: number of counters
 * bits_per_counter: bits used per counter
 * tau: decision threshold of the model
*/
void init_sslbf(SSLBF *sslbf, Model *model, int P, int K, int m, int bits_per_counter, float tau)
{
    SBF sbf;
    init_sbf(&sbf, P, K, m, bits_per_counter);

    sslbf->sbf = sbf;
    sslbf->model = *model;
    sslbf->tau = tau;
}

/**
 * Insert an element to the SSLBF.
 * 
 * sslbf: pointer to an SSLBF
 * data: pointer to the Data object to be inserted
 * length: number of bytes to be used to calculate hash codes
*/
void insert_sslbf(SSLBF *sslbf, Data *data, int length)
{
    if (predict(&(sslbf->model), data) < sslbf->tau)
    {
        insert_sbf(&(sslbf->sbf), &(data->id), length);
    }
}

/**
 * Membership test query processing.
 * 
 * sslbf: pointer to an SSLBF
 * data: pointer to the Data object to be inserted
 * length: number of bytes to be used to calculate hash codes
*/
int test_sslbf(SSLBF *sslbf, Data *data, int length)
{
    if (predict(&(sslbf->model), data) > sslbf->tau)
    {
        return 1;
    }
    else
    {
        return test_sbf(&(sslbf->sbf), &(data->id), length);
    }
}

/**
 * Release memory allocated to sslbf.
 * 
 * sslbf: pointer to an SSLBF
*/
void free_sslbf(SSLBF *sslbf)
{
    ModelCalcerDelete(&(sslbf->model));
    free_sbf(&(sslbf->sbf));
}

/**
 * Find idx of x belonging to which interval
*/
static int lookup_interval(float *intervals, int len, float x)
{
    int lo = 0, hi = len;
    int idx;
    while ((hi - lo) > 1)
    {
        idx = lo + (hi - lo) / 2;
        if (x > intervals[idx])
        {
            lo = idx;
        }
        else
        {
            hi = idx;
        }
    }
    return lo;
}

/**
 * Init a Grouping Stable Learned Bloom filter.
 * 
 * gslbf: pointer to an GSLBF
 * model: pointer to a Model
 * P_array: array of parameter P [of size g]
 * K_array: array of parameter K [of size g]
 * m_array: array of parameter m [of size g]
 * bits_per_counter_array: array of parameter bits_per_counter [of size g]
 * tau_array: array of decision thresholds [of size g+1]
 * g: number of groups
*/
void init_gslbf(GSLBF *gslbf, Model *model, int *P_array, int *K_array, int *m_array, int *bits_per_counter_array, float *tau_array, int g)
{
    SBF *SBF_array = (SBF *)malloc(g * sizeof(SBF));
    for (int i=0; i<g; ++i)
    {
        SBF sbf;
        init_sbf(&sbf, P_array[i], K_array[i], m_array[i], bits_per_counter_array[i]);
        SBF_array[i] = sbf;
    }
    gslbf->SBF_array = SBF_array;
    gslbf->model = *model;
    gslbf->tau_array = tau_array;
    gslbf->g = g;
}

/**
 * Insert an element to the GSLBF.
 * 
 * gsslbf: pointer to an GSSLBF
 * data: pointer to the Data object to be inserted
 * length: number of bytes to be used to calculate hash codes
*/
void insert_gslbf(GSLBF *gslbf, Data *data, int length)
{
    float score = predict(&(gslbf->model), data);
    int idx = lookup_interval(gslbf->tau_array, gslbf->g + 1, score);
    insert_sbf(&(gslbf->SBF_array[idx]), data, length);
}

/**
 * Membership test query processing.
 * 
 * gslbf: pointer to an GSLBF
 * data: pointer to the Data object to be inserted
 * length: number of bytes to be used to calculate hash codes
*/
int test_gslbf(GSLBF *gslbf, Data *data, int length)
{
    float score = predict(&(gslbf->model), data);
    int idx = lookup_interval(gslbf->tau_array, gslbf->g + 1, score);
    return test_sbf(&(gslbf->SBF_array[idx]), data, length);
}

/**
 * Release memory allocated to gslbf.
 * 
 * gslbf: pointer to an GSLBF
*/
void free_gslbf(GSLBF *gslbf)
{
    ModelCalcerDelete(&(gslbf->model));
    free(gslbf->SBF_array);
    gslbf->SBF_array = NULL;
}
