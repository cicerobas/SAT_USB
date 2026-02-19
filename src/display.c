#include "display.h"
#include "u8g2_esp32_hal.h"
#include "esp_log.h"

#include <stdbool.h>

#define SCLK_PIN 18
#define SID_PIN 23
#define CS_PIN 5

#define D_WIDTH 128
#define D_HEIGHT 64

#define CHAR_C_CEDILHA "\xC7"
#define CHAR_O_TIL "\xD5"

// 32x16
const uint8_t logo_bitmap[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0xC0, 0x00, 0x00, 0x01, 0x80, 0x00, 0x00, 0x01, 0x80, 0x00,
                               0x00, 0x01, 0x80, 0x00, 0x01, 0xFD, 0xBF, 0x80, 0x0F, 0xFD, 0xBF, 0xF0, 0x3F, 0xFD, 0xBF, 0xFC,
                               0x7F, 0xF8, 0x1F, 0xFE, 0xFF, 0xE0, 0x07, 0xFF, 0xFF, 0xE0, 0x07, 0xFF, 0x7F, 0xF8, 0x1F, 0xFE,
                               0x3F, 0xFF, 0xFF, 0xFC, 0x0F, 0xFF, 0xFF, 0xF0, 0x01, 0xFF, 0xFF, 0x80, 0x00, 0x00, 0x00, 0x00};
// 32x16
const uint8_t usb_a_bitmap[] = {0x7F, 0xFF, 0xFF, 0xFE, 0xFF, 0xFF, 0xFF, 0xFF, 0xC0, 0x00, 0x00, 0x03, 0xC0, 0x00, 0x00, 0x03,
                                0xC0, 0x00, 0x00, 0x03, 0xCF, 0xFF, 0xFF, 0xF3, 0xCF, 0xFF, 0xFF, 0xF3, 0xCF, 0xFF, 0xFF, 0xF3,
                                0xC3, 0x9E, 0x79, 0xC3, 0xC0, 0x00, 0x00, 0x03, 0xC0, 0x00, 0x00, 0x03, 0xC0, 0x00, 0x00, 0x03,
                                0xC0, 0x00, 0x00, 0x03, 0xC0, 0x00, 0x00, 0x03, 0xFF, 0xFF, 0xFF, 0xFF, 0x7F, 0xFF, 0xFF, 0xFE};
// 24x8
const uint8_t usb_c_bitmap[] = {0x3f, 0xff, 0xfc, 0x60, 0x0, 0x6, 0xc0, 0x0, 0x3, 0x8f, 0xff, 0xf1, 0x8f, 0xff, 0xf1, 0xc0, 0x0, 0x3, 0x60, 0x0, 0x6, 0x3f, 0xff, 0xfc};
// 8x8
const uint8_t arrow[] = {0x8, 0xc, 0xfe, 0xff, 0xff, 0xfe, 0xc, 0x8};

static const char *TAG = "DISPLAY";

u8g2_t u8g2;
static u8g2_esp32_hal_t hal = U8G2_ESP32_HAL_DEFAULT;
static bool display_initialized = false;
const char *menu_options[MENU_OPTIONS] = {"INICIAR", "CONFIGURAR"};
const char *usb_modes[3] = {"(C/A)", "(A/A)", "(C/C)"};
static char str_buffer[32];

void init_display()
{
    hal.bus.spi.clk = SCLK_PIN;
    hal.bus.spi.mosi = SID_PIN;
    hal.bus.spi.cs = CS_PIN;

    u8g2_esp32_hal_init(hal);

    // Setup do display ST7920 128x64 SPI
    u8g2_Setup_st7920_s_128x64_f(
        &u8g2,
        U8G2_R0,
        u8g2_esp32_spi_byte_cb,
        u8g2_esp32_gpio_and_delay_cb);

    u8g2_InitDisplay(&u8g2);
    u8g2_SetPowerSave(&u8g2, 0);
    u8g2_SetFont(&u8g2, u8g2_font_5x8_mf);

    display_initialized = true;

    ESP_LOGI(TAG, "Display ST7920_128x64 -> Inicializado.");
}

void draw_menu(int selected_option)
{
    u8g2_SetFont(&u8g2, u8g2_font_5x8_mf);
    u8g2_ClearBuffer(&u8g2);

    u8g2_DrawFrame(&u8g2, 0, 0, D_WIDTH, D_HEIGHT);
    u8g2_DrawFrame(&u8g2, 35, 2, D_WIDTH - 37, D_HEIGHT - 4);

    u8g2_DrawBitmap(&u8g2, 2, 1, 4, 16, logo_bitmap);
    u8g2_DrawStr(&u8g2, 6, 23, "CEBRA");
    u8g2_SetFont(&u8g2, u8g2_font_micro_tr);
    u8g2_DrawStr(&u8g2, 2, 62, "v1.1");

    u8g2_DrawBitmap(&u8g2, 37, 3 + (10 * selected_option), 1, 8, arrow);
    u8g2_SetFont(&u8g2, u8g2_font_6x10_tf);
    for (int i = 0; i < MENU_OPTIONS; i++)
    {
        u8g2_DrawStr(&u8g2, 46, 11 + (10 * i), menu_options[i]);
    }

    u8g2_SendBuffer(&u8g2);
}

