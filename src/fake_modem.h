#pragma once

// ============================================================
// fake_modem.h
//
// Public interface for the fake modem module.
// The fake modem runs on UART2 and pretends to be an external
// modem device. It listens for AT commands and sends back
// canned responses (like "OK" or "ERROR").
//
// UART2 pins are defined here so both the fake modem source
// and the diagram wiring reference the same GPIOs.
// ============================================================

// --- UART2 pin assignments (fake modem side) ----------------
// These GPIOs represent the "modem's" physical UART pins.
// In the Wokwi diagram they are wired back to UART1's pins
// on the same ESP32 board (loopback).
//
// UART2 TX (fake modem transmits data to ESP UART1 RX):
#define FAKE_MODEM_TX_PIN  10
// UART2 RX (fake modem receives data from ESP UART1 TX):
#define FAKE_MODEM_RX_PIN  9
// UART2 RTS (fake modem signals "I am ready to receive"):
#define FAKE_MODEM_RTS_PIN 12
// UART2 CTS (fake modem checks "is ESP ready to receive"):
#define FAKE_MODEM_CTS_PIN 11

// --- UART2 configuration ------------------------------------
// Which hardware UART peripheral to use (ESP32-S3 has 0, 1, 2):
#define FAKE_MODEM_UART_NUM  2
// Baud rate must match UART1 on the driver side:
#define FAKE_MODEM_BAUD      115200

// --- RX buffer size -----------------------------------------
// How many bytes the UART2 driver can buffer before data is lost.
// 256 is plenty for short AT commands like "AT\r\n":
#define FAKE_MODEM_RX_BUF    256

// --- Public function ----------------------------------------

// fake_modem_start()
//
// Call this once from app_main() before sending any AT commands.
//
// What it does:
//   1. Configures UART2 with the pins and baud rate above.
//   2. Enables hardware flow control (RTS/CTS) on UART2.
//   3. Installs the UART2 driver with an RX ring buffer.
//   4. Creates a FreeRTOS task that loops forever, reading
//      bytes from UART2 RX, looking for complete lines,
//      and writing back AT responses on UART2 TX.
//
// Inputs:  none
// Outputs: none (the task runs in the background)
void fake_modem_start(void);
