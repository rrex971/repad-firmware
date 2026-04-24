#include "repad_config.h"
#include <string.h>
#include "eeconfig.h"
#include "rgb_matrix.h"
#include "repad.h"

static repad_config_t g_cfg;
static bool g_dirty = false;
static uint32_t g_dirty_time = 0;

static inline uint8_t scale_u8(uint8_t value, uint8_t brightness) {
    return (uint8_t)(((uint16_t)value * brightness) / 255);
}

static void cfg_defaults(repad_config_t *cfg) {
    memset(cfg, 0, sizeof(*cfg));
    cfg->magic = REPAD_CFG_MAGIC;
    cfg->version = REPAD_CFG_VERSION;
    for (uint8_t i = 0; i < 3; i++) {
        cfg->key[i].rt_mode = (uint8_t)RT_MODE_STATIC;
        cfg->key[i].actuation = DEFAULT_ACTUATION_POINT;
        cfg->key[i].release = DEFAULT_RELEASE_POINT;
        cfg->key[i].rt_down = DEFAULT_RT_DOWN;
        cfg->key[i].rt_up = DEFAULT_RT_UP;
        cfg->key[i].adc_rest = HE_ADC_DEFAULT_REST;
        cfg->key[i].adc_down = HE_ADC_DEFAULT_DOWN;
    }
    cfg->rgb_brightness = RGB_MATRIX_DEFAULT_VAL;
    cfg->rgb_mode = 0;
    cfg->rgb_speed = 30;
    cfg->led[0].r = 125; cfg->led[0].g = 207; cfg->led[0].b = 255;
    cfg->led[1].r = 122; cfg->led[1].g = 162; cfg->led[1].b = 247;
    cfg->led[2].r = 187; cfg->led[2].g = 154; cfg->led[2].b = 247;
}

static bool cfg_valid(const repad_config_t *cfg) {
    return (cfg->magic == REPAD_CFG_MAGIC) && (cfg->version == REPAD_CFG_VERSION);
}

repad_config_t *repad_config_get(void) {
    return &g_cfg;
}

void repad_config_mark_dirty(void) {
    g_dirty = true;
    g_dirty_time = timer_read32();
}

void repad_config_autosave_tick(void) {
    if (!g_dirty) return;
    if (timer_elapsed32(g_dirty_time) >= REPAD_AUTOSAVE_DELAY_MS) {
        repad_config_save();
    }
}

void repad_config_save(void) {
    eeconfig_update_user_datablock(&g_cfg, 0, sizeof(g_cfg));
    g_dirty = false;
}

void repad_config_load(void) {
    repad_config_t tmp;
    eeconfig_read_user_datablock(&tmp, 0, sizeof(tmp));
    if (!cfg_valid(&tmp)) {
        cfg_defaults(&g_cfg);
        repad_config_save();
        return;
    }
    g_cfg = tmp;
}

void repad_config_apply(void) {
    for (uint8_t i = 0; i < 3; i++) {
        he_key_t *key = get_he_key(i);
        rt_config_t *rt = get_rt_config(i);
        if (!key || !rt) continue;
        key->rest_position = g_cfg.key[i].adc_rest;
        key->down_position = g_cfg.key[i].adc_down;
        key->calibrated = (key->down_position > key->rest_position + SENSOR_BOUNDARY_MIN_DISTANCE);
        rt->mode = (rt_mode_t)g_cfg.key[i].rt_mode;
        rt->actuation_point = g_cfg.key[i].actuation;
        rt->release_point = g_cfg.key[i].release;
        rt->rt_down = g_cfg.key[i].rt_down;
        rt->rt_up = g_cfg.key[i].rt_up;
    }
}

void repad_config_apply_rgb(void) {
    if (g_cfg.rgb_mode == 1) {
        rgb_matrix_disable_noeeprom();
        const uint8_t br = g_cfg.rgb_brightness;
        rgb_multipin_set_color(0, scale_u8(g_cfg.led[0].r, br), scale_u8(g_cfg.led[0].g, br), scale_u8(g_cfg.led[0].b, br));
        rgb_multipin_set_color(1, scale_u8(g_cfg.led[1].r, br), scale_u8(g_cfg.led[1].g, br), scale_u8(g_cfg.led[1].b, br));
        rgb_multipin_set_color(2, scale_u8(g_cfg.led[2].r, br), scale_u8(g_cfg.led[2].g, br), scale_u8(g_cfg.led[2].b, br));
        rgb_multipin_flush();
    } else if (g_cfg.rgb_mode == 2) {
        rgb_matrix_disable_noeeprom();
    } else {
        rgb_matrix_enable_noeeprom();
    }
}

void repad_config_init(void) {
    repad_config_load();
    repad_config_apply();
}

__attribute__((weak)) void eeconfig_init_user_datablock(void) {
    cfg_defaults(&g_cfg);
    repad_config_save();
}
