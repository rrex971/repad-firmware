#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "sma_filter.h"
#include "config.h"

typedef struct {
    uint16_t raw_value;
    uint16_t distance;
    uint16_t rest_position;
    uint16_t down_position;
    bool calibrated;
    bool pressed;
    bool in_rapid_trigger_zone;
    uint16_t rapid_trigger_peak;
    sma_filter_t filter;
} he_key_t;

void he_key_init(he_key_t* key);
void he_key_update_raw(he_key_t* key, uint8_t adc_channel);
void he_key_update_boundaries(he_key_t* key);
void he_key_calculate_distance(he_key_t* key);
void he_key_process(he_key_t* key, uint8_t adc_channel);
bool he_key_is_calibrated(he_key_t* key);
uint16_t he_key_get_distance(he_key_t* key);
uint16_t he_key_get_raw(he_key_t* key);
void he_key_reset_calibration(he_key_t* key);
