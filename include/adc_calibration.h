#ifndef ADC_CALIBRATION_H
#define ADC_CALIBRATION_H

#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_err.h"

#include <stdbool.h>

typedef struct
{
    adc_cali_handle_t handle;
    bool is_calibrated;
} adc_cali_info_t;

esp_err_t adc_cali_create_line_fitting(adc_unit_t unit, adc_atten_t atten, adc_cali_info_t *cali_info);

esp_err_t adc_cali_convert_to_voltage(const adc_cali_info_t *cali_info, int raw_value, int *voltage_mv);

esp_err_t adc_cali_destroy_line_fitting(adc_cali_info_t *cali_info);

#endif