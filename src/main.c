#include <stdio.h>
#include <string.h>

#include "version.h"
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "hardware/gpio.h"
#include "bsp/board.h"
#include "tusb.h"
#include "uart_tx.pio.h"
#include "uart_rx.pio.h"

// ============================================================================
// Configuration
// ============================================================================

/// Default UART baud rate
#define UART_BAUD_DEFAULT     115200
/// LED blink interval in milliseconds
#define LED_BLINK_INTERVAL_MS 500
/// Management console buffer size
#define MGMT_BUF_SIZE         128
/// CDC read buffer size
#define CDC_READ_BUF_SIZE     64

/// UART pin definitions
#define UART0_ID       uart0
#define UART0_TX_PIN   0
#define UART0_RX_PIN   1

#define UART1_ID       uart1
#define UART1_TX_PIN   4
#define UART1_RX_PIN   5

#define UART2_TX_PIN   8
#define UART2_RX_PIN   9

#define UART3_TX_PIN   12
#define UART3_RX_PIN   13

/// USB CDC interface definitions
#define NUM_DATA_PORTS 4  ///< Number of data ports (0-3)
#define MGMT_ITF       4  ///< Management interface number

/// LED pin definition
#ifndef LED_PIN
#define LED_PIN PICO_DEFAULT_LED_PIN
#endif

/**
 * @brief Port type enumeration
 */
typedef enum {
    PORT_HW_UART,  ///< Hardware UART port
    PORT_PIO_UART, ///< PIO-based UART port
} port_type_t;

/**
 * @brief UART port configuration structure
 */
typedef struct {
    port_type_t type;      ///< Port type (hardware or PIO)
    uint32_t    baud;      ///< Baud rate
    uint8_t     parity;    ///< Parity: 0 = none, 1 = odd, 2 = even
    uint8_t     stop_bits; ///< Stop bits: 0 = 1 stop bit, 2 = 2 stop bits (as in CDC line coding)
    union {
        struct {
            uart_inst_t *uart; ///< Hardware UART instance
        } hw;
        struct {
            PIO pio;      ///< PIO instance
            uint tx_sm;   ///< TX state machine number
            uint rx_sm;   ///< RX state machine number
            uint tx_pin;  ///< TX pin (stored for reconfiguration)
            uint rx_pin;  ///< RX pin (stored for reconfiguration)
        } pio;
    };
} port_t;

/// Array of UART ports
static port_t ports[NUM_DATA_PORTS];

/// Management console input buffer
static char mgmt_buf[MGMT_BUF_SIZE];
/// Management console buffer length
static size_t mgmt_len = 0;

// ============================================================================
// UART configuration
// ============================================================================

/**
 * @brief Apply configuration to hardware UART port
 * @param p Pointer to port structure
 */
static void apply_hw_uart_cfg(port_t *p)
{
    uart_inst_t *u = p->hw.uart;
    uart_init(u, p->baud);
    
    // Configure parity
    uart_parity_t parity = UART_PARITY_NONE;
    if (p->parity == 1) parity = UART_PARITY_ODD;
    else if (p->parity == 2) parity = UART_PARITY_EVEN;

    uint data_bits = 8;
    uint stop_bits = (p->stop_bits == 2) ? 2 : 1;
    uart_set_format(u, data_bits, stop_bits, parity);
    uart_set_fifo_enabled(u, true);
}

/**
 * @brief Apply configuration to PIO UART port
 * @param p Pointer to port structure
 * @note For PIO UART, we simply reinitialize the state machines to change baud/format
 */
static void apply_pio_uart_cfg(port_t *p)
{
    PIO pio = p->pio.pio;
    uint tx_sm = p->pio.tx_sm;
    uint rx_sm = p->pio.rx_sm;
    uart_tx_program_init(pio, tx_sm, p->pio.tx_pin, p->baud);
    uart_rx_program_init(pio, rx_sm, p->pio.rx_pin, p->baud);
}

// ============================================================================
// Port initialization
// ============================================================================

