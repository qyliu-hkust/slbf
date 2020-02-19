#include "math.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"

#include "./model.h"
#include "../include/c_api.h"


/**
 * Load classifier from file. 
 * Currently support logistic regression model and boosting tree model using Catboost.
 * 
 * model: pointer to a Model
 * type: model type
 * path: path to model file 
*/
void load_model(Model *model, ModelType type, char *path)
{
    if (type == LOGISTIC)
    {
        FILE *fp = fopen(path, "r");
        char buffer[64];
        if (fp == NULL)
        {
            printf("Read file failed.");
            exit(1);
        }

        float *weights = NULL;
        int num_weights = 0;
        int first_flag = 1;
        int count = 0;

        while (! feof(fp))
        {
            memset(buffer, 0, sizeof(buffer));
            fgets(buffer, sizeof(buffer)-1, fp);
            if (first_flag)
            {
                num_weights = atoi(buffer);
                weights = (float *)malloc(num_weights * sizeof(float));
                first_flag = 0;
            }
            else
            {
                weights[count++] = atof(buffer);
            }
        }
        fclose(fp);

        if ((count == num_weights) && (!weights)) 
        {
            model->type = LOGISTIC;
            model->num_weights = num_weights;
            model->weights = weights;
        }
        else
        {
            printf("model format error.");
            exit(1);
        }
        
    }
    else if (type == BOOST)
    {
        if (! LoadFullModelFromFile(model->catboost_model_handle, path))
        {
            printf("Load Catboost model failed. Error message: %s.\n", GetErrorString());
            exit(1);
        }
        model->type = BOOST;
    }
    else
    {
        printf("Unsupported model type.");
        exit(1);
    }
}


/**
 * Release memory allocated to model.
 * 
 * model: pointer to Model
*/
void free_model(Model *model)
{
    if (model->type == LOGISTIC)
    {
        free(model->weights);
        model->weights = NULL;
    }
    else if (model->type == BOOST)
    {
        ModelCalcerDelete(model->catboost_model_handle);
    }
}

float predict(Model *model, Data *data)
{
    if (model->type == LOGISTIC)
    {
        return predict_logistic(model, data);
    }
    else 
    {
        return predict_boost(model, data);
    }
}


/**
 * Make prediction using logistic model.
 * 
 * data: pointer to Data
 * model: pointer to Model
*/
float predict_logistic(Model *model, Data *data)
{
    float sum = 0;
    for (int i=0; i<data->num_float_features; ++i)
    {
        sum -= data->float_features[i] * model->weights[i];
    }
    return 1 / (1 + exp(sum));
}


/**
 * Make prediction using boosting model.
 * 
 * data: pointer to Data
 * model: pointer to Model
*/
float predict_boost( Model *model, Data *data)
{
    double prediction;
    if (! CalcModelPredictionSingle(
        model->catboost_model_handle,
        data->float_features, data->num_float_features,
        data->cat_features, data->num_cat_features,
        &prediction, 1
    )) 
    { 
        printf("CalcModelPrediction error message: %s\n", GetErrorString());
    }
    return prediction;
}

