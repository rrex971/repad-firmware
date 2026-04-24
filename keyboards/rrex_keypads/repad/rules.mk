MCU = RP2040
BOOTLOADER = rp2040

CUSTOM_MATRIX = lite
SRC += matrix.c
SRC += sma_filter.c
SRC += he_analog.c
SRC += rapid_trigger.c
SRC += repad_config.c

ANALOG_DRIVER_REQUIRED = yes

RGB_MATRIX_ENABLE = yes
RGB_MATRIX_DRIVER = custom
SRC += rgb_multipin.c

RAW_ENABLE = yes
SRC += hid_commands.c

DYNAMIC_KEYMAP_ENABLE = yes

NKRO_ENABLE = yes
EXTRAKEY_ENABLE = yes
MOUSEKEY_ENABLE = no
CONSOLE_ENABLE = no
COMMAND_ENABLE = no

LTO_ENABLE = yes
OPT = 2

PICO_FLASH_SIZE_BYTES = 2097152
