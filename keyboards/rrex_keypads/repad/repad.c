#include "quantum.h"
#include "repad.h"
#include "repad_config.h"
#include "color.h"

static uint32_t rgb_tick_timer = 0;

static void repad_rgb_tick(void) {
    repad_config_t *cfg = repad_config_get();
    if (cfg->rgb_mode != 2) return;
    uint32_t now = timer_read32();
    if (now - rgb_tick_timer < 16) return;
    rgb_tick_timer = now;
    uint8_t speed = cfg->rgb_speed;
    if (speed == 0) speed = 30;
    uint8_t base_hue = (uint8_t)((now * (uint32_t)speed) >> 10);
    for (uint8_t i = 0; i < 3; i++) {
        uint8_t hue = base_hue + (i * 30);
        HSV hsv = {hue, 170, cfg->rgb_brightness};
        RGB rgb = hsv_to_rgb(hsv);
        rgb_multipin_set_color(i, rgb.r, rgb.g, rgb.b);
    }
    rgb_multipin_flush();
}

void keyboard_post_init_kb(void) {
    repad_config_apply_rgb();
    keyboard_post_init_user();
}

void housekeeping_task_kb(void) {
    hid_stream_adc_task();
    repad_rgb_tick();
    repad_config_autosave_tick();
    housekeeping_task_user();
}

#ifdef RGB_MATRIX_ENABLE
bool rgb_matrix_indicators_kb(void) {
    if (!rgb_matrix_indicators_user()) {
        return false;
    }
    return true;
}
#endif