static int center_text(int cx, int cw, const char *text)
{
    int tw = u8g2_GetStrWidth(&u8g2, text);
    int x = (cw - tw) / 2;
    return x + cx;
}

static void draw_settings_menu_item(int cx, int cw, int text_y, const char *text, bool selected)
{
    int font_height = u8g2_GetAscent(&u8g2) - u8g2_GetDescent(&u8g2);
    int x = center_text(cx, cw, text);

    if (selected)
    {
        // Fundo preto
        u8g2_SetDrawColor(&u8g2, 1);
        u8g2_DrawBox(&u8g2, cx, text_y - font_height, cw, font_height + 2);

        // Texto branco
        u8g2_SetDrawColor(&u8g2, 0);
        u8g2_DrawStr(&u8g2, x, text_y, text);

        u8g2_SetDrawColor(&u8g2, 1);
    }
    else
    {
        u8g2_DrawStr(&u8g2, x, text_y, text);
    }
}

void draw_settings(uint8_t usb_mode, int selected_option, int selected_channel, adc_result_t *adc_data, int selected_input)
{
    adc_channel_config_t *ch = adc_channels[selected_channel];

    u8g2_ClearBuffer(&u8g2);
    u8g2_SetFont(&u8g2, u8g2_font_5x8_tf);

    u8g2_DrawFrame(&u8g2, 0, 0, D_WIDTH, D_HEIGHT);
    u8g2_DrawHLine(&u8g2, 1, 9, D_WIDTH - 2);

    draw_settings_menu_item(1, 42, 8, "OP" CHAR_C_CEDILHA CHAR_O_TIL "ES", false);
    u8g2_DrawVLine(&u8g2, 42, 1, 62);
    draw_settings_menu_item(43, 42, 8, "VOLTAR", selected_option == 0);
    u8g2_DrawVLine(&u8g2, 84, 1, 8);
    draw_settings_menu_item(85, 42, 8, "SALVAR", selected_option == 1);
    //
    draw_settings_menu_item(1, 42, 17, "TIPO USB", selected_option == 2);
    u8g2_DrawHLine(&u8g2, 1, 18, 41);
    draw_settings_menu_item(1, 42, 26, "CANAIS", selected_option == 3);
    u8g2_DrawHLine(&u8g2, 1, 27, 41);
    draw_settings_menu_item(1, 42, 35, "ENTRADAS", selected_option == 4);
    u8g2_DrawHLine(&u8g2, 1, 36, 41);

    if (selected_option == 2)
    {
        u8g2_SetFont(&u8g2, u8g2_font_6x10_tf);
        snprintf(str_buffer, sizeof(str_buffer), "TIPO %s", usb_modes[usb_mode]);
        u8g2_DrawStr(&u8g2, center_text(43, 84, str_buffer), 22, str_buffer);
        switch (usb_mode)
        {
        case 0: // C/A
            u8g2_DrawBitmap(&u8g2, 49, 38, 3, 8, usb_c_bitmap);
            u8g2_DrawBitmap(&u8g2, 89, 30, 4, 16, usb_a_bitmap);
            break;
        case 1: // A/A
            u8g2_DrawBitmap(&u8g2, 49, 30, 4, 16, usb_a_bitmap);
            u8g2_DrawBitmap(&u8g2, 89, 30, 4, 16, usb_a_bitmap);
            break;
        case 2: // C/C
            u8g2_DrawBitmap(&u8g2, 49, 38, 3, 8, usb_c_bitmap);
            u8g2_DrawBitmap(&u8g2, 97, 38, 3, 8, usb_c_bitmap);
            break;
        }
        u8g2_DrawHLine(&u8g2, 45, 47, 80);
        u8g2_DrawHLine(&u8g2, 45, 48, 80);
    }

    if (selected_option == 3)
    {
        u8g2_DrawHLine(&u8g2, 43, 18, 84);
        snprintf(str_buffer, sizeof(str_buffer), "ID:%s", ch->name);
        u8g2_DrawStr(&u8g2, 43, 26, str_buffer);
        snprintf(str_buffer, sizeof(str_buffer), "GPIO:%d", ch->gpio_pin);
        u8g2_DrawStr(&u8g2, 86, 26, str_buffer);
        u8g2_DrawVLine(&u8g2, 84, 19, 8);

        u8g2_DrawHLine(&u8g2, 43, 27, 84);
        u8g2_DrawStr(&u8g2, center_text(43, 84, "LEITURAS"), 35, "LEITURAS");
        u8g2_DrawHLine(&u8g2, 43, 36, 84);
        u8g2_DrawStr(&u8g2, center_text(43, 42, "ADC/mV"), 44, "ADC/mV");
        u8g2_DrawStr(&u8g2, center_text(86, 42, "VALOR"), 44, "VALOR");
        u8g2_DrawVLine(&u8g2, 84, 37, 8);
        u8g2_DrawHLine(&u8g2, 43, 45, 84);

        u8g2_SetFont(&u8g2, u8g2_font_6x10_tf);
        snprintf(str_buffer, sizeof(str_buffer), "%d", adc_data->value_mv); // Valor em mV
        u8g2_DrawStr(&u8g2, center_text(43, 42, str_buffer), 58, str_buffer);

        snprintf(str_buffer, sizeof(str_buffer), "%.2f", adc_data->converted_value); // Valor convertido
        u8g2_DrawStr(&u8g2, center_text(85, 42, str_buffer), 58, str_buffer);
        u8g2_DrawVLine(&u8g2, 84, 46, 17);
    }
    if (selected_option == 4)
    {
        u8g2_DrawFrame(&u8g2, 46, 13, 12, 30);
        switch (selected_input)
        {
        case 1:
            u8g2_DrawBox(&u8g2, 48, 24, 8, 8);
            u8g2_DrawStr(&u8g2, 60, 31, "ENTRADA 1");
            break;
        case 2:
            u8g2_DrawBox(&u8g2, 48, 33, 8, 8);
            u8g2_DrawStr(&u8g2, 60, 40, "ENTRADA 2");
            break;

        default:
            u8g2_DrawBox(&u8g2, 48, 15, 8, 8);
            u8g2_DrawStr(&u8g2, 60, 22, "DESLIGADO");

            break;
        }
    }

    u8g2_SendBuffer(&u8g2);
}

