/**
 * @file    adc_calibration.h
 * @brief   Interface para calibração de canais ADC no ESP32
 */

#ifndef ADC_CALIBRATION_H
#define ADC_CALIBRATION_H

#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_err.h"

#include <stdbool.h>

/**
 * @brief Estrutura para armazenar informações de calibração do canal ADC
 */
typedef struct
{
    adc_cali_handle_t handle; /**< Handle de calibração do ESP-IDF */
    bool is_calibrated;       /**< Indica se a calibração foi realizada com sucesso */
} adc_cali_info_t;

/**
 * @brief Cria calibração line fitting para um canal ADC
 *
 * @param unit Unidade ADC (ADC_UNIT_1 ou ADC_UNIT_2)
 * @param atten Atenuação do canal
 * @param cali_info Ponteiro para estrutura que armazenará as informações de calibração
 * @return esp_err_t ESP_OK se sucesso, erro caso contrário
 */
esp_err_t adc_cali_create_line_fitting(adc_unit_t unit, adc_atten_t atten, adc_cali_info_t *cali_info);

/**
 * @brief Converte valor raw ADC para tensão em mV usando calibração
 *
 * @param cali_info Informações de calibração
 * @param raw_value Valor raw do ADC
 * @param voltage_mv Ponteiro para armazenar tensão em mV
 * @return esp_err_t ESP_OK se sucesso, ESP_ERR_INVALID_STATE se não calibrado
 */
esp_err_t adc_cali_convert_to_voltage(const adc_cali_info_t *cali_info, int raw_value, int *voltage_mv);

/**
 * @brief Destroi calibração line fitting
 *
 * @param cali_info Informações de calibração a serem destruídas
 * @return esp_err_t ESP_OK se sucesso
 */
esp_err_t adc_cali_destroy_line_fitting(adc_cali_info_t *cali_info);

#endif