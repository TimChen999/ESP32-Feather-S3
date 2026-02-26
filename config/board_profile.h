#pragma once

// Select one board profile:
//   - BOARD_XIAO_ESP32S3
//   - BOARD_FEATHER_ESP32S3
//   - BOARD_METRO_ESP32S3
#define BOARD_FEATHER_ESP32S3

#if defined(BOARD_XIAO_ESP32S3) && defined(BOARD_FEATHER_ESP32S3)
#error "Select only one board profile."
#endif
#if defined(BOARD_XIAO_ESP32S3) && defined(BOARD_METRO_ESP32S3)
#error "Select only one board profile."
#endif
#if defined(BOARD_FEATHER_ESP32S3) && defined(BOARD_METRO_ESP32S3)
#error "Select only one board profile."
#endif

#if defined(BOARD_XIAO_ESP32S3)
#include "board_xiao_esp32s3.h"
#elif defined(BOARD_FEATHER_ESP32S3)
#include "board_feather_esp32s3.h"
#elif defined(BOARD_METRO_ESP32S3)
#include "board_metro_esp32s3.h"
#else
#error "No board profile selected in board_profile.h"
#endif
