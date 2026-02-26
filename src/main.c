// ============================================================
// main.c
//
// Modem driver side of the UART loopback test.
// Runs on UART1 and sends AT commands to the fake modem
// (which runs on UART2 in fake_modem.c).
//
// Wiring (all on the same ESP32-S3 board, loopback):
//   UART1 TX  (GPIO17) ---wire---> UART2 RX  (GPIO9)
//   UART2 TX  (GPIO10) ---wire---> UART1 RX  (GPIO18)
//   UART1 RTS (GPIO16) ---wire---> UART2 CTS (GPIO11)
//   UART2 RTS (GPIO12) ---wire---> UART1 CTS (GPIO15)
//
// Flow:
//   1. app_main() initializes UART1 with hardware flow control.
//   2. app_main() calls fake_modem_start() to launch the UART2 task.
//   3. app_main() loops: send an AT command on UART1 TX,
//      read the response on UART1 RX, print it, wait, repeat.
// ============================================================

// stdio.h: printf() for console logging to UART0.
#include <stdio.h>

// string.h: strlen() for measuring command strings before sending.
#include <string.h>

// FreeRTOS headers for vTaskDelay() (blocking delay in the main loop).
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// ESP-IDF UART driver API.
#include "driver/uart.h"

// Our fake modem module — provides fake_modem_start().
#include "fake_modem.h"

// ============================================================
// UART1 pin and config definitions (modem driver side)
//
// These match the board_feather_esp32s3.h modem UART mapping:
//   TX=GPIO17, RX=GPIO18, RTS=GPIO16, CTS=GPIO15
// ============================================================

// Which hardware UART peripheral for the modem driver side:
#define MODEM_UART_NUM  1

// GPIO pin for UART1 TX (ESP32 sends data to modem here):
#define MODEM_TX_PIN    17

// GPIO pin for UART1 RX (ESP32 receives data from modem here):
#define MODEM_RX_PIN    18

// GPIO pin for UART1 RTS (ESP32 tells modem "I can receive"):
#define MODEM_RTS_PIN   16

// GPIO pin for UART1 CTS (modem tells ESP32 "I can receive"):
#define MODEM_CTS_PIN   15

// Baud rate for UART1. Must match FAKE_MODEM_BAUD in fake_modem.h:
#define MODEM_BAUD      115200

// RX ring buffer size for the UART1 driver.
// 256 bytes is enough to hold modem responses like "+CSQ: 20,99\r\nOK\r\n":
#define MODEM_RX_BUF    256

// ============================================================
// send_at_command()
//
// Sends one AT command string on UART1 TX and reads back the
// response on UART1 RX.
//
// Inputs:
//   cmd — null-terminated AT command (e.g. "AT\r\n").
//         Must include the trailing \r\n that modems expect.
//
// Outputs:
//   Prints the raw response bytes to the console (UART0)
//   so you can see what the fake modem sent back.
// ============================================================
static void send_at_command(const char *cmd) {
    // --- Send the command on UART1 TX ---
    // strlen(cmd) gives byte count. uart_write_bytes() pushes them
    // into the UART1 TX FIFO. If CTS is deasserted (modem says "wait"),
    // the hardware pauses transmission automatically.
    printf("[main] sending: \"%.*s\"\n", (int)(strlen(cmd) - 2), cmd);
    uart_write_bytes(MODEM_UART_NUM, cmd, strlen(cmd));

    // --- Wait briefly for the fake modem to process and respond ---
    // 200ms is generous; real modems can take longer for some commands.
    vTaskDelay(pdMS_TO_TICKS(200));

    // --- Read response from UART1 RX ---
    // Temporary buffer to hold the response bytes.
    uint8_t rx_buf[128];

    // Try to read up to 127 bytes with a 300ms timeout.
    // Returns the number of bytes actually read (0 if nothing arrived).
    int len = uart_read_bytes(MODEM_UART_NUM, rx_buf, sizeof(rx_buf) - 1,
                              pdMS_TO_TICKS(300));

    if (len > 0) {
        // Null-terminate so we can print it as a C string.
        rx_buf[len] = '\0';

        // Print the raw response. \r\n from the modem will show as
        // line breaks in the console output.
        printf("[main] response (%d bytes): %s\n", len, (char *)rx_buf);
    } else {
        // No bytes received within the timeout.
        // Could mean: wiring issue, fake modem not running,
        // or flow control is blocking transmission.
        printf("[main] no response received\n");
    }
}

