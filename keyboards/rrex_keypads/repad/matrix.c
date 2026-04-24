#include "quantum.h"
#include "repad.h"
#include "config.h"
#include "repad_config.h"

static matrix_row_t matrix[MATRIX_ROWS];
static he_key_t he_keys[MATRIX_COLS];
static rt_config_t rt_configs[MATRIX_COLS];

static const bool key_enabled[MATRIX_COLS] = {
    KEY_0_ENABLED,
    KEY_1_ENABLED,
    KEY_2_ENABLED
};

static const pin_t adc_pins[MATRIX_COLS] = {
    ANALOG_PIN_0,
    ANALOG_PIN_1,
    ANALOG_PIN_2
};

he_key_t* get_he_key(uint8_t key_index) {
    if (key_index >= MATRIX_COLS) return NULL;
    return &he_keys[key_index];
}

rt_config_t* get_rt_config(uint8_t key_index) {
    if (key_index >= MATRIX_COLS) return NULL;
    return &rt_configs[key_index];
}

void he_keys_init(void) {
    for (uint8_t i = 0; i < MATRIX_COLS; i++) {
        he_key_init(&he_keys[i]);
        rt_config_init(&rt_configs[i]);
    }
}

void he_keys_process(void) {
    for (uint8_t i = 0; i < MATRIX_COLS; i++) {
        if (key_enabled[i]) {
            uint16_t prev_rest = he_keys[i].rest_position;
            uint16_t prev_down = he_keys[i].down_position;
            he_key_process(&he_keys[i], adc_pins[i]);
            repad_config_t *cfg = repad_config_get();
            if (cfg) {
                cfg->key[i].adc_rest = he_keys[i].rest_position;
                cfg->key[i].adc_down = he_keys[i].down_position;
            }
            if (he_keys[i].rest_position != prev_rest || he_keys[i].down_position != prev_down) {
                repad_config_mark_dirty();
            }
        }
    }
}

void he_keys_reset_calibration(void) {
    for (uint8_t i = 0; i < MATRIX_COLS; i++) {
        he_key_reset_calibration(&he_keys[i]);
    }
}

void matrix_init_custom(void) {
    for (uint8_t row = 0; row < MATRIX_ROWS; row++) {
        matrix[row] = 0;
    }
    he_keys_init();
    repad_config_init();
    wait_ms(50);
}

bool matrix_scan_custom(matrix_row_t current_matrix[]) {
    bool changed = false;
    he_keys_process();
    matrix_row_t new_row = 0;
    for (uint8_t col = 0; col < MATRIX_COLS; col++) {
        if (!key_enabled[col]) continue;
        rt_process(&he_keys[col], &rt_configs[col]);
        if (he_keys[col].pressed) {
            new_row |= (1 << col);
        }
    }
    if (matrix[0] != new_row) {
        matrix[0] = new_row;
        changed = true;
    }
    current_matrix[0] = matrix[0];
    return changed;
}
