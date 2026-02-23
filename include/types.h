/**
 * @file    types.h
 * @brief   Interface para definição de tipos customizados
 */

#ifndef TYPES_H
#define TYPES_H

#include "adc_utils.h"
#include <stdbool.h>

/**
 * @brief Estrutura para armazenar os dados para leitura no canal ADC
 */
typedef struct
{
    int channel;           /**< Define o Channel_Name do canal para ser lido */
    int value_mv;          /**< Armazena o valor lido e convertido em mV */
    float converted_value; /**< Armazena o valor convertido de mV para V */
} adc_result_t;

/**
 * @brief Estrutura para armazenar o status da etapa atual do teste
 */
typedef struct
{
    int index;        /**< Armazena o indice da etapa atual do teste */
    bool status;      /**< Indica se a etapa foi aprovada ou reprovada (true/false) */
    char message[64]; /**< Armazena a mensagem de erro definida pela etapa em caso de falha */
} step_status_t;

/**
 * @brief Estrutura para armazenar as informações dos pinos USB e limites de validação
 */
typedef struct
{
    int pin;          /**< Armazena o Channel_Name do pino*/
    float high_limit; /**< Define o limite superior para o valor desse pino */
    float low_limit;  /**< Define o limite inferior para o valor desse pino */
    bool required;    /**< Define se a leitura do pino é necessária para a etapa */
} pin_info_t;

/**
 * @brief Usado para indicar se a leitura do pino ficou na faixa(OK), abaixo(BELOW) ou acima(ABOVE) do limite do pino
 */
typedef enum
{
    VALUE_BELOW = -1,
    VALUE_OK,
    VALUE_ABOVE
} pin_result_t;

/**
 * @brief Usado para definir os tipos de teste
 */
typedef enum
{
    CHANGE_INPUT_SOURCE,
    CHECK_CONNECTORS,
    DATA_PINS,
    MINIMUM_LOAD,
    MAXIMUM_LOAD,
    AUTOMATIC_SHORT,
} test_step_type_t;

/**
 * @brief Estrutura para armazenar informações de cada etapa
 */
typedef struct
{
    test_step_type_t type;  /**< Armazena o tipo (test_step_type_t) de cada etapa */
    const char *step_title; /**< Armazena o título da etapa */
} test_step_info_t;

#endif