#include "adc_calibration.h"
#include "esp_log.h"

static const char *TAG = "ADC_CALI";

esp_err_t adc_cali_create_line_fitting(adc_unit_t unit, adc_atten_t atten, adc_cali_info_t *cali_info)
{
    if (cali_info == NULL)
    {
        ESP_LOGE(TAG, "Ponteiro cali_info é NULL");
        return ESP_ERR_INVALID_ARG;
    }

    cali_info->handle = NULL;
    cali_info->is_calibrated = false;

    adc_cali_line_fitting_config_t cali_config = {
        .unit_id = unit,
        .atten = atten,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };

    esp_err_t ret = adc_cali_create_scheme_line_fitting(&cali_config, &cali_info->handle);
    if (ret == ESP_OK)
    {
        cali_info->is_calibrated = true;
        ESP_LOGI(TAG, "Calibração Line Fitting criada - ADC%d, Atten: %d", unit + 1, atten);
    }
    else if (ret == ESP_ERR_NOT_SUPPORTED)
    {
        ESP_LOGW(TAG, "eFuse não gravado, pulando calibração - ADC%d", unit + 1);
    }
    else
    {
        ESP_LOGE(TAG, "Erro ao criar calibração - ADC%d: %s", unit + 1, esp_err_to_name(ret));
    }

    return ret;
}

esp_err_t adc_cali_convert_to_voltage(const adc_cali_info_t *cali_info, int raw_value, int *voltage_mv)
{
    if (cali_info == NULL || voltage_mv == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    if (!cali_info->is_calibrated)
    {
        ESP_LOGD(TAG, "Canal não calibrado, impossível converter");
        return ESP_ERR_INVALID_STATE;
    }

    return adc_cali_raw_to_voltage(cali_info->handle, raw_value, voltage_mv);
}

esp_err_t adc_cali_destroy_line_fitting(adc_cali_info_t *cali_info)
{
    if (cali_info == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    if (!cali_info->is_calibrated || cali_info->handle == NULL)
    {
        ESP_LOGD(TAG, "Nada para destruir - calibração não ativa");
        return ESP_OK;
    }

    esp_err_t ret = adc_cali_delete_scheme_line_fitting(cali_info->handle);
    if (ret == ESP_OK)
    {
        cali_info->handle = NULL;
        cali_info->is_calibrated = false;
        ESP_LOGD(TAG, "Calibração destruída com sucesso");
    }
    else
    {
        ESP_LOGE(TAG, "Erro ao destruir calibração: %s", esp_err_to_name(ret));
    }
    return ret;
}