#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"

#include <adc_utils.h>
#include <display.h>
#include <state.h>
#include <types.h>

#define BTN_A_MOVE 15
#define BTN_B_SELECT 2
#define RL_CARGAS 4
#define OPTO_SRC_1 21
#define OPTO_SRC_2 19
#define STORAGE_NAMESPACE "storage"

//static const char *TAG = "MAIN";

// Configuração dos 12 canais ADC
static adc_channel_config_t channel_configs[12] = {
    {ADC_UNIT_1, ADC_CHANNEL_0, ADC_ATTEN_DB_12, {0}, "A_CC1", 36, 1},
    {ADC_UNIT_1, ADC_CHANNEL_3, ADC_ATTEN_DB_12, {0}, "A_CC2", 39, 1},
    {ADC_UNIT_1, ADC_CHANNEL_4, ADC_ATTEN_DB_12, {0}, "A_DN", 32, 0},
    {ADC_UNIT_1, ADC_CHANNEL_5, ADC_ATTEN_DB_12, {0}, "A_DP", 33, 0},
    {ADC_UNIT_1, ADC_CHANNEL_6, ADC_ATTEN_DB_12, {0}, "A_VCC", 34, 1},
    {ADC_UNIT_1, ADC_CHANNEL_7, ADC_ATTEN_DB_12, {0}, "B_CC1", 35, 1},
    {ADC_UNIT_2, ADC_CHANNEL_8, ADC_ATTEN_DB_12, {0}, "B_CC2", 25, 1},
    {ADC_UNIT_2, ADC_CHANNEL_9, ADC_ATTEN_DB_12, {0}, "B_DN", 26, 0},
    {ADC_UNIT_2, ADC_CHANNEL_7, ADC_ATTEN_DB_12, {0}, "B_DP", 27, 0},
    {ADC_UNIT_2, ADC_CHANNEL_6, ADC_ATTEN_DB_12, {0}, "B_VCC", 14, 1},
    {ADC_UNIT_2, ADC_CHANNEL_5, ADC_ATTEN_DB_12, {0}, "CA", 12, 0},
    {ADC_UNIT_2, ADC_CHANNEL_4, ADC_ATTEN_DB_12, {0}, "CB", 13, 0},
};

adc_channel_config_t *adc_channels[12] = {
    &channel_configs[0],
    &channel_configs[1],
    &channel_configs[2],
    &channel_configs[3],
    &channel_configs[4],
    &channel_configs[5],
    &channel_configs[6],
    &channel_configs[7],
    &channel_configs[8],
    &channel_configs[9],
    &channel_configs[10],
    &channel_configs[11],
};

// Filas
QueueHandle_t adc_request_queue;
QueueHandle_t adc_response_queue;

void setup();

void main_task(void *pvParameters);
void adc_task(void *pvParameters);

void state_machine();

bool button_pressed(gpio_num_t pin);
uint8_t load_usb_mode();
void save_usb_mode(uint8_t usb_mode);

uint8_t usb_mode;

void app_main()
{
    setup();

    adc_request_queue = xQueueCreate(5, sizeof(adc_request_t));
    adc_response_queue = xQueueCreate(5, sizeof(adc_response_t));

    xTaskCreate(&main_task, "main_task", 4096, NULL, 6, NULL);
    xTaskCreate(&adc_task, "adc_task", 4096, NULL, 5, NULL);
}

void setup()
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    }

    ESP_ERROR_CHECK(adc_init());
    init_display();
    usb_mode = load_usb_mode();

    gpio_set_direction(BTN_A_MOVE, GPIO_MODE_INPUT);
    gpio_set_direction(BTN_B_SELECT, GPIO_MODE_INPUT);
    gpio_set_pull_mode(BTN_B_SELECT, GPIO_PULLUP_ONLY);
    gpio_set_direction(RL_CARGAS, GPIO_MODE_OUTPUT);
    gpio_set_direction(OPTO_SRC_1, GPIO_MODE_OUTPUT);
    gpio_set_direction(OPTO_SRC_2, GPIO_MODE_OUTPUT);
}

