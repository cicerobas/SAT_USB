#ifndef DISPLAY_H
#define DISPLAY_H

#include "u8g2.h"
#include "adc_utils.h"
#include "types.h"

#define MENU_OPTIONS 2

void init_display();
void draw_menu(int selected_option);
void draw_settings(uint8_t usb_mode, int selected_option, int selected_channel, adc_result_t *adc_data, int selected_input);
void draw_check_connectors_test(const char *title, int usb_types[2]);

void draw_test_fail_page(const char *step_title, const char *error_message);

#endif
