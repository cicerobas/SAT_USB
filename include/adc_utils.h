#ifndef ADC_UTILS_H
#define ADC_UTILS_H

#include "esp_err.h"
#include "adc_calibration.h"

typedef enum
{
    A_CC1,
    A_CC2,
    A_DN,
    A_DP,
    A_VCC,
    B_CC1,
    B_CC2,
    B_DN,
    B_DP,
    B_VCC,
    CA,
    CB
} Channel_Name;

typedef struct
{
    adc_unit_t unit;
    adc_channel_t channel;
    adc_atten_t atten;
    adc_cali_info_t cali_info;
    const char *name;
    int gpio_pin;
    int needs_2x;
} adc_channel_config_t;

extern adc_channel_config_t *adc_channels[12];

esp_err_t adc_init();
esp_err_t read_channel(Channel_Name channel_name, int *adc_voltage_mv);
float convert_reading(int adc_voltage_mv);
#endif