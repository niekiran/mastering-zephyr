# Mastering Zephyr RTOS with DeviceTree and Board Bring Up

Complete workflow: toolchain setup, DeviceTree from scratch, custom board porting, and real world applications on STM32.

**Udemy course:** https://www.udemy.com/course/mastering-zephyr-rtos-with-devicetree/?referralCode=D0E4381AAF67FBBA1B15

This repository contains the source code, custom boards, out-of-tree drivers, and DeviceTree bindings used throughout the course. The Zephyr RTOS source, HALs, and SDK are **not** included here — **Zephyr RTOS environment setup and SDK installation are covered step-by-step in the course.**

---

## Folder structure

### Course applications

| Folder | What you learn |
|---|---|
| `001_za_led_blinky/` | First Zephyr application. Project layout, `prj.conf`, DeviceTree basics, GPIO, building and flashing. |
| `002_za_threads_workq_ledburst/` | Threads, work queues, synchronisation primitives, and a small LED-burst pattern. |
| `003_za_tilt_brick_breaker/` | A full brick-breaker game driven by tilt input on a small display — event-based architecture, timing, display driver integration. |
| `app/` | Reference template app. Copy this when starting a new application. |

Each application is a standalone Zephyr project with the usual layout: `CMakeLists.txt`, `prj.conf`, `src/`, `boards/`, optional `debug.conf`, and a `sample.yaml` for Twister.

### Shared infrastructure (used by the apps above)

| Folder | Purpose |
|---|---|
| `boards/` | Custom board definitions. `common/` holds shared board parts, `vendor/` holds the course's custom boards (used in the Board Bring Up sections). |
| `drivers/` | Out-of-tree drivers written during the course: `blink/` and `sensor/`. Built as part of the workspace so apps can `select` them. |
| `dts/bindings/` | Custom DeviceTree bindings for the out-of-tree drivers and custom hardware. |
| `lib/custom/` | Shared library code reused across applications. |
| `include/app/` | Shared C headers available to every app. |
| `scripts/` | West extension commands for the workspace. |
| `tests/` | Integration tests. |
| `doc/` | Doxygen / Sphinx scaffolding. |

### Workspace files

| File | Purpose |
|---|---|
| `west.yml` | West manifest. Tells `west` to fetch Zephyr plus only the HALs needed (`cmsis_6`, `hal_nordic`, `hal_stm32`) — keeps the workspace small. |
| `CMakeLists.txt`, `Kconfig` | Top-level build entry points for the workspace. |
| `zephyr/module.yml` | Registers this folder as a Zephyr module so its boards, drivers, and bindings are visible to the build system. |
| `notes`, `notes_macros_rust` | Course-specific setup notes. |

---

## Building an application

Once you have completed the Zephyr environment and SDK setup lessons in the course, build any application from the workspace root:

```bash
# Build 001_za_led_blinky for the Fastbit STM32 Nano board
west build -b fastbit_stm32nano mastering-zephyr/001_za_led_blinky

# Flash
west flash
```

Replace the app path and board target as appropriate for the lesson you are on.

---

## License

Apache 2.0 — see [LICENSE](LICENSE).
