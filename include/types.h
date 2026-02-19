#ifndef TYPES_H
#define TYPES_H

#include "adc_utils.h"
#include <stdbool.h>

typedef struct
{
    int channel;
    int value_mv;
    float converted_value;
} adc_result_t;

typedef struct
{
    int index;
    bool status;
    char message[64];
} step_status_t;

typedef struct
{
    int pin;
    float high_limit;
    float low_limit;
    bool required;
} pin_info_t;

typedef enum
{
    CHANGE_INPUT_SOURCE,
    CHECK_CONNECTORS,
    MINIMUM_LOAD,
    MAXIMUM_LOAD,
    AUTOMATIC_SHORT,
} test_step_type_t;

typedef struct
{
    test_step_type_t type;
    const char *step_title;
} test_step_info_t;

#endif