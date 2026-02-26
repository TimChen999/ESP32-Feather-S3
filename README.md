# Wokwi Comms Periph

## Overview - what each tool is used for

- **PlatformIO** (firmware build and hardware workflow)
  - Creates and manages MCU projects.
  - Compiles source code into firmware artifacts (for example, `.elf` and `.bin`).
  - Uploads firmware to real boards and provides serial/debug tooling.
  - By itself, PlatformIO does not simulate peripherals.

- **Wokwi** (simulation workflow)
  - Simulates supported boards and peripherals using `diagram.json`.
  - Runs firmware built by PlatformIO so you can test behavior without wiring physical hardware.
  - Uses `wokwi.toml` to point to your compiled firmware artifact.

- **How they fit together**
  - Write firmware in PlatformIO.
  - Build once in PlatformIO to generate firmware output files.
  - Run that firmware in Wokwi for virtual board/peripheral testing.

## End-to-end setup (PlatformIO + Wokwi)

### Step 1 - Install required extensions

> **Editor recommendation:** use **VS Code** for the smoothest PlatformIO experience.  
> Cursor can work, but PlatformIO in Cursor may require extra fixes/workarounds depending on extension compatibility.

1. Open Extensions in Cursor/VS Code.
   - Windows/Linux: `Ctrl+Shift+X`
   - macOS: `Cmd+Shift+X`
2. Install **PlatformIO IDE**.
   - This provides project scaffolding, toolchains, build, and upload tasks.
3. Install **Wokwi for VS Code**.
   - This adds the Wokwi simulator commands and integration.

### Step 2 - Activate your Wokwi license

1. Open the Command Palette.
   - Windows/Linux: `Ctrl+Shift+P` or `F1`
   - macOS: `Cmd+Shift+P` or `F1`
2. Run **Wokwi: Request a new License**.
3. Confirm opening your browser.
4. On the Wokwi page, click **GET YOUR LICENSE**.
   - Sign in or create a Wokwi account if prompted.
5. Confirm the transfer back to Cursor/VS Code.
6. Verify success message:
   - **"License activated for [your name]"**

### Step 3 - Create a new PlatformIO project from scratch

1. Open the Command Palette and run **PlatformIO: New Project**.
2. Fill in project details.
   - **Name**: your project name (example: `wokwi-comms-periph`)
   - **Board**: the MCU board you want to simulate (example: ESP32 Dev Module)
   - **Framework**: Arduino / ESP-IDF / other supported framework
   - **Location**: folder where project should be created
3. Click **Finish/Create** and wait for PlatformIO setup.
   - First run can take a while because toolchains are downloaded.

### Step 4 - Add a minimal test firmware

1. Open `src/main.cpp` (or equivalent source file for your framework).
2. Add simple code to confirm build + simulation path is correct.
   - Typical test: blink LED or periodic serial print.
3. Save the file.

### Step 5 - Build once to generate firmware outputs

1. Run **PlatformIO: Build** (or use the PlatformIO Build button).
2. Wait for successful compile.
3. Note your build output path in `.pio/build/<environment>/`.
   - Common firmware file names:
     - `firmware.elf`
     - `firmware.bin`
     - `firmware.hex`

### Step 6 - Add Wokwi project files

> **Why Steps 4 and 5 are required first:**  
> Step 4 gives PlatformIO firmware code to compile, and Step 5 generates `.pio/build/<environment>/firmware.elf`.  
> Step 6 references that file path in `wokwi.toml`, so it must exist first.

1. In the project root, create `diagram.json`.
   - Define your board and parts.
   - You can copy a starter diagram from a new Wokwi online project for your board type.
2. In the project root, create `wokwi.toml`.
3. Add minimal configuration (adjust path to your real build artifact):

```toml
[wokwi]
version = 1
firmware = '.pio/build/<environment>/firmware.elf'
elf = '.pio/build/<environment>/firmware.elf'
```

4. Keep paths portable.
   - Use forward slashes (`/`) on all operating systems.

**Where to get fields and values**

- **wokwi.toml**
  - Use the `[wokwi]` section with `version`, `firmware`, and `elf` as in the example above.
  - Replace `<environment>` with your PlatformIO environment name from `platformio.ini` (e.g. the `[env:adafruit_feather_esp32s3]` block → use `adafruit_feather_esp32s3`). The path must point to the `.elf` file produced by **PlatformIO: Build** in Step 5.
