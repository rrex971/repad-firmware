#include "quantum.h"
#include "raw_hid.h"
#include "rgb_matrix.h"
#include "dynamic_keymap.h"
#include "repad.h"
#include "config.h"
#include "repad_config.h"

#ifndef RAW_EPSIZE
#define RAW_EPSIZE 32
#endif

static bool adc_streaming_enabled = false;
static uint16_t stream_interval_ms = 10;
static uint32_t last_stream_time = 0;
static bool hid_rgb_control = false;

static inline uint8_t scale_u8(uint8_t value, uint8_t brightness) {
    return (uint8_t)(((uint16_t)value * brightness) / 255);
}

static void send_ok(uint8_t cmd) {
    uint8_t response[RAW_EPSIZE] = {HID_RESP_OK, cmd, 0};
    raw_hid_send(response, RAW_EPSIZE);
}

static void send_error(uint8_t cmd, uint8_t error_code) {
    uint8_t response[RAW_EPSIZE] = {HID_RESP_ERROR, cmd, error_code, 0};
    raw_hid_send(response, RAW_EPSIZE);
}

static void handle_handshake(uint8_t *data) {
    (void)data;
    uint8_t response[RAW_EPSIZE] = {
        HID_RESP_HANDSHAKE,
        'r', 'e', 'P', 'a', 'd',
        0x01, 0x00,
        0x03,
        0x03,
        0
    };
    raw_hid_send(response, RAW_EPSIZE);
}

static void handle_get_status(uint8_t *data) {
    (void)data;
    uint8_t pressed = 0;
    uint8_t calibrated = 0;
    for (uint8_t i = 0; i < MATRIX_COLS; i++) {
        he_key_t *key = get_he_key(i);
        if (!key) continue;
        if (key->pressed) pressed |= (1 << i);
        if (key->calibrated) calibrated |= (1 << i);
    }
    repad_config_t *cfg = repad_config_get();
    uint8_t response[RAW_EPSIZE] = {0};
    response[0] = HID_RESP_STATUS;
    response[1] = 0x01;
    response[4] = MATRIX_COLS;
    response[5] = 3;
    response[6] = pressed;
    response[7] = calibrated;
    response[8] = cfg->rgb_mode;
    response[9] = cfg->rgb_brightness;
    response[10] = adc_streaming_enabled ? 1 : 0;
    response[11] = (uint8_t)dynamic_keymap_get_layer_count();
    raw_hid_send(response, RAW_EPSIZE);
}

static void handle_get_key_config(uint8_t *data) {
    uint8_t key_index = data[1];
    if (key_index >= MATRIX_COLS) {
        send_error(HID_CMD_GET_KEY_CONFIG, 0x01);
        return;
    }
    he_key_t* key = get_he_key(key_index);
    rt_config_t* config = get_rt_config(key_index);
    if (!key || !config) {
        send_error(HID_CMD_GET_KEY_CONFIG, 0x02);
        return;
    }
    uint8_t response[RAW_EPSIZE] = {
        HID_RESP_KEY_CONFIG,
        key_index,
        (uint8_t)config->mode,
        (config->actuation_point >> 8) & 0xFF,
        config->actuation_point & 0xFF,
        (config->release_point >> 8) & 0xFF,
        config->release_point & 0xFF,
        (config->rt_down >> 8) & 0xFF,
        config->rt_down & 0xFF,
        (config->rt_up >> 8) & 0xFF,
        config->rt_up & 0xFF,
        (key->rest_position >> 8) & 0xFF,
        key->rest_position & 0xFF,
        (key->down_position >> 8) & 0xFF,
        key->down_position & 0xFF,
        key->calibrated ? 1 : 0,
        0
    };
    raw_hid_send(response, RAW_EPSIZE);
}

