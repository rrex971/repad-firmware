#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "quantum.h"

#define REPAD_CFG_MAGIC 0x5245u
#define REPAD_CFG_VERSION 0x01u
#define REPAD_AUTOSAVE_DELAY_MS 2000

typedef struct __attribute__((packed)) {
    uint16_t magic;
    uint8_t version;
    uint8_t flags;

    struct __attribute__((packed)) {
        uint8_t rt_mode;
        uint16_t actuation;
        uint16_t release;
        uint16_t rt_down;
        uint16_t rt_up;
        uint16_t adc_rest;
        uint16_t adc_down;
    } key[3];

    uint8_t rgb_flags;
    uint8_t rgb_brightness;
    uint8_t rgb_mode;
    uint8_t rgb_speed;

    struct __attribute__((packed)) {
        uint8_t r;
        uint8_t g;
        uint8_t b;
    } led[3];

    uint8_t reserved1[16];
} repad_config_t;

void repad_config_init(void);
void repad_config_apply(void);
void repad_config_save(void);
void repad_config_load(void);
void repad_config_mark_dirty(void);
void repad_config_autosave_tick(void);
void repad_config_apply_rgb(void);
repad_config_t *repad_config_get(void);
