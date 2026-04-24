#include "hardware/clocks.h"
#include <hal.h>
#include "hardware/pio.h"

#include "quantum.h"
#include "repad.h"

#include "rgb_matrix.h"

#define MP_T0H  350
#define MP_T0L  900
#define MP_T1H  900
#define MP_T1L  350
#define MP_TRST 280

#define MP_PIO_T1L (MP_T1L / 50)
#define MP_PIO_T1L_A (MAX((MP_PIO_T1L + 1) / 2 - 1, 0))
#define MP_PIO_T1L_B (MAX(MP_PIO_T1L / 2 - 1, 0))
#define MP_PIO_T0L (MAX(MP_T0L / 50 - MP_PIO_T1L, 0))
#define MP_PIO_T0L_A (MAX(MP_PIO_T0L - 1, 0))
#define MP_PIO_T0H (MP_T0H / 50)
#define MP_PIO_T0H_A MAX(MP_PIO_T0H - 1, 0)
#define MP_PIO_T1H (MAX(MP_T1H / 50 - MP_PIO_T0H, 0))
#define MP_PIO_T1H_A (MAX(((MP_PIO_T1H + 1) / 2) - 1, 0))
#define MP_PIO_T1H_B (MAX((MP_PIO_T1H / 2) - 1, 0))

#define PIO_DELAY(delay, opcode) (((delay & 0xF) << 8U) | opcode)

#define MP_WRAP_TARGET 0
#define MP_WRAP 5

static const uint16_t mp_pio_program_instructions[] = {
    PIO_DELAY(MP_PIO_T1L_A, 0x6021),
    PIO_DELAY(MP_PIO_T1L_B, 0xa042),
    PIO_DELAY(MP_PIO_T0H_A, 0x1025),
    PIO_DELAY(MP_PIO_T1H_A, 0xb042),
    PIO_DELAY(MP_PIO_T1H_B, 0x1000),
    PIO_DELAY(MP_PIO_T0L_A, 0xa042),
};

static const pio_program_t mp_pio_program = {
    .instructions = mp_pio_program_instructions,
    .length       = 6,
    .origin       = -1,
};

static const PIO mp_pio = pio1;
static int mp_sm[3] = {-1, -1, -1};
static uint32_t led_grb[3];

static const pin_t rgb_pins[3] = {
    GP29,
    GP16,
    GP0
};

void rgb_multipin_init(void) {
    uint pio_idx = pio_get_index(mp_pio);
    hal_lld_peripheral_unreset(pio_idx == 0 ? RESETS_ALLREG_PIO0 : RESETS_ALLREG_PIO1);

    uint offset = pio_add_program(mp_pio, &mp_pio_program);

    iomode_t pin_mode = PAL_RP_PAD_SLEWFAST |
                        PAL_RP_GPIO_OE |
                        (pio_idx == 0 ? PAL_MODE_ALTERNATE_PIO0 : PAL_MODE_ALTERNATE_PIO1);

    float div = clock_get_hz(clk_sys) / (20.0f * 1000000.0f);

    for (uint8_t i = 0; i < 3; i++) {
        mp_sm[i] = pio_claim_unused_sm(mp_pio, true);
        if (mp_sm[i] < 0) continue;

        palSetLineMode(rgb_pins[i], pin_mode);
        pio_sm_set_consecutive_pindirs(mp_pio, mp_sm[i], rgb_pins[i], 1, true);

        pio_sm_config config = pio_get_default_sm_config();
        sm_config_set_wrap(&config, offset + MP_WRAP_TARGET, offset + MP_WRAP);
        sm_config_set_sideset_pins(&config, rgb_pins[i]);
        sm_config_set_fifo_join(&config, PIO_FIFO_JOIN_TX);
        sm_config_set_sideset(&config, 1, false, false);
        sm_config_set_out_shift(&config, false, true, 24);
        sm_config_set_clkdiv(&config, div);

        pio_sm_init(mp_pio, mp_sm[i], offset, &config);
        pio_sm_set_enabled(mp_pio, mp_sm[i], true);

        led_grb[i] = 0;
    }
}

void rgb_multipin_set_color(uint8_t index, uint8_t r, uint8_t g, uint8_t b) {
    if (index >= 3) return;
    led_grb[index] = ((uint32_t)g << 24) | ((uint32_t)r << 16) | ((uint32_t)b << 8);
}

void rgb_multipin_set_color_all(uint8_t r, uint8_t g, uint8_t b) {
    uint32_t grb = ((uint32_t)g << 24) | ((uint32_t)r << 16) | ((uint32_t)b << 8);
    led_grb[0] = grb;
    led_grb[1] = grb;
    led_grb[2] = grb;
}

void rgb_multipin_flush(void) {
    for (uint8_t i = 0; i < 3; i++) {
        if (mp_sm[i] < 0) continue;
        pio_sm_put_blocking(mp_pio, mp_sm[i], led_grb[i]);
    }
}

static inline uint8_t scale_brightness(uint8_t value, uint8_t brightness) {
    return (uint8_t)(((uint16_t)value * brightness) / 255);
}

static void custom_init(void) {
    rgb_multipin_init();
}

static void custom_flush(void) {
    rgb_multipin_flush();
}

static void custom_set_color(int index, uint8_t r, uint8_t g, uint8_t b) {
    uint8_t brightness = rgb_matrix_get_val();
    r = scale_brightness(r, brightness);
    g = scale_brightness(g, brightness);
    b = scale_brightness(b, brightness);
    rgb_multipin_set_color(index, r, g, b);
}

static void custom_set_color_all(uint8_t r, uint8_t g, uint8_t b) {
    uint8_t brightness = rgb_matrix_get_val();
    r = scale_brightness(r, brightness);
    g = scale_brightness(g, brightness);
    b = scale_brightness(b, brightness);
    rgb_multipin_set_color_all(r, g, b);
}

const rgb_matrix_driver_t rgb_matrix_driver = {
    .init          = custom_init,
    .flush         = custom_flush,
    .set_color     = custom_set_color,
    .set_color_all = custom_set_color_all,
};