static void handle_set_actuation(uint8_t *data) {
    uint8_t key_index = data[1];
    uint16_t actuation = (data[2] << 8) | data[3];
    uint16_t release = (data[4] << 8) | data[5];
    if (actuation < RT_MIN_ACTUATION || actuation > RT_MAX_ACTUATION) {
        send_error(HID_CMD_SET_ACTUATION, 0x02);
        return;
    }
    repad_config_t *cfg = repad_config_get();
    if (key_index == 255) {
        for (uint8_t i = 0; i < MATRIX_COLS; i++) {
            rt_config_t* config = get_rt_config(i);
            if (config) rt_config_set_static_points(config, actuation, release);
            cfg->key[i].actuation = actuation;
            cfg->key[i].release = release;
        }
    } else if (key_index < MATRIX_COLS) {
        rt_config_t* config = get_rt_config(key_index);
        if (config) rt_config_set_static_points(config, actuation, release);
        cfg->key[key_index].actuation = actuation;
        cfg->key[key_index].release = release;
    } else {
        send_error(HID_CMD_SET_ACTUATION, 0x01);
        return;
    }
    repad_config_mark_dirty();
    send_ok(HID_CMD_SET_ACTUATION);
}

static void handle_set_rt_sensitivity(uint8_t *data) {
    uint8_t key_index = data[1];
    uint16_t down_sens = (data[2] << 8) | data[3];
    uint16_t up_sens = (data[4] << 8) | data[5];
    if (down_sens < RT_MIN_SENSITIVITY || down_sens > RT_MAX_SENSITIVITY ||
        up_sens < RT_MIN_SENSITIVITY || up_sens > RT_MAX_SENSITIVITY) {
        send_error(HID_CMD_SET_RT_SENSITIVITY, 0x02);
        return;
    }
    repad_config_t *cfg = repad_config_get();
    if (key_index == 255) {
        for (uint8_t i = 0; i < MATRIX_COLS; i++) {
            rt_config_t* config = get_rt_config(i);
            if (config) rt_config_set_rt_sensitivity(config, down_sens, up_sens);
            cfg->key[i].rt_down = down_sens;
            cfg->key[i].rt_up = up_sens;
        }
    } else if (key_index < MATRIX_COLS) {
        rt_config_t* config = get_rt_config(key_index);
        if (config) rt_config_set_rt_sensitivity(config, down_sens, up_sens);
        cfg->key[key_index].rt_down = down_sens;
        cfg->key[key_index].rt_up = up_sens;
    } else {
        send_error(HID_CMD_SET_RT_SENSITIVITY, 0x01);
        return;
    }
    repad_config_mark_dirty();
    send_ok(HID_CMD_SET_RT_SENSITIVITY);
}

static void handle_set_rt_mode(uint8_t *data) {
    uint8_t key_index = data[1];
    uint8_t mode = data[2];
    if (mode > 3) {
        send_error(HID_CMD_SET_RT_MODE, 0x02);
        return;
    }
    rt_mode_t rt_mode = (rt_mode_t)mode;
    repad_config_t *cfg = repad_config_get();
    if (key_index == 255) {
        for (uint8_t i = 0; i < MATRIX_COLS; i++) {
            rt_config_t* config = get_rt_config(i);
            if (config) rt_config_set_mode(config, rt_mode);
            cfg->key[i].rt_mode = (uint8_t)rt_mode;
        }
    } else if (key_index < MATRIX_COLS) {
        rt_config_t* config = get_rt_config(key_index);
        if (config) rt_config_set_mode(config, rt_mode);
        cfg->key[key_index].rt_mode = (uint8_t)rt_mode;
    } else {
        send_error(HID_CMD_SET_RT_MODE, 0x01);
        return;
    }
    repad_config_mark_dirty();
    send_ok(HID_CMD_SET_RT_MODE);
}