/**
 * @brief Initialize hardware UART port
 * @param p Pointer to port structure
 * @param uart UART instance (uart0 or uart1)
 * @param tx_pin TX GPIO pin
 * @param rx_pin RX GPIO pin
 * @param baud Baud rate
 */
static void init_hw_uart(port_t *p, uart_inst_t *uart,
                         uint tx_pin, uint rx_pin, uint baud)
{
    p->type = PORT_HW_UART;
    p->baud = baud;
    p->parity = 0;    // none
    p->stop_bits = 0; // 1 stop bit

    p->hw.uart = uart;

    gpio_set_function(tx_pin, GPIO_FUNC_UART);
    gpio_set_function(rx_pin, GPIO_FUNC_UART);
    apply_hw_uart_cfg(p);
}

/**
 * @brief Initialize PIO UART port
 * @param p Pointer to port structure
 * @param pio PIO instance (pio0 or pio1)
 * @param tx_pin TX GPIO pin
 * @param rx_pin RX GPIO pin
 * @param baud Baud rate
 * @param tx_sm TX state machine number
 * @param rx_sm RX state machine number
 */
static void init_pio_uart(port_t *p, PIO pio,
                          uint tx_pin, uint rx_pin, uint baud,
                          uint tx_sm, uint rx_sm)
{
    p->type = PORT_PIO_UART;
    p->baud = baud;
    p->parity = 0;
    p->stop_bits = 0;

    p->pio.pio = pio;
    p->pio.tx_sm = tx_sm;
    p->pio.rx_sm = rx_sm;
    p->pio.tx_pin = tx_pin;
    p->pio.rx_pin = rx_pin;

    apply_pio_uart_cfg(p);
}

// ============================================================================
// Port I/O operations
// ============================================================================

/**
 * @brief Write data to UART port
 * @param p Pointer to port structure
 * @param data Data buffer to write
 * @param len Number of bytes to write
 * @note This is a blocking operation
 */
static void port_write(port_t *p, const uint8_t *data, size_t len)
{
    if (p->type == PORT_HW_UART) {
        uart_write_blocking(p->hw.uart, data, len);
    } else {
        for (size_t i = 0; i < len; i++) {
            uart_tx_putc(p->pio.pio, p->pio.tx_sm, data[i]);
        }
    }
}

/**
 * @brief Read a single byte from UART port
 * @param p Pointer to port structure
 * @param out Pointer to output byte
 * @return true if byte was read, false if no data available
 */
static bool port_read_byte(port_t *p, uint8_t *out)
{
    if (p->type == PORT_HW_UART) {
        if (!uart_is_readable(p->hw.uart)) return false;
        *out = (uint8_t)uart_getc(p->hw.uart);
        return true;
    } else {
        if (pio_sm_is_rx_fifo_empty(p->pio.pio, p->pio.rx_sm)) return false;
        *out = uart_rx_getc(p->pio.pio, p->pio.rx_sm);
        return true;
    }
}

// ============================================================================
// Management console
// ============================================================================

/**
 * @brief Write string to management console
 * @param s Null-terminated string to write
 */
static void mgmt_write_str(const char *s) {
    if (!tud_cdc_n_connected(MGMT_ITF)) return;
    tud_cdc_n_write(MGMT_ITF, s, strlen(s));
    tud_cdc_n_write_flush(MGMT_ITF);
}

/**
 * @brief Display management console prompt
 */
static void mgmt_prompt(void) {
    mgmt_write_str("> ");
}  

/**
 * @brief Handle management console command line
 * @param line Command line string (will be modified)
 */
