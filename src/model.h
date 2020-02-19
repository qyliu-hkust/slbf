#ifndef MODEL_H
#define MODEL_H

#include "../include/c_api.h"

typedef struct Data
{
    unsigned int id;
    float *float_features;
    int num_float_features;
    const char **cat_features;
    int num_cat_features;
} Data;


typedef enum ModelType
{
    LOGISTIC,
    BOOST
} ModelType;

typedef struct Model
{
    ModelType type;
    // for logistic model
    float *weights;
    int num_weights;
    // for boost model (using catboost)
    ModelCalcerHandle *catboost_model_handle;
} Model;


void load_model(Model *model, ModelType type, char *path);
void free_model(Model *model);
float predict(Model *model, Data *data);
float predict_logistic(Model *model, Data *data);
float predict_boost(Model *model, Data *data);

#endif
