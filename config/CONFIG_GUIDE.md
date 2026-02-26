# Config Guide: Multi-Board Setup (ESP32-S3)

This folder is designed to make board swaps mostly a config change.

Supported board profiles:
- Seeed XIAO ESP32S3
- Adafruit Feather ESP32-S3
- Adafruit Metro ESP32-S3

## File roles

- `board_profile.h`
  - Selects the active board profile.
  - Includes exactly one board definition file.
- `board_xiao_esp32s3.h`
- `board_feather_esp32s3.h`
- `board_metro_esp32s3.h`
  - Board-specific hardware facts:
    - capabilities (`HAS_PSRAM`, `MAX_PSRAM_MB`, etc.)
    - pin mappings (I2C/SPI/UART/I2S)
    - board identity (`BOARD_NAME`)
- `app_config.h`
  - Board-agnostic app behavior:
    - feature toggles
    - memory/buffer sizing
    - timeouts, stack sizes, watchdog settings
    - compile-time safety checks

## Quick start: switch boards

1. Open `board_profile.h`.
2. Keep only one board macro enabled:
   - `BOARD_XIAO_ESP32S3`
   - `BOARD_FEATHER_ESP32S3`
   - `BOARD_METRO_ESP32S3`
3. Build and run tests/simulation.
4. Validate pin mappings against your exact board docs.

## What must stay in board files

Keep all hardware-specific definitions in `board_*.h`:

- Capability flags
  - `HAS_PSRAM`
  - `MAX_PSRAM_MB`
  - `HAS_CAMERA`
  - `HAS_NATIVE_USB`
- Pin mappings by function (not raw GPIO numbers in app code)
  - `PIN_I2C_*`
  - `PIN_SPI_*`
  - `PIN_MODEM_TX/RX/RTS/CTS`
  - `PIN_I2S_*` (or `INVALID_PIN` if unused)

Rule: app code should never hardcode board GPIO values.

## What belongs in app_config.h

Use `app_config.h` for behavior and scaling knobs:

- Feature toggles
  - `FEATURE_MODEM`, `FEATURE_AUDIO`, `FEATURE_CAMERA`, `FEATURE_OTA`, etc.
- Memory and buffering
  - `RX_RING_BYTES`, `TX_RING_BYTES`, `HTTP_RX_MAX`, `JSON_DOC_BYTES`
- Timing and reliability
  - command timeouts, task stack sizes, watchdog settings

Use capability-aware defaults:
- Larger buffers on 8MB PSRAM boards
- Conservative buffers on smaller-memory boards

## Recommended coding pattern

1. Include `app_config.h` in firmware modules.
2. Use config macros instead of literals:
   - Good: `Serial1.begin(MODEM_BAUD, ... PIN_MODEM_RX, PIN_MODEM_TX);`
   - Avoid: `Serial1.begin(115200, ... 18, 17);`
3. Initialize hardware in one place (for example, `hal_init.cpp`).
4. Keep peripheral setup centralized (I2C/SPI/UART init and pin mode setup).

## Portability checklist (before changing boards)

1. Pin usage
   - Verify all mapped pins exist and are routable on target board.
   - Avoid strapping/boot-sensitive pins unless intentional.
2. Memory assumptions
   - Check PSRAM size difference (8MB -> 2MB can break large buffers).
3. Peripheral conflicts
   - Confirm no SPI/UART/I2C overlaps with onboard peripherals.
4. Feature compatibility
   - Ensure `FEATURE_CAMERA`/`FEATURE_AUDIO` align with available pins and hardware.
5. Toolchain/project settings
   - Re-check board selection in PlatformIO and any USB/PSRAM build options.

## Known practical guidance

- No camera + typical IoT sensors/networking:
  - Usually low-friction board swaps.
- Heavy buffering / media / camera:
  - Expect memory and pin-routing rework.
- UART modem with RTS/CTS:
  - Usually straightforward if modem pins stay centralized in board config.

## Maintenance tips

- Update this guide whenever you add:
  - new board profile
  - new capability macro
  - new peripheral pin contract
- Keep pin comments short and factual.
- If unsure about a pin, mark it `INVALID_PIN` until verified.
