#include QMK_KEYBOARD_H

const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    [0] = LAYOUT(KC_Z, KC_X, KC_C),
    [1] = LAYOUT(KC_F13, KC_F14, KC_F15),
};

#ifdef RGB_MATRIX_ENABLE
bool rgb_matrix_indicators_user(void) {
    return true;
}
#endif
