#pragma once

#include "board_profile.h"

// =========================
// Feature toggles
// =========================
#define FEATURE_MODEM 1
#define FEATURE_TLS 1
#define FEATURE_SD_LOGGING 0
#define FEATURE_AUDIO 0
#define FEATURE_CAMERA 0
#define FEATURE_DISPLAY 0
#define FEATURE_OTA 1
#define FEATURE_DEEP_SLEEP 0

// =========================
// Memory and buffering knobs
// =========================
#if HAS_PSRAM && (MAX_PSRAM_MB >= 8)
#define RX_RING_BYTES 8192
#define TX_RING_BYTES 4096
#define HTTP_RX_MAX 16384
#define JSON_DOC_BYTES 16384
#else
#define RX_RING_BYTES 4096
#define TX_RING_BYTES 2048
#define HTTP_RX_MAX 8192
#define JSON_DOC_BYTES 6144
#endif

#define MODEM_LINE_MAX 512
#define AUDIO_FRAME_BYTES 1024
#define AUDIO_DMA_BUF_COUNT 6
#define AUDIO_DMA_BUF_LEN 256
#define CAM_FRAME_BYTES 0  // Keep 0 unless camera is enabled.

// =========================
// Timeouts and task sizing
// =========================
#define MODEM_CMD_TIMEOUT_MS 12000
#define MODEM_BOOT_GRACE_MS 8000
#define I2C_TIMEOUT_MS 100
#define NET_TASK_STACK_BYTES 8192
#define MODEM_TASK_STACK_BYTES 6144
#define WATCHDOG_ENABLE 1
#define WATCHDOG_TIMEOUT_S 10

// =========================
// Compile-time safety checks
// =========================
#if FEATURE_CAMERA && !HAS_CAMERA
#error "FEATURE_CAMERA is enabled but HAS_CAMERA is false for this board."
#endif

#if FEATURE_AUDIO
#if (PIN_I2S_BCLK < 0) || (PIN_I2S_WS < 0)
#error "FEATURE_AUDIO is enabled but I2S pins are not mapped in this board profile."
#endif
#endif
