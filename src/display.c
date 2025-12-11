#include "display.h"

#include <stdbool.h>

#include "u8g2_esp32_hal.h"
#include "esp_log.h"

#define SCLK_PIN 18
#define SID_PIN 23
#define CS_PIN 5

#define DISPLAY_W 128
#define DISPLAY_H 64

static const char *TAG = "DISPLAY";

u8g2_t u8g2;
static u8g2_esp32_hal_t hal = U8G2_ESP32_HAL_DEFAULT;
static bool display_initialized = false;

void init_display(void)
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

void draw_test_page(int *values)
{
}