static void handle_set_rgb_color(uint8_t *data) {
    uint8_t led_index = data[1];
    uint8_t r = data[2];
    uint8_t g = data[3];
    uint8_t b = data[4];
    repad_config_t *cfg = repad_config_get();
    cfg->rgb_mode = 1;
    if (!hid_rgb_control) {
        hid_rgb_control = true;
        rgb_matrix_disable_noeeprom();
    }
    if (led_index == 255) {
        for (uint8_t i = 0; i < 3; i++) {
            cfg->led[i].r = r;
            cfg->led[i].g = g;
            cfg->led[i].b = b;
        }
        const uint8_t br = cfg->rgb_brightness;
        rgb_multipin_set_color_all(scale_u8(r, br), scale_u8(g, br), scale_u8(b, br));
    } else if (led_index < 3) {
        cfg->led[led_index].r = r;
        cfg->led[led_index].g = g;
        cfg->led[led_index].b = b;
        const uint8_t br = cfg->rgb_brightness;
        rgb_multipin_set_color(led_index, scale_u8(r, br), scale_u8(g, br), scale_u8(b, br));
    } else {
        send_error(HID_CMD_SET_RGB_COLOR, 0x01);
        return;
    }
    rgb_multipin_flush();
    repad_config_mark_dirty();
    send_ok(HID_CMD_SET_RGB_COLOR);
}

static void handle_get_rgb_color(uint8_t *data) {
    uint8_t led_index = data[1];
    repad_config_t *cfg = repad_config_get();
    uint8_t response[RAW_EPSIZE] = {0};
    response[0] = HID_RESP_RGB_COLOR;
    response[1] = led_index;
    response[2] = cfg->rgb_mode;
    response[3] = cfg->rgb_brightness;
    if (led_index == 255) {
        for (uint8_t i = 0; i < 3; i++) {
            response[4 + i * 3 + 0] = cfg->led[i].r;
            response[4 + i * 3 + 1] = cfg->led[i].g;
            response[4 + i * 3 + 2] = cfg->led[i].b;
        }
    } else if (led_index < 3) {
        response[4] = cfg->led[led_index].r;
        response[5] = cfg->led[led_index].g;
        response[6] = cfg->led[led_index].b;
    } else {
        send_error(HID_CMD_GET_RGB_COLOR, 0x01);
        return;
    }
    raw_hid_send(response, RAW_EPSIZE);
}

static void handle_set_rgb_params(uint8_t *data) {
    repad_config_t *cfg = repad_config_get();
    cfg->rgb_mode = data[1];
    cfg->rgb_brightness = data[2];
    cfg->rgb_speed = data[7];
    if (cfg->rgb_mode == 0) {
        hid_rgb_control = false;
        rgb_matrix_enable_noeeprom();
        rgb_matrix_mode_noeeprom(data[3]);
        rgb_matrix_sethsv_noeeprom(data[4], data[5], data[6]);
        rgb_matrix_set_speed_noeeprom(data[7]);
    } else {
        if (!hid_rgb_control) {
            hid_rgb_control = true;
            rgb_matrix_disable_noeeprom();
        }
        if (cfg->rgb_mode == 1) {
            repad_config_apply();
        }
    }
    repad_config_mark_dirty();
    send_ok(HID_CMD_SET_RGB_PARAMS);
}

static void handle_get_rgb_params(uint8_t *data) {
    (void)data;
    repad_config_t *cfg = repad_config_get();
    uint8_t response[RAW_EPSIZE] = {0};
    response[0] = HID_RESP_RGB_PARAMS;
    response[1] = cfg->rgb_mode;
    response[2] = cfg->rgb_brightness;
    response[3] = rgb_matrix_get_mode();
    HSV hsv = rgb_matrix_get_hsv();
    response[4] = hsv.h;
    response[5] = hsv.s;
    response[6] = hsv.v;
    response[7] = cfg->rgb_speed;
    raw_hid_send(response, RAW_EPSIZE);
}

