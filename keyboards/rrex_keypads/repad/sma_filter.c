#include "sma_filter.h"
#include <string.h>

void sma_init(sma_filter_t* filter) {
    memset(filter->buffer, 0, sizeof(filter->buffer));
    filter->index = 0;
    filter->sum = 0;
    filter->initialized = false;
}

uint16_t sma_filter(sma_filter_t* filter, uint16_t value) {
    filter->sum = filter->sum - filter->buffer[filter->index] + value;
    filter->buffer[filter->index] = value;
    filter->index = (filter->index + 1) & (SMA_SAMPLES - 1);
    if (filter->index == 0) {
        filter->initialized = true;
    }
    return filter->sum >> SMA_SAMPLES_EXPONENT;
}
