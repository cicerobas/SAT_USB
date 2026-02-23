#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "nvs.h"

#include <adc_utils.h>
#include <display.h>
#include <state.h>
#include <types.h>

#define BTN_A_MOVE 15
#define BTN_B_SELECT 2
#define RL_CARGAS 4
#define RL_CURTO_A 17
#define RL_CURTO_B 16
#define OPTO_SRC_1 21
#define OPTO_SRC_2 19
#define STORAGE_NAMESPACE "storage"

// static const char *TAG = "MAIN";

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

// Limites superior e inferior de cada pino
pin_info_t pins_info[10] = {
    {A_CC1, 5.30, 4.9, true},
    {A_CC2, 5.30, 4.9, true},
    {A_DN, 2.9, 2.5, true},
    {A_DP, 2.2, 1.8, true},
    {A_VCC, 5.30, 4.9, true},
    {B_CC1, 5.30, 4.9, true},
    {B_CC2, 5.30, 4.9, true},
    {B_DN, 2.9, 2.5, true},
    {B_DP, 2.2, 1.8, true},
    {B_VCC, 5.30, 4.9, true},
};

void setup();

void main_task(void *pvParameters);

void state_machine();
bool button_pressed(gpio_num_t pin);
bool check_timer_delay(int delay_ms);
uint8_t load_usb_mode();
void save_usb_mode(uint8_t usb_mode);

step_status_t test_check_connectors();
step_status_t test_data_pins(float *values);
step_status_t test_vcc_load(float *values, bool load_on);
step_status_t test_auto_short(float *values, int *status);
pin_result_t check_pin_value(float value, pin_info_t pin_info);

uint8_t usb_mode;
int usb_types[2];
int64_t step_start_time_us = 0;
step_status_t step_result;

void app_main()
{
    setup();

    xTaskCreate(&main_task, "main_task", 4096, NULL, 6, NULL);
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
    gpio_set_pull_mode(BTN_B_SELECT, GPIO_PULLUP_ONLY); // Configurado como PULLUP_ONLY para evitar conflito com as funções secundárias do GPIO2
    gpio_set_direction(RL_CARGAS, GPIO_MODE_OUTPUT);
    gpio_set_direction(RL_CURTO_A, GPIO_MODE_OUTPUT);
    gpio_set_direction(RL_CURTO_B, GPIO_MODE_OUTPUT);
    gpio_set_direction(OPTO_SRC_1, GPIO_MODE_OUTPUT);
    gpio_set_direction(OPTO_SRC_2, GPIO_MODE_OUTPUT);
}

