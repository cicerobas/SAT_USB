#ifndef DISPLAY_H
#define DISPLAY_H

#include "u8g2.h"
#include "adc_utils.h"
#include "types.h"

#define MENU_OPTIONS 2

void init_display();
void draw_menu(int selected_option);
void draw_settings(uint8_t usb_mode, int selected_option, int selected_channel, adc_response_t *response_data, int selected_input);

#endif
