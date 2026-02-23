/**
 * @file    display.h
 * @brief   Interface para interação com o display ST7920
 */

#ifndef DISPLAY_H
#define DISPLAY_H

#include "u8g2.h"
#include "adc_utils.h"
#include "types.h"

#define MENU_OPTIONS 2

/**
 * @brief Inicializa o display com a biblioteca u8g2-hal
 *
 * @note Para esse projeto, é necessaria uma configuração especifica no u8g2_esp32_hal.c, adicionando "dev_config.flags = SPI_DEVICE_POSITIVE_CS" em u8g2_esp32_spi_byte_cb()
 */
void init_display();

/**
 * @brief Desenha a tela de menu principal
 *
 * @param selected_option Define a opção atual do menu
 */
void draw_menu(int selected_option);

/**
 * @brief Desenha a tela de configurações
 *
 * @param usb_mode Define o mode de USB para ser desenhado no menu "TIPO USB", fica salvo em NVS
 * @param selected_option Define a opção atual do menu
 * @param selected_channel Define o canal atual para leitura do ADC no menu "CANAIS"
 * @param adc_data Ponteiro para armazenar os dados da leitura do canal ADC
 * @param selected_input Define qual pino de input está ativo
 */
void draw_settings(uint8_t usb_mode, int selected_option, int selected_channel, adc_result_t *adc_data, int selected_input);

/**
 * @brief Desenha a tela de teste para os tipos de conector
 *
 * @param title Título da etapa
 * @param usb_types Define os tipos de conector a serem exibidos
 */
void draw_check_connectors_test(const char *title, int usb_types[2]);

/**
 * @brief Desenha a tela de teste para os pinos de dados do conector
 *
 * @param title Título da etapa
 * @param usb_types Define os tipos de conector para desenhar as linhas de acordo com os pinos necessários
 * @param values Armazena os valores lidos para cada pino, de acordo com o tipo de conector
 */
void draw_data_pins_test(const char *title, int usb_types[2], float values[10]);

/**
 * @brief Desenha a tela de teste para carga continua, com e sem corrente
 *
 * @param title Título da etapa
 * @param values Armazena os valores de tensão e corrente
 */
void draw_vcc_load_test(const char *title, float values[4]);

/**
 * @brief Desenha a tela de teste para curto automático
 *
 * @param title Título da etapa
 * @param values Armazena os valores de tensão
 * @param status Armazena as condições de SHUTDOWN e RECOVERY para cada saida.
 */
void draw_auto_short_test(const char *title, float values[2], int status[4]);

/**
 * @brief Desenha a tela de falha no teste
 *
 * @param step_title Título da etapa que resultou em falha
 * @param error_message Mensagem de erro definida na etapa falha, usada para indicar o problema
 */
void draw_test_fail_page(const char *step_title, const char *error_message);

/**
 * @brief Desenha a tela de teste concluido sem falhas
 */
void draw_test_pass_page();

#endif
