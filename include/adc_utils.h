/**
 * @file    adc_utils.h
 * @brief   Interface para interação direta nos canais ADC no ESP32
 */

#ifndef ADC_UTILS_H
#define ADC_UTILS_H

#include "esp_err.h"
#include "adc_calibration.h"
#include "types.h"

/**
 * @brief Enum para nomear os pinos de leitura de acordo com a posição.
 */
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

/**
 * @brief Estrutura para armazenar as configurações de cada canal ADC.
 */
typedef struct
{
    adc_unit_t unit;           /**< Unidade do ADC (ADC_UNIT_1 ou ADC_UNIT_2) */
    adc_channel_t channel;     /**< Canal do ADC */
    adc_atten_t atten;         /**< Atenuação do canal (padrão: ADC_ATTEN_DB_12) */
    adc_cali_info_t cali_info; /**< Estrutura que armazena as informações da calibração do canal */
    const char *name;          /**< Nome do pino, usado para referência com Channel_Name */
    int gpio_pin;              /**< Número GPIO do pino. */
    int is_5V;                 /**< Indica de o canal lido deve ser calculado para a faixa de 5V */
} adc_channel_config_t;

extern adc_channel_config_t *adc_channels[12];

/**
 * @brief Inicializa a configuração e calibração dos canais ADC
 */
esp_err_t adc_init();

/**
 * @brief Faz a leitura do canal ADC.
 *
 * @param adc_data Estrutura para armazenar os valores da leitura.
 * @return esp_err_t ESP_OK se sucesso
 */
esp_err_t read_channel(adc_result_t *adc_data);

#endif