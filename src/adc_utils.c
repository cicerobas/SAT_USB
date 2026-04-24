#include "adc_utils.h"
#include "math.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "ADC_UTILS";
static adc_oneshot_unit_handle_t adc1_handle, adc2_handle;

const int samples = 100;

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

/**
 * @brief Converte o valor em mV para V a partir das medições feitas em calibração
 *
 * @note Não alterar os coeficiêntes sem uma nova calibração
 */
static float convert_voltage_reading(int adc_voltage_mv)
{
    // 0.0000002135f * adc_f * adc_f + 0.001102682f * adc_f + 0.366791839f;
    float adc_f = (float)adc_voltage_mv;
    float result = 0.0000002135f * adc_f * adc_f + 0.001102682f * adc_f + 0.366791839f;
    return result;
}

static float convert_load_reading(int adc_voltage_mv)
{
    return 1.0 + (adc_voltage_mv - 1563) * 0.0032258;
}

esp_err_t read_channel(adc_result_t *adc_data)
{
    adc_channel_config_t *ch = adc_channels[adc_data->channel];
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
        vTaskDelay(pdMS_TO_TICKS(1));
    }
    int voltage_avg = (voltage_sum / samples);
    adc_data->value_mv = 0;

    // offset é usado para ignorar leituras de "ruido" ou valores baixos no ADC
    int offset = (adc_data->channel == CA || adc_data->channel == CB) ? 1400 : 1000;

    if (voltage_avg > offset)
    {
        adc_data->value_mv = voltage_avg;
        float converted = (adc_data->channel == CA || adc_data->channel == CB) ? convert_load_reading(voltage_avg) : convert_voltage_reading(voltage_avg);
        adc_data->converted_value = ch->is_5V ? converted * 2 : converted; // Multiplica x2 o valor convertido caso o pino seja de 5V
    }

    return ESP_OK;
}