static void handle_get_calibration(uint8_t *data) {
    uint8_t key_index = data[1];
    repad_config_t *cfg = repad_config_get();
    uint8_t response[RAW_EPSIZE] = {0};
    response[0] = HID_RESP_CALIBRATION;
    response[1] = key_index;
    if (key_index == 255) {
        for (uint8_t i = 0; i < 3; i++) {
            response[2 + i * 4 + 0] = (cfg->key[i].adc_rest >> 8) & 0xFF;
            response[2 + i * 4 + 1] = cfg->key[i].adc_rest & 0xFF;
            response[2 + i * 4 + 2] = (cfg->key[i].adc_down >> 8) & 0xFF;
            response[2 + i * 4 + 3] = cfg->key[i].adc_down & 0xFF;
        }
    } else if (key_index < MATRIX_COLS) {
        response[2] = (cfg->key[key_index].adc_rest >> 8) & 0xFF;
        response[3] = cfg->key[key_index].adc_rest & 0xFF;
        response[4] = (cfg->key[key_index].adc_down >> 8) & 0xFF;
        response[5] = cfg->key[key_index].adc_down & 0xFF;
    } else {
        send_error(HID_CMD_GET_CALIBRATION, 0x01);
        return;
    }
    raw_hid_send(response, RAW_EPSIZE);
}

static void handle_set_calibration(uint8_t *data) {
    uint8_t key_index = data[1];
    uint16_t rest = (data[2] << 8) | data[3];
    uint16_t down = (data[4] << 8) | data[5];
    if (down <= rest) {
        send_error(HID_CMD_SET_CALIBRATION, 0x02);
        return;
    }
    repad_config_t *cfg = repad_config_get();
    if (key_index == 255) {
        for (uint8_t i = 0; i < MATRIX_COLS; i++) {
            cfg->key[i].adc_rest = rest;
            cfg->key[i].adc_down = down;
            he_key_t *key = get_he_key(i);
            if (key) {
                key->rest_position = rest;
                key->down_position = down;
                key->calibrated = (down > rest + SENSOR_BOUNDARY_MIN_DISTANCE);
            }
        }
    } else if (key_index < MATRIX_COLS) {
        cfg->key[key_index].adc_rest = rest;
        cfg->key[key_index].adc_down = down;
        he_key_t *key = get_he_key(key_index);
        if (key) {
            key->rest_position = rest;
            key->down_position = down;
            key->calibrated = (down > rest + SENSOR_BOUNDARY_MIN_DISTANCE);
        }
    } else {
        send_error(HID_CMD_SET_CALIBRATION, 0x01);
        return;
    }
    repad_config_mark_dirty();
    send_ok(HID_CMD_SET_CALIBRATION);
}

static void handle_keymap_get_keycode(uint8_t *data) {
    uint8_t layer = data[1];
    uint8_t row = data[2];
    uint8_t col = data[3];
    if (row >= MATRIX_ROWS || col >= MATRIX_COLS || layer >= dynamic_keymap_get_layer_count()) {
        send_error(HID_CMD_KEYMAP_GET_KEYCODE, 0x01);
        return;
    }
    uint16_t kc = dynamic_keymap_get_keycode(layer, row, col);
    uint8_t response[RAW_EPSIZE] = {0};
    response[0] = HID_RESP_KEYMAP_KEYCODE;
    response[1] = layer;
    response[2] = row;
    response[3] = col;
    response[4] = (kc >> 8) & 0xFF;
    response[5] = kc & 0xFF;
    raw_hid_send(response, RAW_EPSIZE);
}

static void handle_keymap_set_keycode(uint8_t *data) {
    uint8_t layer = data[1];
    uint8_t row = data[2];
    uint8_t col = data[3];
    uint16_t kc = (data[4] << 8) | data[5];
    if (row >= MATRIX_ROWS || col >= MATRIX_COLS || layer >= dynamic_keymap_get_layer_count()) {
        send_error(HID_CMD_KEYMAP_SET_KEYCODE, 0x01);
        return;
    }
    dynamic_keymap_set_keycode(layer, row, col, kc);
    send_ok(HID_CMD_KEYMAP_SET_KEYCODE);
}