void main_task(void *pvParameters)
{
    while (1)
    {
        state_machine();
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

// Define a sequência de etapas no teste
const test_step_info_t test_sequence[] = {
    {CHANGE_INPUT_SOURCE, ""},
    {CHECK_CONNECTORS, "VERIFICAR CONECTORES"},
    {DATA_PINS, "PINOS DE DADOS"},
    {MINIMUM_LOAD, "CARGA MINIMA|E1"},
    {MAXIMUM_LOAD, "CARGA MAXIMA|E1"},
    {CHANGE_INPUT_SOURCE, ""},
    {MINIMUM_LOAD, "CARGA MINIMA|E2"},
    {MAXIMUM_LOAD, "CARGA MAXIMA|E2"},
    {AUTOMATIC_SHORT, "CURTO AUTOMATICO"},
};

void state_machine()
{
    static system_state_t current_state = STATE_MENU;
    static int main_menu_option = 0;
    static int settings_menu_option = 0;
    static int settings_selected_channel = -1;
    static int settings_selected_input = 0;

    static int current_step_index = 0;
    static int current_input_source = 0;

    switch (current_state)
    {
    case STATE_MENU:
        current_step_index = 0;
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
                current_state = STATE_TEST_RUNNING;
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
        adc_result_t adc_data = {-1, 0, 0.0};

        if (settings_selected_channel >= 0)
        {
            adc_data.channel = settings_selected_channel;
            read_channel(&adc_data);
        }

        draw_settings(usb_mode, settings_menu_option, settings_selected_channel, &adc_data, settings_selected_input);

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

    case STATE_TEST_RUNNING:
        static bool executed = false;

        step_result.index = current_step_index;
        switch (test_sequence[current_step_index].type)
        {
        case CHANGE_INPUT_SOURCE:
            current_input_source++;
            gpio_set_level(OPTO_SRC_1, current_input_source == 1);
            gpio_set_level(OPTO_SRC_2, current_input_source == 2);

            vTaskDelay(pdMS_TO_TICKS(100));
            current_step_index++;
            break;

        case CHECK_CONNECTORS:
            if (!executed)
            {
                usb_types[0] = 2;
                usb_types[1] = 2;
                step_start_time_us = esp_timer_get_time();

                draw_check_connectors_test(test_sequence[current_step_index].step_title, usb_types);
                step_result = test_check_connectors();
                draw_check_connectors_test(test_sequence[current_step_index].step_title, usb_types);

                executed = true;
            }
            else if (check_timer_delay(2000))
            {
                executed = false;
                if (step_result.status)
                {
                    step_start_time_us = esp_timer_get_time();
                    current_step_index++;
                }
                else
                {
                    current_state = STATE_TEST_FAIL;
                }
            }

            break;
        case DATA_PINS:
            if (!executed)
            {
                float values[10];
                step_result = test_data_pins(values);
                draw_data_pins_test(test_sequence[current_step_index].step_title, usb_types, values);
                executed = !step_result.status || check_timer_delay(3000);
            }
            else
            {
                executed = false;
                if (step_result.status)
                {
                    step_start_time_us = esp_timer_get_time();
                    current_step_index++;
                }
                else
                {
                    current_state = STATE_TEST_FAIL;
                }
            }

            break;
        case MINIMUM_LOAD:
        case MAXIMUM_LOAD:
            bool load_on = test_sequence[current_step_index].type == MAXIMUM_LOAD;
            static bool rl_on = false;
            if (!executed)
            {
                float values[4];
                if (!rl_on && load_on)
                {
                    gpio_set_level(RL_CARGAS, 1);
                    rl_on = true;
                    vTaskDelay(pdMS_TO_TICKS(100));
                }

                step_result = test_vcc_load(values, load_on);
                draw_vcc_load_test(test_sequence[current_step_index].step_title, values);

                executed = check_timer_delay(4000);
            }
            else
            {
                executed = false;
                if (rl_on)
                {
                    gpio_set_level(RL_CARGAS, 0);
                    rl_on = false;
                    vTaskDelay(pdMS_TO_TICKS(100));
                }
                if (step_result.status)
                {
                    step_start_time_us = esp_timer_get_time();
                    current_step_index++;
                }
                else
                {
                    current_state = STATE_TEST_FAIL;
                }
            }
            break;
        case AUTOMATIC_SHORT:
            if (!executed)
            {
                float values[2];
                static int status[4] = {0, 0, 0, 0};
                static bool success_detected = false;
                static int64_t success_time = 0;
                adc_result_t adc_data;

                for (int i = 0; i < 2; i++)
                {
                    adc_data.channel = i == 0 ? A_VCC : B_VCC;
                    read_channel(&adc_data);
                    values[i] = adc_data.converted_value;
                }
                draw_auto_short_test(test_sequence[current_step_index].step_title, values, status);

                if (!success_detected)
                {
                    step_result = test_auto_short(values, status);
                }

                if (step_result.status && !success_detected)
                {
                    success_detected = true;
                    success_time = esp_timer_get_time();
                }

                bool timeout = check_timer_delay(10000);
                bool success_delay_done = success_detected && ((esp_timer_get_time() - success_time) >= 1500000);

                executed = success_delay_done || timeout;

                if (executed)
                {
                    memset(status, 0, sizeof(status));
                    success_detected = false;
                    success_time = 0;
                }
            }
            else
            {
                executed = false;
                if (step_result.status)
                {
                    step_start_time_us = esp_timer_get_time();
                    current_step_index++;
                }
                else
                {
                    strcpy(step_result.message, "FALHA EM SHUT/REC");
                    current_state = STATE_TEST_FAIL;
                }
            }
            break;
        default:
            memset(&step_result, 0, sizeof(step_status_t));
            current_input_source = 0;
            current_state = STATE_MENU;
            gpio_set_level(OPTO_SRC_1, 0);
            gpio_set_level(OPTO_SRC_2, 0);
            current_state = STATE_TEST_PASS;
            break;
        }
        break;

    case STATE_TEST_FAIL:
        draw_test_fail_page(test_sequence[step_result.index].step_title, step_result.message);
        current_input_source = 0;
        gpio_set_level(OPTO_SRC_1, 0);
        gpio_set_level(OPTO_SRC_2, 0);
        if (button_pressed(BTN_B_SELECT))
        {
            current_state = STATE_MENU;
        }
        break;

    case STATE_TEST_PASS:
        draw_test_pass_page();
        if (button_pressed(BTN_B_SELECT))
        {
            current_state = STATE_MENU;
        }
        if (button_pressed(BTN_A_MOVE))
        {
            current_step_index = 0;
            current_state = STATE_TEST_RUNNING;
        }
        break;

    default:
        break;
    }
}

// Verifica se o botão foi pressionado com um pequeno debounce
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

// Usa o timer interno do ESP para verificar se um tempo (delay_ms) passou desde (step_start_time_us)
bool check_timer_delay(int delay_ms)
{
    int64_t current_time_us = esp_timer_get_time();
    int elapsed_ms = (current_time_us - step_start_time_us) / 1000;
    return elapsed_ms >= delay_ms;
}

// Salva o tipo de conector (CA/AA/CC) em NVS
void save_usb_mode(uint8_t usb_mode)
{
    nvs_handle_t handle;
    nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &handle);
    nvs_set_u8(handle, "usb_mode", usb_mode);
    nvs_commit(handle);
    nvs_close(handle);
}

// Carrega o tipo de conector salvo em NVS ou 0 por padrão
uint8_t load_usb_mode()
{
    nvs_handle_t handle;
    uint8_t usb_mode = 0;
    nvs_open(STORAGE_NAMESPACE, NVS_READONLY, &handle);
    nvs_get_u8(handle, "usb_mode", &usb_mode);
    nvs_close(handle);
    return usb_mode;
}

step_status_t test_check_connectors()
{
    const int usb_cc_pins[4] = {A_CC1, A_CC2, B_CC1, B_CC2};
    int cc_values[4] = {0, 0, 0, 0};
    step_status_t test_result;
    adc_result_t adc_data;

    // Verifica se a fonte no conector A tem VCC
    adc_data.channel = A_VCC;
    read_channel(&adc_data);
    if (adc_data.converted_value < pins_info[A_VCC].low_limit)
    {
        test_result.status = false;
        strcpy(test_result.message, "CON_A:VCC BAIXO");
        return test_result;
    }

    memset(&adc_data, 0, sizeof(adc_result_t));

    // Verifica se a fonte no conector B tem VCC
    adc_data.channel = B_VCC;
    read_channel(&adc_data);
    if (adc_data.converted_value < pins_info[B_VCC].low_limit)
    {
        test_result.status = false;
        strcpy(test_result.message, "CON_B:VCC BAIXO");
        return test_result;
    }

    memset(&adc_data, 0, sizeof(adc_result_t));

    for (int i = 0; i < 4; i++)
    {
        adc_data.channel = usb_cc_pins[i];
        read_channel(&adc_data);
        cc_values[i] = adc_data.value_mv;
    }

    // Verifica se os pinos CC1 e CC2 não estão em ALTO simultaneamente
    // 0=A 1=C 2=Erro
    usb_types[0] = (cc_values[0] != 0) + (cc_values[1] != 0);
    usb_types[1] = (cc_values[2] != 0) + (cc_values[3] != 0);
    if (usb_types[0] == 2)
    {
        test_result.status = false;
        strcpy(test_result.message, "CON_A:CC1/CC2 ALTOS");
        return test_result;
    }
    if (usb_types[1] == 2)
    {
        test_result.status = false;
        strcpy(test_result.message, "CON_B:CC1/CC2 ALTOS");
        return test_result;
    }

    // Verifica se os pinos CC1 e CC2 estão de acordo com o tipo de conector configurado
    // 0=C/A 1=A/A 2=C/C
    switch (usb_mode)
    {
    case 0:
        test_result.status = usb_types[0] != usb_types[1];
        strcpy(test_result.message, test_result.status ? "OK" : "USB_MODE C/A: INVALIDO");
        break;
    case 1:
        test_result.status = (usb_types[0] + usb_types[1]) == 0;
        strcpy(test_result.message, test_result.status ? "OK" : "USB_MODE A/A: INVALIDO");
        break;
    case 2:
        test_result.status = (usb_types[0] + usb_types[1]) == 2;
        strcpy(test_result.message, test_result.status ? "OK" : "USB_MODE C/C: INVALIDO");
        break;
    }

    return test_result;
}

step_status_t test_data_pins(float *values)
{
    step_status_t test_result;
    adc_result_t adc_data;
    char str_buffer[32];

    pins_info[0].required = usb_types[0];
    pins_info[1].required = usb_types[0];
    pins_info[5].required = usb_types[1];
    pins_info[6].required = usb_types[1];
    pins_info[4].required = false;
    pins_info[9].required = false;

    for (int i = 0; i < 10; i++)
    {
        if (pins_info[i].required)
        {
            adc_data.channel = pins_info[i].pin;
            read_channel(&adc_data);
            values[i] = adc_data.converted_value;
        }
        else
        {
            values[i] = 0.0;
        }
        memset(&adc_data, 0, sizeof(adc_result_t));
    }

    // Verifica os valores dos pinos de acordo com o tipo de conector
    for (int i = 0; i < 10; i++)
    {
        if (pins_info[i].required)
        {
            switch (i)
            {
            case 0:
            case 1:
                if (check_pin_value(values[0], pins_info[0]) == check_pin_value(values[1], pins_info[1]))
                {
                    test_result.status = false;
                    strcpy(test_result.message, "CON_A: PINOS CC1/CC2");
                    return test_result;
                }
                break;
            case 5:
            case 6:
                if (check_pin_value(values[5], pins_info[5]) == check_pin_value(values[6], pins_info[6]))
                {
                    test_result.status = false;
                    strcpy(test_result.message, "CON_B: PINOS CC1/CC2");
                    return test_result;
                }
                break;

            default:
                pin_result_t pin_result = check_pin_value(values[i], pins_info[i]);
                switch (pin_result)
                {
                case VALUE_BELOW:
                    snprintf(str_buffer, sizeof(str_buffer), "PINO: %s BAIXO", adc_channels[i]->name);
                    test_result.status = false;
                    strcpy(test_result.message, str_buffer);
                    return test_result;
                    break;
                case VALUE_ABOVE:
                    snprintf(str_buffer, sizeof(str_buffer), "PINO: %s ALTO", adc_channels[i]->name);
                    test_result.status = false;
                    strcpy(test_result.message, str_buffer);
                    return test_result;
                    break;
                default:
                    break;
                }

                break;
            }
        }
    }

    test_result.status = true;
    strcpy(test_result.message, "OK");
    return test_result;
}

step_status_t test_vcc_load(float *values, bool load_on)
{
    const int pins[4] = {A_VCC, CA, B_VCC, CB};
    step_status_t test_result;
    pin_result_t con_a, con_b;
    adc_result_t adc_data;
    char str_buffer[32];

    for (int i = 0; i < 4; i++)
    {
        adc_data.channel = pins[i];
        read_channel(&adc_data);
        vTaskDelay(pdMS_TO_TICKS(1));
        if (i == 1 || i == 3)
        {
            values[i] = load_on ? adc_data.converted_value : 0.0;
        }
        else
        {
            values[i] = adc_data.converted_value;
        }
    }

    con_a = check_pin_value(values[0], pins_info[4]);
    con_b = check_pin_value(values[2], pins_info[9]);

    if (con_a != VALUE_OK)
    {
        test_result.status = false;
        snprintf(str_buffer, sizeof(str_buffer), "CON_A: VCC %s", con_a == VALUE_BELOW ? "BAIXO" : "ALTO");
        strcpy(test_result.message, str_buffer);
        return test_result;
    }
    if (con_b != VALUE_OK)
    {
        test_result.status = false;
        snprintf(str_buffer, sizeof(str_buffer), "CON_B: VCC %s", con_b == VALUE_BELOW ? "BAIXO" : "ALTO");
        strcpy(test_result.message, str_buffer);
        return test_result;
    }
    test_result.status = true;
    strcpy(test_result.message, "OK");
    return test_result;
}

step_status_t test_auto_short(float *values, int *status)
{
    step_status_t test_result;

    static bool ca_done = false;
    static bool cb_done = false;
    static bool short_ca_done = false;
    static bool short_cb_done = false;
    static bool rl_ca_on = false;
    static bool rl_cb_on = false;

    vTaskDelay(pdMS_TO_TICKS(300));

    // Aciona o curto no conector A e verifica shutdown & recovery
    if (!ca_done)
    {
        if (values[0] >= pins_info[4].low_limit && !short_ca_done)
        {
            gpio_set_level(RL_CURTO_A, 1);
            rl_ca_on = true;
        }
        else
        {
            if (rl_ca_on)
            {
                vTaskDelay(pdMS_TO_TICKS(300));
                gpio_set_level(RL_CURTO_A, 0);
                rl_ca_on = false;
                short_ca_done = true;
                status[0] = 1;
            }
            else
            {
                ca_done = values[0] >= pins_info[4].low_limit;
            }
        }
    }
    else
    {
        status[1] = 1;
    }

    // Aciona o curto no conector B e verifica shutdown & recovery
    if (!cb_done)
    {
        if (values[1] >= pins_info[9].low_limit && !short_cb_done)
        {
            gpio_set_level(RL_CURTO_B, 1);
            rl_cb_on = true;
        }
        else
        {
            if (rl_cb_on)
            {
                vTaskDelay(pdMS_TO_TICKS(300));
                gpio_set_level(RL_CURTO_B, 0);
                rl_cb_on = false;
                short_cb_done = true;
                status[2] = 1;
            }
            else
            {
                cb_done = values[1] >= pins_info[9].low_limit;
            }
        }
    }
    else
    {
        status[3] = 1;
    }

    // Retorna (true) caso os dois status de recovery sejam (true)
    if (status[1] && status[3])
    {
        ca_done = false;
        cb_done = false;
        short_ca_done = false;
        short_cb_done = false;
        test_result.status = true;
        return test_result;
    }
    else
    {
        test_result.status = false;
        return test_result;
    }
}

pin_result_t check_pin_value(float value, pin_info_t pin_info)
{
    return (value >= pin_info.low_limit) - (value <= pin_info.high_limit);
}
