# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Specification

Always read `docs/spec.md` before writing any code, suggesting any architecture, or asking clarifying questions.
Treat it as the single source of truth for hardware, requirements, and constraints.
If anything in the request conflicts with `docs/spec.md`, flag it before proceeding.

## Project Overview

This is a Zephyr RTOS application: a tilt-controlled brick breaker game targeting the `fastbit_stm32nano_stm32f303xc` board (STM32F303xC MCU). It lives inside a West T2 workspace at `D:\zephyr_workspace\zephyrproject\my-apps\`.

The project is in early development. `src/game.c`, `src/display.c`, `src/input.c`, and `src/util.c` are currently empty stubs, and only `src/main.c` is registered in `CMakeLists.txt`.

## Build Commands

All commands run from the workspace root (`D:\zephyr_workspace\zephyrproject\my-apps\`) using the West build system.

```shell
# Build for target board
west build -b fastbit_stm32nano 003_za_tilt_brick_breaker

# Build with debug config (enables logging + debug optimizations)
west build -b fastbit_stm32nano 003_za_tilt_brick_breaker -- -DEXTRA_CONF_FILE=debug.conf

# Flash to hardware
west flash

# Clean build
west build -t clean

# Run Twister integration tests
west twister -T tests --integration
```

## Architecture

Intended module split matching the source file names:

| File | Purpose |
|------|---------|
| `src/main.c` | Entry point, main loop |
| `src/game.c` | Game logic (ball, paddle, bricks, collision) |
| `src/display.c` | OLED/LCD rendering via Zephyr display API |
| `src/input.c` | Accelerometer/tilt input via Zephyr sensor API |
| `src/util.c` | Shared utilities |

## Key Configuration Files

- `prj.conf` — Kconfig options (currently enables UART console/stdout)
- `debug.conf` — Extra overlay for debug builds: enables `CONFIG_LOG` and `CONFIG_APP_LOG_LEVEL_DBG`
- `Kconfig` — App-level Kconfig; exposes `APP` log module via `subsys/logging/Kconfig.template.log_config`
- `boards/fastbit_stm32nano_stm32f303xc.overlay` — Device tree overlay for the target board (currently empty)

## Adding New Source Files

When adding new `.c` files, register them in `CMakeLists.txt`:

```cmake
target_sources(app PRIVATE
    src/main.c
    src/game.c
    src/display.c
    src/input.c
    src/util.c
)
```

## Zephyr APIs Likely Needed

- **Display**: `<zephyr/drivers/display.h>` — framebuffer/pixel drawing
- **Sensor (accelerometer)**: `<zephyr/drivers/sensor.h>` — `sensor_sample_fetch` / `sensor_channel_get`
- **Logging**: `LOG_MODULE_REGISTER(app, CONFIG_APP_LOG_LEVEL)` + `LOG_INF/DBG/ERR`
- **Timing**: `k_msleep()`, `k_uptime_get()` from `<zephyr/kernel.h>`