- **diagram.json**
  - **Structure:** `version`, `editor` (use `"wokwi"`), `parts`, and `connections`. See [Wokwi diagram format](https://docs.wokwi.com/diagram-format).
  - **Board and parts types:** Create a new project at [wokwi.com](https://wokwi.com) for your MCU (e.g. ESP32-S3), add the board and any peripherals, then copy the generated `diagram.json` into your project root. You can also look up part IDs in the [Wokwi parts list](https://docs.wokwi.com/parts).

### Step 7 - Start and verify simulation

1. Run **Wokwi: Start Simulator** from Command Palette.
2. Confirm the virtual board opens and firmware starts.
   - Check expected LED behavior or serial output.
3. If needed, stop and restart after code or diagram changes.

### Step 8 - Iteration workflow (day-to-day)

1. Edit firmware code (`src/*`).
2. Build with PlatformIO.
3. Run/restart Wokwi simulator.
4. Update `diagram.json` when adding/changing components.

### Troubleshooting checklist

1. Simulator fails to start.
   - Re-check `wokwi.toml` path to firmware file.
   - Build again to ensure artifact exists.
2. Board behaves incorrectly.
   - Verify selected PlatformIO board matches the board in `diagram.json`.
3. No output or stale behavior.
   - Stop simulator, rebuild, then start simulator again.
4. License problems.
   - Re-run **Wokwi: Request a new License** and complete browser confirmation.

## Board portability config guide (Feather <-> XIAO <-> Metro)

### Goal

Keep all hardware assumptions in config files so board changes are mostly "edit one file and rebuild."

### Files added in this project

- `config/board_profile.h`
  - Picks the active board profile in one place.
- `config/board_xiao_esp32s3.h`
  - XIAO-specific capabilities and pins.
- `config/board_feather_esp32s3.h`
  - Feather-specific capabilities and pins.
- `config/board_metro_esp32s3.h`
  - Metro-specific capabilities and pins.
- `config/app_config.h`
  - Feature flags, buffer sizes, timeouts, and safety checks.

### Step 1 - Switch boards with one macro

1. Open `config/board_profile.h`.
2. Enable exactly one board macro:
   - `BOARD_XIAO_ESP32S3`
   - `BOARD_FEATHER_ESP32S3`
   - `BOARD_METRO_ESP32S3`
3. Rebuild and verify pin/peripheral behavior.

### Step 2 - What to keep board-specific

1. Board identity and capabilities.
   - `BOARD_NAME`
   - `HAS_PSRAM`, `MAX_PSRAM_MB`
   - `HAS_CAMERA`, `HAS_NATIVE_USB`, `HAS_BATTERY_CHARGER`
2. Pin map by function (not by raw GPIO in random files).
   - Core: status LED, buttons
   - Buses: I2C, SPI, UART (including RTS/CTS)
   - Optional: I2S/audio pins
3. Per-board fixed limits and defaults.
   - I2C/SPI frequency defaults
   - Pins unavailable on that board (`INVALID_PIN`)

### Step 3 - What to keep app-level and portable

1. Feature toggles in `config/app_config.h`.
   - `FEATURE_MODEM`, `FEATURE_AUDIO`, `FEATURE_CAMERA`, `FEATURE_OTA`, etc.
2. Buffer/memory knobs.
   - RX/TX ring sizes
   - HTTP and JSON limits
   - DMA/audio frame sizes
3. Timing and reliability knobs.
   - Modem timeouts
   - Task stack sizes
   - Watchdog settings

### Step 4 - Use capability-aware sizing

1. Keep memory settings conditional on PSRAM size.
   - Larger buffers on 8MB PSRAM boards.
   - Smaller defaults on lower-memory boards.
2. Add compile-time guards so invalid feature/board combos fail early.
   - Example: audio enabled but no I2S pin mapping.

### Step 5 - Keep hardware init centralized

1. In your firmware source, initialize I2C/SPI/UART in one place (for example, `hal_init.cpp`).
2. Read all pins and feature flags from `config/app_config.h`.
3. Avoid hardcoded GPIO values outside config headers.

### Practical rules for painless porting

1. No camera + normal IoT sensors/networking:
   - Usually low effort, mostly pin swaps.
2. Heavy PSRAM usage:
   - Expect trouble when moving from 8MB to 2MB boards.
3. UART cellular with RTS/CTS:
   - Usually straightforward if modem pins are centralized in config.
4. Camera/I2S-heavy designs:
   - Expect more pin-routing and memory rework.

### Important note on pin mappings

The starter pin maps in `config/board_*.h` are templates and should be validated against your exact board docs and schematic before production use.