static void handle_mgmt_line(char *line) {
    // Remove trailing whitespace and newlines
    size_t len = strlen(line);
    while (len && (line[len-1] == '\r' || line[len-1] == '\n' || line[len-1] == ' ')) {
        line[--len] = '\0';
    }
    
    if (!len) {
        mgmt_prompt();
        return;
    }

    if (strcmp(line, "help") == 0) {
        mgmt_write_str("Available commands:\r\n");
        mgmt_write_str("  help                      - show this help\r\n");
        mgmt_write_str("  show                      - show ports configuration\r\n");
        mgmt_write_str("  set <port> baud=<v> parity=<0|1|2> stop=<1|2>\r\n");
        mgmt_write_str("                            - configure UART port\r\n");
        mgmt_write_str("\r\n");
        mgmt_prompt();
        return;
    }

    if (strcmp(line, "show") == 0) {
        mgmt_write_str("Ports:\r\n");
        for (int i = 0; i < NUM_DATA_PORTS; i++) {
            char buf[128];
            snprintf(buf, sizeof(buf),
                     "  port %d: baud=%lu parity=%u stop=%u type=%s\r\n",
                     i,
                     (unsigned long)ports[i].baud,
                     ports[i].parity,
                     ports[i].stop_bits,
                     (ports[i].type == PORT_HW_UART) ? "HW" : "PIO");
            mgmt_write_str(buf);
        }
        mgmt_prompt();
        return;
    }

    if (strncmp(line, "set ", 4) == 0) {
        int port = -1;
        unsigned baud = 0;
        unsigned parity = 0;
        unsigned stop = 1;

        if (sscanf(line, "set %d baud=%u parity=%u stop=%u",
                   &port, &baud, &parity, &stop) >= 2) {

            if (port < 0 || port >= NUM_DATA_PORTS) {
                mgmt_write_str("ERR: invalid port\r\n");
                mgmt_prompt();
                return;
            }

            // Validate parameters
            if (baud == 0) baud = UART_BAUD_DEFAULT;
            if (parity > 2) parity = 0;
            if (stop != 2) stop = 1;

            // Apply settings
            ports[port].baud = baud;
            ports[port].parity = (uint8_t)parity;
            ports[port].stop_bits = (uint8_t)stop;

            // Apply configuration to UART
            if (ports[port].type == PORT_HW_UART) {
                apply_hw_uart_cfg(&ports[port]);
            } else {
                apply_pio_uart_cfg(&ports[port]);
            }

            mgmt_write_str("OK\r\n");
            mgmt_prompt();
        } else {
            mgmt_write_str("ERR: usage: set <port> baud=<v> parity=<0|1|2> stop=<1|2>\r\n");
            mgmt_prompt();
        }
        return;
    }

    mgmt_write_str("ERR: unknown command\r\n");
    mgmt_prompt();
}

// ============================================================================
// TinyUSB callbacks
// ============================================================================

/**
 * @brief TinyUSB CDC RX callback
 * @param itf Interface number
 * @note Called when data is received on a CDC interface
 */
void tud_cdc_rx_cb(uint8_t itf)
{
    if (itf == MGMT_ITF) {
        uint8_t buf[CDC_READ_BUF_SIZE];
        uint32_t count = tud_cdc_n_read(itf, buf, sizeof(buf));
        if (!count) return;

        for (uint32_t i = 0; i < count; i++) {
            char c = (char)buf[i];

            // Enter (CR/LF) -> complete command
            if (c == '\r' || c == '\n') {
                // Print CRLF
                tud_cdc_n_write(MGMT_ITF, "\r\n", 2);

                if (mgmt_len > 0) {
                    mgmt_buf[mgmt_len] = '\0';
                    handle_mgmt_line(mgmt_buf);
                    mgmt_len = 0;
                } else {
                    // Empty line -> just prompt
                    mgmt_prompt();
                }
                continue;
            }

            // Simple backspace handling
            if (c == '\b' || c == 0x7F) {
                if (mgmt_len > 0) {
                    mgmt_len--;
                    // Erase character on screen
                    tud_cdc_n_write(MGMT_ITF, "\b \b", 3);
                }
                continue;
            }

            // Normal printable character: add to buffer and echo
            if (mgmt_len < sizeof(mgmt_buf) - 1) {
                mgmt_buf[mgmt_len++] = c;
                tud_cdc_n_write_char(MGMT_ITF, c);
            }
        }

        tud_cdc_n_write_flush(MGMT_ITF);
        return;
    }

    if (itf < NUM_DATA_PORTS) {
        // Data ports: CDCn -> UARTn (no echo)
        uint8_t buf[CDC_READ_BUF_SIZE];
        uint32_t count = tud_cdc_n_read(itf, buf, sizeof(buf));
        if (!count) return;

        port_write(&ports[itf], buf, count);
    }
}