void adc_task(void *pvParameters)
{
    adc_request_t request;
    adc_response_t response;

    while (1)
    {
        if (xQueueReceive(adc_request_queue, &request, portMAX_DELAY))
        {
            int count = request.num_channels;
            response.num_values = count;

            for (int i = 0; i < count; i++)
            {
                read_channel(request.channels[i], &response.values[i]);
                float converted = response.values[i] > 0 ? convert_reading(response.values[i]) : 0.0;
                if (request.channels[i] == A_VCC || request.channels[i] == B_VCC)
                {
                    response.converted_values[i] = converted * 2;
                }
                else
                {
                    response.converted_values[i] = converted;
                }
            }

            xQueueSend(adc_response_queue, &response, portMAX_DELAY);
        }
    }
}

void main_task(void *pvParameters)
{
    while (1)
    {
        state_machine();
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void state_machine()
{
    static system_state_t current_state = STATE_MENU;
    static int main_menu_option = 0;
    static int settings_menu_option = 0;
    static int settings_selected_channel = -1;
    static int settings_selected_input = 0;

    switch (current_state)
    {
    case STATE_MENU:
        draw_menu(main_menu_option);
        if (button_pressed(BTN_A_MOVE))
        {
            if (main_menu_option == (MENU_OPTIONS - 1))
            {
                main_menu_option = 0;
            }
            else
            {
                main_menu_option++;
            }
        }
        if (button_pressed(BTN_B_SELECT))
        {
            switch (main_menu_option)
            {
            case 0:
                break;
            case 1:
                current_state = STATE_SETTINGS;
                break;

            default:
                break;
            }
        }
        break;

    case STATE_SETTINGS:
        adc_response_t response;

        if (settings_selected_channel >= 0)
        {
            adc_request_t request = {.num_channels = 1};
            request.channels[0] = settings_selected_channel;
            xQueueSend(adc_request_queue, &request, portMAX_DELAY);
        }

        xQueueReceive(adc_response_queue, &response, 0);
        draw_settings(usb_mode, settings_menu_option, settings_selected_channel, &response, settings_selected_input);

        if (button_pressed(BTN_A_MOVE))
        {
            if (settings_menu_option == 2)
            {
                settings_selected_channel = 0;
            }
            if (settings_menu_option == 3)
            {
                settings_selected_channel = -1;
            }
            if (settings_menu_option == 4) // 5 Opções
            {
                settings_menu_option = 0;
            }
            else
            {
                settings_menu_option++;
            }
        }
        if (button_pressed(BTN_B_SELECT))
        {
            switch (settings_menu_option)
            {
            case 0:
                usb_mode = load_usb_mode();
                current_state = STATE_MENU;
                break;
            case 1:
                save_usb_mode(usb_mode);
                current_state = STATE_MENU;
                break;
            case 2:
                if (usb_mode == 2)
                {
                    usb_mode = 0;
                }
                else
                {
                    usb_mode++;
                }
                break;

            case 3:

                if (settings_selected_channel == 9)
                {
                    gpio_set_level(RL_CARGAS, 1);
                }

                if (settings_selected_channel == 11)
                {
                    gpio_set_level(RL_CARGAS, 0);
                    settings_selected_channel = 0;
                }
                else
                {
                    settings_selected_channel++;
                }

                break;
            case 4:
                if (settings_selected_input == 2)
                {
                    settings_selected_input = 0;
                }
                else
                {
                    settings_selected_input++;
                }
                gpio_set_level(OPTO_SRC_1, settings_selected_input == 1);
                gpio_set_level(OPTO_SRC_2, settings_selected_input == 2);

                break;
            }
        }
        break;
    default:
        break;
    }
}

bool button_pressed(gpio_num_t pin)
{
    static TickType_t last_press_time[2] = {0};
    static int last_state[2] = {1};

    int index = (pin == BTN_B_SELECT) ? 0 : 1;

    int current = gpio_get_level(pin);
    TickType_t now = xTaskGetTickCount();

    if (current == 0 && last_state[index] == 1)
    {
        if ((now - last_press_time[index]) > pdMS_TO_TICKS(200))
        {
            last_press_time[index] = now;
            last_state[index] = 0;
            return true;
        }
    }

    last_state[index] = current;
    return false;
}

void save_usb_mode(uint8_t usb_mode)
{
    nvs_handle_t handle;
    nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &handle);
    nvs_set_u8(handle, "usb_mode", usb_mode);
    nvs_commit(handle);
    nvs_close(handle);
}

uint8_t load_usb_mode()
{
    nvs_handle_t handle;
    uint8_t usb_mode = 0;
    nvs_open(STORAGE_NAMESPACE, NVS_READONLY, &handle);
    nvs_get_u8(handle, "usb_mode", &usb_mode);
    nvs_close(handle);
    return usb_mode;
}
