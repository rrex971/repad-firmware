#include "rapid_trigger.h"

void rt_config_init(rt_config_t* config) {
    config->mode = RT_MODE_STATIC;
    config->actuation_point = 200;
    config->release_point = 180;
    config->rt_down = 40;
    config->rt_up = 40;
}

void rt_config_set_mode(rt_config_t* config, rt_mode_t mode) {
    config->mode = mode;
}

void rt_config_set_static_points(rt_config_t* config, uint16_t actuation, uint16_t release) {
    config->actuation_point = actuation;
    config->release_point = release;
}

void rt_config_set_rt_sensitivity(rt_config_t* config, uint16_t down, uint16_t up) {
    config->rt_down = down;
    config->rt_up = up;
}

bool rt_process_static(he_key_t* key, rt_config_t* config) {
    bool state_changed = false;
    uint16_t distance = key->distance;
    if (!key->pressed) {
        uint16_t actuation_with_tolerance = config->actuation_point + HYSTERESIS_TOLERANCE;
        if (distance >= actuation_with_tolerance) {
            key->pressed = true;
            state_changed = true;
        }
    } else {
        uint16_t release_with_tolerance = config->release_point - HYSTERESIS_TOLERANCE;
        if (distance <= release_with_tolerance) {
            key->pressed = false;
            state_changed = true;
        }
    }
    return state_changed;
}

bool rt_process_dynamic(he_key_t* key, rt_config_t* config) {
    bool state_changed = false;
    uint16_t distance = key->distance;
    if (key->in_rapid_trigger_zone && !key->pressed) {
        if (distance < config->actuation_point) {
            key->in_rapid_trigger_zone = false;
            key->rapid_trigger_peak = 0;
        }
    }
    if (!key->in_rapid_trigger_zone && !key->pressed) {
        uint16_t actuation_with_tolerance = config->actuation_point + RAPID_TRIGGER_TOLERANCE;
        if (distance >= actuation_with_tolerance) {
            key->pressed = true;
            key->in_rapid_trigger_zone = true;
            key->rapid_trigger_peak = distance;
            state_changed = true;
        }
    }
    if (key->in_rapid_trigger_zone) {
        if (key->pressed) {
            if (distance > key->rapid_trigger_peak) {
                key->rapid_trigger_peak = distance;
            } else {
                uint16_t release_threshold = key->rapid_trigger_peak - config->rt_up;
                if (key->rapid_trigger_peak < config->rt_up) {
                    release_threshold = 0;
                }
                if (distance <= release_threshold) {
                    key->pressed = false;
                    key->rapid_trigger_peak = distance;
                    state_changed = true;
                }
            }
        } else {
            if (distance < key->rapid_trigger_peak) {
                key->rapid_trigger_peak = distance;
            } else {
                uint16_t press_threshold = key->rapid_trigger_peak + config->rt_down;
                if (press_threshold > TRAVEL_DISTANCE_IN_0_01MM) {
                    press_threshold = TRAVEL_DISTANCE_IN_0_01MM;
                }
                if (distance >= press_threshold) {
                    key->pressed = true;
                    key->rapid_trigger_peak = distance;
                    state_changed = true;
                }
            }
        }
    }
    return state_changed;
}

bool rt_process_continuous(he_key_t* key, rt_config_t* config) {
    bool state_changed = false;
    uint16_t distance = key->distance;
    if (key->in_rapid_trigger_zone && !key->pressed) {
        if (distance <= CONTINUOUS_RAPID_TRIGGER_THRESHOLD) {
            key->in_rapid_trigger_zone = false;
            key->rapid_trigger_peak = 0;
        }
    }
    if (!key->in_rapid_trigger_zone) {
        uint16_t actuation_with_tolerance = config->actuation_point + RAPID_TRIGGER_TOLERANCE;
        if (distance >= actuation_with_tolerance) {
            key->pressed = true;
            key->in_rapid_trigger_zone = true;
            key->rapid_trigger_peak = distance;
            state_changed = true;
        }
    }
    if (key->in_rapid_trigger_zone) {
        if (key->pressed) {
            if (distance > key->rapid_trigger_peak) {
                key->rapid_trigger_peak = distance;
            } else {
                uint16_t release_threshold = key->rapid_trigger_peak - config->rt_up;
                if (key->rapid_trigger_peak < config->rt_up) {
                    release_threshold = 0;
                }
                if (distance <= release_threshold) {
                    key->pressed = false;
                    key->rapid_trigger_peak = distance;
                    state_changed = true;
                }
            }
        } else {
            if (distance < key->rapid_trigger_peak) {
                key->rapid_trigger_peak = distance;
            } else {
                uint16_t press_threshold = key->rapid_trigger_peak + config->rt_down;
                if (press_threshold > TRAVEL_DISTANCE_IN_0_01MM) {
                    press_threshold = TRAVEL_DISTANCE_IN_0_01MM;
                }
                if (distance >= press_threshold) {
                    key->pressed = true;
                    key->rapid_trigger_peak = distance;
                    state_changed = true;
                }
            }
        }
    }
    return state_changed;
}

bool rt_process(he_key_t* key, rt_config_t* config) {
    if (!key->calibrated) return false;
    switch (config->mode) {
        case RT_MODE_DISABLED:   return false;
        case RT_MODE_STATIC:     return rt_process_static(key, config);
        case RT_MODE_DYNAMIC:    return rt_process_dynamic(key, config);
        case RT_MODE_CONTINUOUS:  return rt_process_continuous(key, config);
        default:                 return false;
    }
}
