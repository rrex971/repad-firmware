#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "he_analog.h"
#include "config.h"

typedef enum {
    RT_MODE_DISABLED = 0,
    RT_MODE_STATIC,
    RT_MODE_DYNAMIC,
    RT_MODE_CONTINUOUS
} rt_mode_t;

typedef struct {
    rt_mode_t mode;
    uint16_t actuation_point;
    uint16_t release_point;
    uint16_t rt_down;
    uint16_t rt_up;
} rt_config_t;

void rt_config_init(rt_config_t* config);
void rt_config_set_mode(rt_config_t* config, rt_mode_t mode);
void rt_config_set_static_points(rt_config_t* config, uint16_t actuation, uint16_t release);
void rt_config_set_rt_sensitivity(rt_config_t* config, uint16_t down, uint16_t up);
bool rt_process(he_key_t* key, rt_config_t* config);
bool rt_process_static(he_key_t* key, rt_config_t* config);
bool rt_process_dynamic(he_key_t* key, rt_config_t* config);
bool rt_process_continuous(he_key_t* key, rt_config_t* config);