/**
 * @brief TinyUSB CDC line coding callback
 * @param itf Interface number
 * @param p_line_coding Pointer to line coding structure
 * @note Called when host changes line coding (baud rate, parity, etc.)
 */
void tud_cdc_line_coding_cb(uint8_t itf, cdc_line_coding_t const* p_line_coding)
{
    if (itf >= NUM_DATA_PORTS) {
        return; // Management or other interface - ignore
    }

    port_t *p = &ports[itf];

    // Update parameters from line coding
    p->baud = p_line_coding->bit_rate;
    p->stop_bits = (p_line_coding->stop_bits == 2) ? 2 : 1;

    // Parity: 0 = none, 1 = odd, 2 = even
    uint8_t parity = 0;
    if (p_line_coding->parity == 1) parity = 1;
    else if (p_line_coding->parity == 2) parity = 2;
    p->parity = parity;

    // Apply configuration
    if (p->type == PORT_HW_UART) {
        apply_hw_uart_cfg(p);
    } else {
        apply_pio_uart_cfg(p);
    }
}

// ============================================================================
// Main
// ============================================================================

/**
 * @brief Main entry point
 * @return Always returns 0 (never reached)
 */
int main(void)
{
    // Initialize USB
    board_init();
    tusb_init();

    // Initialize LED
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    gpio_put(LED_PIN, 0);

    // Give USB time to initialize
    sleep_ms(500);
    mgmt_write_str("Management console ready\r\n");
    mgmt_prompt();

    // Initialize hardware UART ports
    init_hw_uart(&ports[0], UART0_ID, UART0_TX_PIN, UART0_RX_PIN, UART_BAUD_DEFAULT);
    init_hw_uart(&ports[1], UART1_ID, UART1_TX_PIN, UART1_RX_PIN, UART_BAUD_DEFAULT);

    // Initialize PIO UART ports
    PIO pio = pio0;
    uint tx_sm2 = pio_claim_unused_sm(pio, true);
    uint rx_sm2 = pio_claim_unused_sm(pio, true);
    init_pio_uart(&ports[2], pio, UART2_TX_PIN, UART2_RX_PIN, UART_BAUD_DEFAULT,
                  tx_sm2, rx_sm2);

    uint tx_sm3 = pio_claim_unused_sm(pio, true);
    uint rx_sm3 = pio_claim_unused_sm(pio, true);
    init_pio_uart(&ports[3], pio, UART3_TX_PIN, UART3_RX_PIN, UART_BAUD_DEFAULT,
                  tx_sm3, rx_sm3);

    // Heartbeat LED variables
    absolute_time_t led_next_toggle = make_timeout_time_ms(LED_BLINK_INTERVAL_MS);
    bool led_state = false;

    // Main loop
    while (1) {
        // Process USB events
        tud_task();

        // UART -> USB CDC (only for connected ports)
        for (uint8_t i = 0; i < NUM_DATA_PORTS; i++) {
            if (!tud_cdc_n_connected(i)) continue;

            uint8_t ch;
            bool any = false;
            while (port_read_byte(&ports[i], &ch)) {
                tud_cdc_n_write_char(i, ch);
                any = true;
            }
            if (any) {
                tud_cdc_n_write_flush(i);
            }
        }

        // Heartbeat LED
        if (absolute_time_diff_us(get_absolute_time(), led_next_toggle) < 0) {
            led_state = !led_state;
            gpio_put(LED_PIN, led_state);
            led_next_toggle = make_timeout_time_ms(LED_BLINK_INTERVAL_MS);
        }
    }

    return 0;
}
