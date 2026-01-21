#include "adc_utils.h"
#include "math.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "ADC_UTILS";
static adc_oneshot_unit_handle_t adc1_handle, adc2_handle;

const int samples = 10;

esp_err_t adc_init()
{
    esp_err_t ret;

    // ADC1
    adc_oneshot_unit_init_cfg_t init_config1 = {
        .unit_id = ADC_UNIT_1,
    };
    ret = adc_oneshot_new_unit(&init_config1, &adc1_handle);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Falha ao inicializar ADC1: %s", esp_err_to_name(ret));
    }

    // ADC2
    adc_oneshot_unit_init_cfg_t init_config2 = {
        .unit_id = ADC_UNIT_2,
    };
    ret = adc_oneshot_new_unit(&init_config2, &adc2_handle);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Falha ao inicializar ADC2: %s", esp_err_to_name(ret));
        adc_oneshot_del_unit(adc1_handle);
    }

    // Configurar os canais
    for (int i = 0; i < 12; i++)
    {
        adc_oneshot_chan_cfg_t config = {
            .bitwidth = ADC_BITWIDTH_DEFAULT,
            .atten = adc_channels[i]->atten,
        };

        adc_oneshot_unit_handle_t handle = (adc_channels[i]->unit == ADC_UNIT_1) ? adc1_handle : adc2_handle;
        ret = adc_oneshot_config_channel(handle, adc_channels[i]->channel, &config);
        if (ret != ESP_OK)
        {
            ESP_LOGE(TAG, "Falha ao configurar %s (GPIO%d): %s", adc_channels[i]->name, adc_channels[i]->gpio_pin, esp_err_to_name(ret));
            continue;
        }

        ret = adc_cali_create_line_fitting(adc_channels[i]->unit, adc_channels[i]->atten, &adc_channels[i]->cali_info);
    }

    return ESP_OK;
}

static float calculate_reading_to_voltage(int voltage_avg)
{
    //Possivelmente ajustar o calculo depois
    return (voltage_avg - 34.5) / 582.5;
}

esp_err_t read_channel(Channel_Name channel_name, float *voltage, int *adc_voltage_mv)
{
    adc_channel_config_t *ch = adc_channels[channel_name];
    adc_oneshot_unit_handle_t handle = (ch->unit == ADC_UNIT_1) ? adc1_handle : adc2_handle;
    int raw_value, voltage_sample;
    long voltage_sum = 0;
    esp_err_t ret;

    for (int i = 0; i < samples; i++)
    {
        ret = adc_oneshot_read(handle, ch->channel, &raw_value);
        if (ret != ESP_OK)
        {
            return ret;
        }

        ret = adc_cali_convert_to_voltage(&ch->cali_info, raw_value, &voltage_sample);
        if (ret != ESP_OK)
        {
            return ret;
        }

        voltage_sum += voltage_sample;
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    int voltage_avg = (voltage_sum / samples);
    if (adc_voltage_mv != NULL)
    {
        *adc_voltage_mv = voltage_avg > 1000 ? voltage_avg : 0;
    }

    if (voltage != NULL)
    {
        if (voltage_avg > 1000)
        {
            *voltage = calculate_reading_to_voltage(voltage_avg);
            *voltage *= ch->needs_2x ? 2 : 1;
            if (channel_name == CA || channel_name == CB)
            {
                *voltage -= 1.0;
            }
        }
        else
        {
            *voltage = 0.0;
        }
    }

    return ESP_OK;
}

esp_err_t read_all_channels(float *voltage_values)
{
    esp_err_t ret;
    int voltage_avg;
    if (voltage_values == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    for (int i = 0; i < 12; i++)
    {
        ret = read_channel(i, &voltage_values[i], &voltage_avg);
        if (ret != ESP_OK)
        {
            voltage_values[i] = -1;
        }
    }

    return ESP_OK;
}

void adc_deinit()
{
    for (int i = 0; i < 12; i++)
    {
        adc_cali_destroy_line_fitting(&adc_channels[i]->cali_info);
    }

    adc_oneshot_del_unit(adc1_handle);
    adc_oneshot_del_unit(adc2_handle);
}