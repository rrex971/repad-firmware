#include "he_analog.h"
#include "analog.h"
#include <string.h>

void he_key_init(he_key_t* key) {
    memset(key, 0, sizeof(he_key_t));
    sma_init(&key->filter);
    key->rest_position = HE_ADC_DEFAULT_REST;
    key->down_position = HE_ADC_DEFAULT_DOWN;
    key->calibrated = (key->down_position > key->rest_position + SENSOR_BOUNDARY_MIN_DISTANCE);
    key->pressed = false;
    key->in_rapid_trigger_zone = false;
    key->rapid_trigger_peak = 0;
}

void he_key_update_raw(he_key_t* key, uint8_t adc_channel) {
    uint16_t raw = analogReadPin(adc_channel);
    key->raw_value = sma_filter(&key->filter, raw);
}

void he_key_update_boundaries(he_key_t* key) {
    if (!key->filter.initialized) return;
    uint16_t value = key->raw_value;
    if (value + SENSOR_BOUNDARY_DEADZONE < key->rest_position) {
        key->rest_position = value;
    }
    if (value > key->down_position + SENSOR_BOUNDARY_DEADZONE) {
        key->down_position = value;
    }
    key->calibrated = (key->down_position > key->rest_position + SENSOR_BOUNDARY_MIN_DISTANCE);
}

void he_key_calculate_distance(he_key_t* key) {
    if (!key->calibrated) {
        key->distance = 0;
        return;
    }
    uint16_t value = key->raw_value;
    if (value < key->rest_position) value = key->rest_position;
    if (value > key->down_position) value = key->down_position;
    uint16_t range = key->down_position - key->rest_position;
    if (range == 0) {
        key->distance = 0;
        return;
    }
    uint16_t travel = value - key->rest_position;
    key->distance = (uint16_t)(((uint32_t)travel * TRAVEL_DISTANCE_IN_0_01MM) / range);
}

void he_key_process(he_key_t* key, uint8_t adc_channel) {
    he_key_update_raw(key, adc_channel);
    he_key_update_boundaries(key);
    he_key_calculate_distance(key);
}

bool he_key_is_calibrated(he_key_t* key) {
    return key->calibrated;
}

uint16_t he_key_get_distance(he_key_t* key) {
    return key->distance;
}

uint16_t he_key_get_raw(he_key_t* key) {
    return key->raw_value;
}

void he_key_reset_calibration(he_key_t* key) {
    key->rest_position = 0xFFFF;
    key->down_position = 0;
    key->calibrated = false;
}
