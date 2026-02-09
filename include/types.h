#ifndef TYPES_H
#define TYPES_H

#include "adc_utils.h"

typedef struct
{
    Channel_Name channels[12];
    uint8_t num_channels;
} adc_request_t;

typedef struct
{
    int values[12];
    float converted_values[12];
    uint8_t num_values;
} adc_response_t;

#endif