static void handle_keymap_reset(uint8_t *data) {
    (void)data;
    dynamic_keymap_reset();
    send_ok(HID_CMD_KEYMAP_RESET);
}

static void handle_keymap_info(uint8_t *data) {
    (void)data;
    uint8_t response[RAW_EPSIZE] = {0};
    response[0] = HID_RESP_KEYMAP_INFO;
    response[1] = dynamic_keymap_get_layer_count();
    response[2] = MATRIX_ROWS;
    response[3] = MATRIX_COLS;
    raw_hid_send(response, RAW_EPSIZE);
}

static void handle_macro_info(uint8_t *data) {
    (void)data;
    uint8_t count = dynamic_keymap_macro_get_count();
    uint16_t size = dynamic_keymap_macro_get_buffer_size();
    uint8_t response[RAW_EPSIZE] = {0};
    response[0] = HID_RESP_MACRO_INFO;
    response[1] = count;
    response[2] = (size >> 8) & 0xFF;
    response[3] = size & 0xFF;
    raw_hid_send(response, RAW_EPSIZE);
}

static void handle_macro_get_buffer(uint8_t *data) {
    uint16_t offset = (data[1] << 8) | data[2];
    uint8_t size = data[3];
    if (size > (RAW_EPSIZE - 4)) size = RAW_EPSIZE - 4;
    uint8_t response[RAW_EPSIZE] = {0};
    response[0] = HID_RESP_MACRO_BUFFER;
    response[1] = (offset >> 8) & 0xFF;
    response[2] = offset & 0xFF;
    response[3] = size;
    dynamic_keymap_macro_get_buffer(offset, size, &response[4]);
    raw_hid_send(response, RAW_EPSIZE);
}

static void handle_macro_set_buffer(uint8_t *data) {
    uint16_t offset = (data[1] << 8) | data[2];
    uint8_t size = data[3];
    if (size > (RAW_EPSIZE - 4)) {
        send_error(HID_CMD_MACRO_SET_BUFFER, 0x01);
        return;
    }
    dynamic_keymap_macro_set_buffer(offset, size, &data[4]);
    send_ok(HID_CMD_MACRO_SET_BUFFER);
}

static void handle_macro_reset(uint8_t *data) {
    (void)data;
    dynamic_keymap_macro_reset();
    send_ok(HID_CMD_MACRO_RESET);
}

static void handle_macro_send(uint8_t *data) {
    uint8_t id = data[1];
    dynamic_keymap_macro_send(id);
    send_ok(HID_CMD_MACRO_SEND);
}

static void handle_stream_adc(uint8_t *data) {
    adc_streaming_enabled = data[1] != 0;
    stream_interval_ms = (data[2] << 8) | data[3];
    if (stream_interval_ms < 1) stream_interval_ms = 1;
    if (stream_interval_ms > 1000) stream_interval_ms = 1000;
    send_ok(HID_CMD_STREAM_ADC);
}

static void handle_calibrate(uint8_t *data) {
    uint8_t key_index = data[1];
    if (key_index == 255) {
        he_keys_reset_calibration();
    } else if (key_index < MATRIX_COLS) {
        he_key_t* key = get_he_key(key_index);
        if (key) he_key_reset_calibration(key);
    } else {
        send_error(HID_CMD_CALIBRATE, 0x01);
        return;
    }
    repad_config_mark_dirty();
    send_ok(HID_CMD_CALIBRATE);
}

static void handle_save_config(uint8_t *data) {
    (void)data;
    repad_config_save();
    send_ok(HID_CMD_SAVE_CONFIG);
}

