#pragma once

#include "quantum.h"
#include "he_analog.h"
#include "rapid_trigger.h"

#define LAYOUT( \
    k00, k01, k02  \
) { \
    { k00, k01, k02 } \
}

he_key_t* get_he_key(uint8_t key_index);
rt_config_t* get_rt_config(uint8_t key_index);
void he_keys_init(void);
void he_keys_process(void);
void he_keys_reset_calibration(void);
void rgb_multipin_init(void);
void rgb_multipin_set_color(uint8_t index, uint8_t r, uint8_t g, uint8_t b);
void rgb_multipin_set_color_all(uint8_t r, uint8_t g, uint8_t b);
void rgb_multipin_flush(void);
void hid_stream_adc_task(void);

#define HID_CMD_HANDSHAKE           0x01
#define HID_CMD_GET_STATUS          0x02
#define HID_CMD_GET_KEY_CONFIG      0x10
#define HID_CMD_SET_ACTUATION       0x11
#define HID_CMD_SET_RT_SENSITIVITY  0x12
#define HID_CMD_SET_RT_MODE         0x13
#define HID_CMD_GET_CALIBRATION     0x14
#define HID_CMD_SET_CALIBRATION     0x15
#define HID_CMD_SET_RGB_COLOR       0x20
#define HID_CMD_GET_RGB_COLOR       0x21
#define HID_CMD_SET_RGB_PARAMS      0x22
#define HID_CMD_GET_RGB_PARAMS      0x23
#define HID_CMD_KEYMAP_GET_KEYCODE  0x30
#define HID_CMD_KEYMAP_SET_KEYCODE  0x31
#define HID_CMD_KEYMAP_RESET        0x32
#define HID_CMD_KEYMAP_INFO         0x33
#define HID_CMD_MACRO_INFO          0x40
#define HID_CMD_MACRO_GET_BUFFER    0x41
#define HID_CMD_MACRO_SET_BUFFER    0x42
#define HID_CMD_MACRO_RESET         0x43
#define HID_CMD_MACRO_SEND          0x44
#define HID_CMD_STREAM_ADC          0x90
#define HID_CMD_CALIBRATE           0xA0
#define HID_CMD_SAVE_CONFIG         0xF0
#define HID_CMD_LOAD_CONFIG         0xF1

#define HID_RESP_OK                 0x00
#define HID_RESP_ERROR              0xFF
#define HID_RESP_HANDSHAKE          0x01
#define HID_RESP_STATUS             0x02
#define HID_RESP_KEY_CONFIG         0x10
#define HID_RESP_CALIBRATION        0x14
#define HID_RESP_RGB_COLOR          0x21
#define HID_RESP_RGB_PARAMS         0x23
#define HID_RESP_KEYMAP_KEYCODE     0x30
#define HID_RESP_KEYMAP_INFO        0x33
#define HID_RESP_MACRO_INFO         0x40
#define HID_RESP_MACRO_BUFFER       0x41
#define HID_RESP_ADC_DATA           0x90