char *str_ca = "CONECTOR A";
char *str_cb = "CONECTOR B";
void draw_check_connectors_test(const char *title, int usb_types[2])
{
    char ca_type_buffer[8], cb_type_buffer[8];
    char types[4] = {"AC?"};

    snprintf(ca_type_buffer, sizeof(ca_type_buffer), "TIPO %c", types[usb_types[0]]);
    snprintf(cb_type_buffer, sizeof(cb_type_buffer), "TIPO %c", types[usb_types[1]]);

    u8g2_ClearBuffer(&u8g2);
    u8g2_SetFont(&u8g2, u8g2_font_5x8_tf);

    u8g2_DrawFrame(&u8g2, 0, 0, D_WIDTH, D_HEIGHT);
    u8g2_DrawStr(&u8g2, center_text(1, 126, title), 8, title);
    u8g2_DrawHLine(&u8g2, 1, 9, D_WIDTH - 2);

    u8g2_DrawStr(&u8g2, center_text(1, 62, str_ca), 26, str_ca);
    u8g2_DrawStr(&u8g2, center_text(1, 62, ca_type_buffer), 37, ca_type_buffer);
    u8g2_DrawStr(&u8g2, center_text(65, 62, str_cb), 26, str_cb);
    u8g2_DrawStr(&u8g2, center_text(65, 62, cb_type_buffer), 37, cb_type_buffer);
    u8g2_SendBuffer(&u8g2);
}

void draw_test_fail_page(const char *step_title, const char *error_message)
{
    char *title = "TESTE REPROVADO";

    u8g2_ClearBuffer(&u8g2);
    u8g2_SetFont(&u8g2, u8g2_font_5x8_tf);

    u8g2_DrawFrame(&u8g2, 0, 0, D_WIDTH, D_HEIGHT);
    u8g2_DrawStr(&u8g2, center_text(1, 126, title), 8, title);
    u8g2_DrawHLine(&u8g2, 1, 9, D_WIDTH - 2);

    u8g2_DrawStr(&u8g2, 2, 17, "ETAPA:");
    u8g2_DrawStr(&u8g2, 2, 26, step_title);
    u8g2_DrawStr(&u8g2, 2, 39, "ERRO:");
    u8g2_DrawStr(&u8g2, 2, 48, error_message);

    u8g2_DrawHLine(&u8g2, 1, 54, D_WIDTH - 2);
    u8g2_DrawStr(&u8g2, 2, 62, "B)VOLTAR");
    
    u8g2_SendBuffer(&u8g2);
}