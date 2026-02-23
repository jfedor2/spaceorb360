#include <stdio.h>

#include <bsp/board.h>
#include <tusb.h>

#include <hardware/gpio.h>
#include <hardware/uart.h>
#include <pico/stdio.h>

#define SERIAL_MOUSE_RX_PIN 21
#define SERIAL_MOUSE_TX_PIN 20
#define SERIAL_MOUSE_DTR_PIN 16
#define SERIAL_MOUSE_RTS_PIN 27
#define SERIAL_MOUSE_UART uart1

#ifdef SPACEMOUSE
uint16_t trans_report[3];
uint16_t rot_report[3];
uint8_t buttons_report[6];
#endif

#ifdef JOYSTICK
uint16_t report[7];
#endif

#ifdef SPACEMOUSE
uint8_t trans_pending = 0;
uint8_t rot_pending = 0;
uint8_t buttons_pending = 0;
#endif

#ifdef JOYSTICK
uint8_t pending = 0;
#endif

#ifdef SPACEMOUSE
uint8_t button_bits[] = { 12, 13, 14, 15, 25, 23, 24, 22, 0, 1, 2, 4, 5, 8, 26 };
#endif

#ifdef JOYSTICK
uint8_t button_bits[] = { 0, 1, 2, 3, 4, 5, 6 };
#endif

char xor_key[] = "SpaceWare!";

void handle_buttons(uint8_t buttons) {
#ifdef SPACEMOUSE
    memset(buttons_report, 0, sizeof(buttons_report));
#endif

#ifdef JOYSTICK
    report[6] = 0;
#endif

    for (int i = 0; i < 7; i++) {
        if (buttons & (1 << i)) {
#ifdef SPACEMOUSE
            buttons_report[button_bits[i] / 8] |= 1 << (button_bits[i] % 8);
#endif

#ifdef JOYSTICK
            report[6] |= 1 << button_bits[i];
#endif
        }
    }

#ifdef SPACEMOUSE
    buttons_pending = 1;
#endif

#ifdef JOYSTICK
    pending = 1;
#endif
}

int main() {
    board_init();
    tusb_init();
    stdio_init_all();

    printf("hello\n");

    // set these lines to low manually to provide power to the SpaceOrb
    gpio_init(SERIAL_MOUSE_DTR_PIN);
    gpio_set_dir(SERIAL_MOUSE_DTR_PIN, GPIO_OUT);
    gpio_put(SERIAL_MOUSE_DTR_PIN, false);
    gpio_init(SERIAL_MOUSE_RTS_PIN);
    gpio_set_dir(SERIAL_MOUSE_RTS_PIN, GPIO_OUT);
    gpio_put(SERIAL_MOUSE_RTS_PIN, false);

    gpio_set_function(SERIAL_MOUSE_RX_PIN, GPIO_FUNC_UART);
    gpio_set_function(SERIAL_MOUSE_TX_PIN, GPIO_FUNC_UART);
    uart_init(SERIAL_MOUSE_UART, 9600);
    uart_set_translate_crlf(SERIAL_MOUSE_UART, false);
    uart_set_format(SERIAL_MOUSE_UART, 8, 1, UART_PARITY_NONE);

    uint8_t buf[64];
    uint8_t idx = 0;

    while (true) {
        tud_task();
        if (tud_hid_ready()) {
#ifdef SPACEMOUSE
            if (trans_pending) {
                tud_hid_report(1, trans_report, 6);
                trans_pending = 0;
            } else if (rot_pending) {
                tud_hid_report(2, rot_report, 6);
                rot_pending = 0;
            } else if (buttons_pending) {
                tud_hid_report(3, buttons_report, 6);
                buttons_pending = 0;
            }
#endif

#ifdef JOYSTICK
            if (pending) {
                tud_hid_report(0, report, sizeof(report));
                pending = 0;
            }
#endif
        }

        if (uart_is_readable(SERIAL_MOUSE_UART)) {
            char c = uart_getc(SERIAL_MOUSE_UART);
            buf[idx++] = c;
            idx %= sizeof(buf);

            if ((buf[0] == 'D') && (idx == 12)) {
                for (int i = 0; i < 10; i++) {
                    buf[i + 2] ^= xor_key[i];
                    buf[i + 2] &= 0x7F;
                }
                int16_t values[6];
                values[0] = ((buf[2] & 0b01111111) << 3) |
                            ((buf[3] & 0b01110000) >> 4);
                values[1] = ((buf[3] & 0b00001111) << 6) |
                            ((buf[4] & 0b01111110) >> 1);
                values[2] = ((buf[4] & 0b00000001) << 9) |
                            ((buf[5] & 0b01111111) << 2) |
                            ((buf[6] & 0b01100000) >> 5);
                values[3] = ((buf[6] & 0b00011111) << 5) |
                            ((buf[7] & 0b01111100) >> 2);
                values[4] = ((buf[7] & 0b00000011) << 8) |
                            ((buf[8] & 0b01111111) << 1) |
                            ((buf[9] & 0b01000000) >> 6);
                values[5] = ((buf[9] & 0b00111111) << 4) |
                            ((buf[10] & 0b01111000) >> 3);

                for (int i = 0; i < 6; i++) {
                    values[i] <<= 6;
                    values[i] >>= 6;
                }

#ifdef SPACEMOUSE
                trans_report[0] = values[0];
                trans_report[1] = values[2];
                trans_report[2] = -values[1];
                rot_report[0] = values[3];
                rot_report[1] = values[5];
                rot_report[2] = -values[4];

                trans_pending = 1;
                rot_pending = 1;
#endif

#ifdef JOYSTICK
                report[0] = values[0];
                report[1] = values[2];
                report[2] = -values[1];
                report[3] = values[3];
                report[4] = values[5];
                report[5] = -values[4];

                pending = 1;
#endif

                handle_buttons(buf[1] & 0x7F);
                idx = 0;
            }

            if ((buf[0] == 'K') && (idx == 5)) {
                handle_buttons(buf[2] & 0x7F);
                idx = 0;
            }

            printf("%02x ", c);
            if (c == '\r') {
                printf("\n");
                idx = 0;
            }
        }
    }

    return 0;
}

void tud_hid_set_report_cb(uint8_t itf, uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize) {
}

uint16_t tud_hid_get_report_cb(uint8_t itf, uint8_t report_id, hid_report_type_t report_type, uint8_t* buffer, uint16_t reqlen) {
    return 0;
}