// ============================================================
// app_main()
//
// Entry point called by ESP-IDF after boot.
//
// Steps:
//   1. Configure UART1 (baud, word format, flow control).
//   2. Assign GPIO pins to UART1 signals.
//   3. Install UART1 driver with RX buffer.
//   4. Start the fake modem on UART2 (background task).
//   5. Loop forever: send AT commands, read responses, delay.
// ============================================================
void app_main(void) {
    printf("[main] UART loopback test starting\n");

    // --- Step 1: UART1 configuration struct ---
    // Same structure as fake_modem.c but for UART1.
    uart_config_t uart_cfg = {
        // Baud rate: 115200 (must match fake modem).
        .baud_rate = MODEM_BAUD,

        // 8 data bits, no parity, 1 stop bit (8N1).
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,

        // Enable hardware flow control on UART1.
        // UART1 will assert RTS when its RX FIFO has room.
        // UART1 will check CTS before sending each byte.
        .flow_ctrl = UART_HW_FLOWCTRL_CTS_RTS,

        // Deassert RTS when RX FIFO exceeds 122 bytes.
        .rx_flow_ctrl_thresh = 122,

        // Use default reference clock for baud rate generation.
        .source_clk = UART_SCLK_DEFAULT,
    };

    // --- Step 2: Apply config to UART1 hardware ---
    // Programs UART1's baud divisor, frame format, and flow control.
    uart_param_config(MODEM_UART_NUM, &uart_cfg);

    // --- Step 3: Assign GPIO pins to UART1 signals ---
    // Order: TX, RX, RTS, CTS.
    uart_set_pin(MODEM_UART_NUM,
                 MODEM_TX_PIN,   // UART1 TX  -> GPIO17
                 MODEM_RX_PIN,   // UART1 RX  -> GPIO18
                 MODEM_RTS_PIN,  // UART1 RTS -> GPIO16
                 MODEM_CTS_PIN); // UART1 CTS -> GPIO15

    // --- Step 4: Install UART1 driver ---
    // RX ring buffer = MODEM_RX_BUF bytes.
    // TX buffer = 0 (blocking writes directly to FIFO).
    // No event queue.
    uart_driver_install(MODEM_UART_NUM,
                        MODEM_RX_BUF,  // RX buffer size
                        0,             // TX buffer size
                        0,             // Event queue size
                        NULL,          // Event queue handle
                        0);            // Interrupt flags

    printf("[main] UART1 configured on TX=%d RX=%d RTS=%d CTS=%d\n",
           MODEM_TX_PIN, MODEM_RX_PIN, MODEM_RTS_PIN, MODEM_CTS_PIN);

    // --- Step 5: Start fake modem on UART2 ---
    // This configures UART2 and launches a background task.
    // After this call, the fake modem is listening on UART2 RX.
    fake_modem_start();

    // Give the fake modem task time to initialize.
    vTaskDelay(pdMS_TO_TICKS(100));

    printf("[main] sending AT commands...\n\n");

    // --- Step 6: Main loop — send commands, read responses ---
    while (1) {
        // Send basic "AT" command (modem alive check).
        // The \r\n at the end is the standard AT command terminator.
        send_at_command("AT\r\n");

        // Wait 2 seconds between commands.
        vTaskDelay(pdMS_TO_TICKS(2000));

        // Send "AT+CSQ" (signal quality query).
        send_at_command("AT+CSQ\r\n");

        // Wait 2 seconds before next round.
        vTaskDelay(pdMS_TO_TICKS(2000));

        // Send an unknown command to test ERROR handling.
        send_at_command("AT+UNKNOWN\r\n");

        // Wait 2 seconds before repeating the whole cycle.
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}
