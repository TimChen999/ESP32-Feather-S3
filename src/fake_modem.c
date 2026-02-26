// ============================================================
// fake_modem.c
//
// Simulates an external modem device on UART2.
// Runs as a FreeRTOS background task.
//
// Behavior:
//   - Reads bytes from UART2 RX one at a time.
//   - Accumulates bytes into a line buffer until '\r' or '\n'.
//   - When a complete line arrives, checks if it matches
//     a known AT command.
//   - Sends back a canned response on UART2 TX.
//
// Supported commands (case-sensitive):
//   "AT"       -> responds "\r\nOK\r\n"
//   "AT+CSQ"   -> responds "\r\n+CSQ: 20,99\r\nOK\r\n"
//   anything   -> responds "\r\nERROR\r\n"
// ============================================================

#include "fake_modem.h"

// stdio.h: printf() for debug logging to UART0 console.
#include <stdio.h>

// string.h: strcmp() to match AT commands, memset() to clear buffers.
#include <string.h>

// FreeRTOS headers for creating the background task.
// FreeRTOS.h must come before task.h.
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// ESP-IDF UART driver API.
// Provides uart_config_t, uart_param_config(), uart_set_pin(),
// uart_driver_install(), uart_read_bytes(), uart_write_bytes().
#include "driver/uart.h"

// ESP-IDF logging tag — used in printf statements so you can
// tell which module is printing.
static const char *TAG = "fake_modem";

// --- Line buffer --------------------------------------------
// Holds the incoming AT command as bytes arrive one at a time.
// 128 bytes is enough for any realistic AT command string.
#define LINE_BUF_SIZE 128

// ============================================================
// send_response()
//
// Writes a string out on UART2 TX (back toward the ESP UART1 RX).
//
// Inputs:
//   response — null-terminated C string to send (e.g. "\r\nOK\r\n")
//
// Outputs:
//   Bytes are queued into the UART2 TX FIFO. The UART hardware
//   shifts them out at FAKE_MODEM_BAUD. If hardware flow control
//   is active, transmission pauses when CTS is deasserted.
// ============================================================
static void send_response(const char *response) {
    // strlen(response) gives the number of bytes to write.
    // uart_write_bytes() blocks until all bytes are in the TX FIFO.
    uart_write_bytes(FAKE_MODEM_UART_NUM, response, strlen(response));
}

// ============================================================
// process_line()
//
// Takes one complete AT command line (without trailing \r\n)
// and sends back the appropriate modem response.
//
// Inputs:
//   line — null-terminated string, e.g. "AT" or "AT+CSQ"
//
// Outputs:
//   Calls send_response() to write back on UART2 TX.
//   Also printf()s to the console (UART0) for debug visibility.
// ============================================================
static void process_line(const char *line) {
    // Log what the fake modem received (shows up in serial monitor / Wokwi console).
    printf("[%s] received: \"%s\"\n", TAG, line);

    // Match known AT commands and send canned responses.
    if (strcmp(line, "AT") == 0) {
        // "AT" is the basic attention command.
        // A real modem responds with "OK" to confirm it's alive.
        send_response("\r\nOK\r\n");

    } else if (strcmp(line, "AT+CSQ") == 0) {
        // "AT+CSQ" queries signal quality.
        // Response format: +CSQ: <rssi>,<ber>
        //   rssi=20 is a moderate signal (~-73 dBm).
        //   ber=99 means "not known or not detectable".
        send_response("\r\n+CSQ: 20,99\r\nOK\r\n");

    } else {
        // Any unrecognized command gets a generic ERROR.
        send_response("\r\nERROR\r\n");
    }
}

// ============================================================
// fake_modem_task()
//
// FreeRTOS task function. Runs forever in the background.
//
// Loop behavior:
//   1. Attempt to read 1 byte from UART2 RX with a 100ms timeout.
//   2. If a byte arrived:
//      a. If it's '\r' or '\n' and the line buffer is non-empty,
//         null-terminate the buffer and call process_line().
//      b. Otherwise append the byte to the line buffer.
//      c. If the buffer is full, reset it (drop the overlong line).
//   3. If no byte arrived (timeout), loop back and try again.
//
// Inputs:
//   arg — unused (required by FreeRTOS task signature)
//
// Outputs:
//   Sends AT responses via send_response() → UART2 TX.
// ============================================================
static void fake_modem_task(void *arg) {
    // Line buffer to accumulate incoming bytes.
    char line_buf[LINE_BUF_SIZE];

    // Current write position in line_buf (0 = empty).
    int line_pos = 0;

    // Temporary buffer for reading one byte at a time from UART2.
    uint8_t byte;

    while (1) {
        // Try to read exactly 1 byte from UART2 RX.
        // Timeout is 100ms (expressed in FreeRTOS ticks).
        // Returns the number of bytes actually read (0 or 1).
        int n = uart_read_bytes(FAKE_MODEM_UART_NUM, &byte, 1,
                                pdMS_TO_TICKS(100));

        // If no byte was received within 100ms, loop back.
        if (n <= 0) {
            continue;
        }

        // Check if this byte is a line terminator (\r or \n).
        if (byte == '\r' || byte == '\n') {
            // Only process if we have accumulated at least one character.
            // This avoids processing empty lines from \r\n pairs.
            if (line_pos > 0) {
                // Null-terminate the buffer to make it a valid C string.
                line_buf[line_pos] = '\0';

                // Hand the complete command to the response logic.
                process_line(line_buf);

                // Reset the buffer position for the next command.
                line_pos = 0;
            }
        } else {
            // Not a line terminator — append this byte to the buffer.
            line_buf[line_pos] = (char)byte;
            line_pos++;

            // Safety: if buffer is full, discard the whole line.
            // This prevents writing past the end of line_buf.
            if (line_pos >= LINE_BUF_SIZE - 1) {
                printf("[%s] line too long, discarding\n", TAG);
                line_pos = 0;
            }
        }
    }
}

