#pragma once

#include <stdint.h>
#include <stdbool.h>

#define SMA_SAMPLES_EXPONENT 2
#define SMA_SAMPLES (1 << SMA_SAMPLES_EXPONENT)

typedef struct {
    uint16_t buffer[SMA_SAMPLES];
    uint8_t  index;
    uint32_t sum;
    bool     initialized;
} sma_filter_t;

void sma_init(sma_filter_t* filter);
uint16_t sma_filter(sma_filter_t* filter, uint16_t value);