static void handle_load_config(uint8_t *data) {
    (void)data;
    repad_config_load();
    repad_config_apply();
    repad_config_apply_rgb();
    send_ok(HID_CMD_LOAD_CONFIG);
}

void raw_hid_receive(uint8_t *data, uint8_t length) {
    if (length < 1) return;
    uint8_t cmd = data[0];
    switch (cmd) {
        case HID_CMD_HANDSHAKE:          handle_handshake(data); break;
        case HID_CMD_GET_STATUS:         handle_get_status(data); break;
        case HID_CMD_GET_KEY_CONFIG:     handle_get_key_config(data); break;
        case HID_CMD_SET_ACTUATION:      handle_set_actuation(data); break;
        case HID_CMD_SET_RT_SENSITIVITY: handle_set_rt_sensitivity(data); break;
        case HID_CMD_SET_RT_MODE:        handle_set_rt_mode(data); break;
        case HID_CMD_SET_RGB_COLOR:      handle_set_rgb_color(data); break;
        case HID_CMD_GET_RGB_COLOR:      handle_get_rgb_color(data); break;
        case HID_CMD_SET_RGB_PARAMS:     handle_set_rgb_params(data); break;
        case HID_CMD_GET_RGB_PARAMS:     handle_get_rgb_params(data); break;
        case HID_CMD_GET_CALIBRATION:    handle_get_calibration(data); break;
        case HID_CMD_SET_CALIBRATION:    handle_set_calibration(data); break;
        case HID_CMD_KEYMAP_GET_KEYCODE: handle_keymap_get_keycode(data); break;
        case HID_CMD_KEYMAP_SET_KEYCODE: handle_keymap_set_keycode(data); break;
        case HID_CMD_KEYMAP_RESET:       handle_keymap_reset(data); break;
        case HID_CMD_KEYMAP_INFO:        handle_keymap_info(data); break;
        case HID_CMD_MACRO_INFO:         handle_macro_info(data); break;
        case HID_CMD_MACRO_GET_BUFFER:   handle_macro_get_buffer(data); break;
        case HID_CMD_MACRO_SET_BUFFER:   handle_macro_set_buffer(data); break;
        case HID_CMD_MACRO_RESET:        handle_macro_reset(data); break;
        case HID_CMD_MACRO_SEND:         handle_macro_send(data); break;
        case HID_CMD_STREAM_ADC:         handle_stream_adc(data); break;
        case HID_CMD_CALIBRATE:          handle_calibrate(data); break;
        case HID_CMD_SAVE_CONFIG:        handle_save_config(data); break;
        case HID_CMD_LOAD_CONFIG:        handle_load_config(data); break;
        default: send_error(cmd, 0xFF); break;
    }
}

void hid_stream_adc_task(void) {
    if (!adc_streaming_enabled) return;
    uint32_t now = timer_read32();
    if (now - last_stream_time < stream_interval_ms) return;
    last_stream_time = now;
    uint8_t response[RAW_EPSIZE] = {HID_RESP_ADC_DATA, 0};
    for (uint8_t i = 0; i < MATRIX_COLS; i++) {
        he_key_t* key = get_he_key(i);
        if (key) {
            response[2 + i*6] = (key->raw_value >> 8) & 0xFF;
            response[3 + i*6] = key->raw_value & 0xFF;
            response[4 + i*6] = (key->distance >> 8) & 0xFF;
            response[5 + i*6] = key->distance & 0xFF;
            response[6 + i*6] = (key->rest_position >> 8) & 0xFF;
            response[7 + i*6] = key->rest_position & 0xFF;
        }
    }
    response[20] = 0;
    response[21] = 0;
    for (uint8_t i = 0; i < MATRIX_COLS; i++) {
        he_key_t* key = get_he_key(i);
        if (key) {
            if (key->pressed) response[20] |= (1 << i);
            if (key->calibrated) response[21] |= (1 << i);
        }
    }
    raw_hid_send(response, RAW_EPSIZE);
}