// ============================================================
// fake_modem_start()
//
// Public entry point. Called once from app_main().
//
// Steps:
//   1. Fill a uart_config_t struct with baud rate, word format,
//      and hardware flow control mode.
//   2. Apply that config to UART2 via uart_param_config().
//   3. Assign physical GPIO pins to UART2's TX, RX, RTS, CTS
//      signals via uart_set_pin().
//   4. Install the UART2 driver with an RX ring buffer so
//      incoming bytes are queued even if the task is busy.
//   5. Create the FreeRTOS background task that reads and
//      responds to AT commands.
//
// Inputs:  none
// Outputs: none (task runs in background after this returns)
// ============================================================
void fake_modem_start(void) {
    // --- Step 1: UART2 configuration struct ---
    // This struct tells the UART peripheral how to operate.
    uart_config_t uart_cfg = {
        // Baud rate: must match the other side (UART1 in main.c).
        .baud_rate = FAKE_MODEM_BAUD,

        // 8 data bits per frame (standard for AT commands).
        .data_bits = UART_DATA_8_BITS,

        // No parity bit (standard for most serial devices).
        .parity = UART_PARITY_DISABLE,

        // 1 stop bit per frame.
        .stop_bits = UART_STOP_BITS_1,

        // Enable hardware (CTS/RTS) flow control.
        // The UART peripheral will:
        //   - Assert RTS when the RX FIFO has room (telling sender "go ahead").
        //   - Check CTS before transmitting (pausing if receiver says "wait").
        .flow_ctrl = UART_HW_FLOWCTRL_CTS_RTS,

        // RX FIFO threshold for RTS assertion.
        // When the FIFO has more than this many bytes, RTS is deasserted
        // (tells the sender to stop). 122 of 128 is the ESP-IDF default.
        .rx_flow_ctrl_thresh = 122,

        // Use the default reference clock.
        .source_clk = UART_SCLK_DEFAULT,
    };

    // --- Step 2: Apply config to UART2 hardware ---
    // This programs the baud rate divisor, word format, and
    // flow control registers inside the ESP32-S3's UART2 peripheral.
    uart_param_config(FAKE_MODEM_UART_NUM, &uart_cfg);

    // --- Step 3: Assign GPIO pins to UART2 signals ---
    // Maps each UART2 signal to a physical GPIO pin.
    // Order: TX pin, RX pin, RTS pin, CTS pin.
    uart_set_pin(FAKE_MODEM_UART_NUM,
                 FAKE_MODEM_TX_PIN,   // UART2 TX  -> GPIO10
                 FAKE_MODEM_RX_PIN,   // UART2 RX  -> GPIO9
                 FAKE_MODEM_RTS_PIN,  // UART2 RTS -> GPIO12
                 FAKE_MODEM_CTS_PIN); // UART2 CTS -> GPIO11

    // --- Step 4: Install UART2 driver ---
    // Allocates an RX ring buffer of FAKE_MODEM_RX_BUF bytes.
    // TX buffer size = 0 means uart_write_bytes() blocks until
    // all bytes are pushed into the hardware FIFO.
    // No event queue (NULL), no interrupt alloc flags (0).
    uart_driver_install(FAKE_MODEM_UART_NUM,
                        FAKE_MODEM_RX_BUF,  // RX ring buffer size
                        0,                   // TX buffer size (0 = no SW buffer)
                        0,                   // Event queue size (0 = disabled)
                        NULL,                // Event queue handle (not used)
                        0);                  // Interrupt alloc flags

    // --- Step 5: Launch the background task ---
    // Creates a FreeRTOS task that loops forever reading UART2.
    // Stack size: 4096 bytes (enough for the line buffer + UART calls).
    // Priority: 5 (moderate; higher than idle, lower than critical tasks).
    // Task handle: NULL (we don't need to reference it later).
    printf("[%s] starting fake modem task on UART%d\n", TAG, FAKE_MODEM_UART_NUM);
    xTaskCreate(fake_modem_task,   // Task function pointer
                "fake_modem",      // Task name (for debug/monitoring)
                4096,              // Stack size in bytes
                NULL,              // Argument passed to task (unused)
                5,                 // Task priority
                NULL);             // Task handle output (not needed)
}
