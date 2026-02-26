#pragma once

// NOTE:
// - Verify all GPIO mappings against your exact Feather ESP32-S3 variant docs.
// - Keep this file focused on board-specific hardware facts only.

#define BOARD_NAME "Adafruit Feather ESP32-S3"

// Capabilities
#define HAS_PSRAM 1
#define MAX_PSRAM_MB 2
#define HAS_CAMERA 0
#define HAS_NATIVE_USB 1
#define HAS_BATTERY_CHARGER 1
#define HAS_RGB_LED 1
#define HAS_SD 0

// Sentinel for unavailable pins in this profile.
#define INVALID_PIN (-1)

// Core / debug mapping:
//   - Status LED signal -> GPIO13
//   - Boot button input -> GPIO0
//   - User button -> not mapped on this profile
#define PIN_LED_STATUS 13
#define PIN_BTN_BOOT 0
#define PIN_BTN_USER INVALID_PIN

// I2C mapping:
//   - SDA line -> GPIO3
//   - SCL line -> GPIO4
#define PIN_I2C_SDA 3
#define PIN_I2C_SCL 4
#define I2C_FREQ_HZ 400000

// SPI mapping (bus lines only, CS stays per device):
//   - SCK clock -> GPIO36
//   - MISO data in -> GPIO37
//   - MOSI data out -> GPIO35
#define PIN_SPI_SCK 36
#define PIN_SPI_MISO 37
#define PIN_SPI_MOSI 35
#define SPI_FREQ_HZ 8000000

// Modem UART mapping:
//   - UART1 TX (ESP -> modem RX) -> GPIO17
//   - UART1 RX (ESP <- modem TX) -> GPIO18
//   - RTS (ESP ready signal) -> GPIO16
//   - CTS (modem ready signal) -> GPIO15
#define UART_MODEM_NUM 1
#define PIN_MODEM_TX 17
#define PIN_MODEM_RX 18
#define PIN_MODEM_RTS 16
#define PIN_MODEM_CTS 15
#define MODEM_BAUD 115200
#define MODEM_USE_HWFC 1

// Optional I2S / audio mapping:
//   - Not assigned in this template profile
#define PIN_I2S_BCLK INVALID_PIN
#define PIN_I2S_WS INVALID_PIN
#define PIN_I2S_DOUT INVALID_PIN
#define PIN_I2S_DIN INVALID_PIN

// Reserved pins (documented constraints)
// - GPIO0 is a strap pin on ESP32-S3.
// - Keep USB/JTAG-capable pins free unless intentionally used.
