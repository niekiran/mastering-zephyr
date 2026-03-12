# Project Name

Zephyr Tilt Brick Breaker

# Project Objective

Build a Zephyr-based microcontroller game application where an MPU6050 IMU provides board tilt input to control a paddle, and an RGB LCD renders the brick breaker gameplay in real time.

# Expected Outcome

A working embedded brick breaker game that demonstrates:
- Sensor integration (MPU6050 via I2C)
- Input filtering for stable tilt-to-paddle control
- Deterministic game loop timing
- Embedded graphics rendering on an RGB LCD

All running on resource-constrained hardware using the Zephyr RTOS.

# Hardware Details

## Board

- **Name**: FastBit STM32 Nano (nano-v2.0)
- **Manufacturer**: FastBit Embedded Technologies
- **Form factor**: Circular PCB

## Microcontroller

- **Part**: STM32F303CCT6
- **Core**: 32-bit ARM Cortex-M4 with FPU
- **Max clock**: 72 MHz
- **Flash**: 256 KB
- **SRAM**: 48 KB
- **Package**: LQFP48

## Motion Sensor — MPU6050 (U3)

- **Type**: 6-axis MEMS IMU (3-axis gyroscope + 3-axis accelerometer)
- **Interface**: I2C
- **Features**: Onboard Digital Motion Processor (DMP) for Motion Fusion algorithms

| Signal | Net name           | MCU GPIO |
|--------|--------------------|----------|
| SCL    | I2C1_SCL           | PB6      |
| SDA    | I2C1_SDA           | PB7      |
| INT    | MPU6050_INTERUPT   | PA8      |

## Display — LCD GC9A01 (sheet 2)

- **Type**: 1.28" round LCD
- **Controller**: GC9A01
- **Interface**: SPI
- **Supply**: 3.3V

| Signal   | Net name  | MCU GPIO | Notes                              |
|----------|-----------|----------|------------------------------------|
| SCK      | LCD_SCL   | PA5      | SPI1_SCK                           |
| MOSI     | LCD_MOSI  | PA7      | SPI1_MOSI                          |
| MISO     | LCD_MISO  | PA6      | SPI1_MISO                          |
| D/C      | LCD_DCX   | PB1      |                                    |
| CS       | LCD_CSX   | PA4      |                                    |
| RESET    | LCD_RST   | PB8      |                                    |

### Round Display and Clipping

The GC9A01 is a 240x240 **round** display.  The 240x240 framebuffer is fully
addressable, but only pixels inside the inscribed circle (center 120,120,
radius 120) are physically visible through the round glass.  The display
orientation is set to **270 degrees clockwise** (`CONFIG_DISPLAY_ROTATION_270`).

All drawing primitives must perform **per-row circular clipping**: for each
scan line, compute the visible x-span from the circle equation and discard
pixels outside it.  This is handled by `circle_row_span()` in `src/display.c`.

### SPI Pixel Byte Order

The GC9A01 expects RGB565 pixel data in **big-endian** (MSB-first) byte order
over SPI.  The STM32F303 is little-endian, so every 16-bit pixel value must be
byte-swapped before transmission.  Use `sys_cpu_to_be16()` from
`<zephyr/sys/byteorder.h>` on each pixel value written to a framebuffer row.

> **Note:** If the Zephyr MIPI DBI SPI driver is later found to handle the swap
> internally, remove the application-side `sys_cpu_to_be16()` calls to avoid a
> double swap (symptoms: colours appear wrong — e.g. red shows as cyan).

## Touch Panel — CST816S (sheet 2)

- **Interface**: I2C
- **Note**: Likely shares the I2C1 bus with MPU6050 (same PB6/PB7 nets); RST and INT are dedicated GPIOs

| Signal | Net name | MCU GPIO | Notes                                          |
|--------|----------|----------|------------------------------------------------|
| SCL    | TP_SCL   | PB6      | Shared I2C1 bus with MPU6050 — verify from J5  |
| SDA    | TP_SDA   | PB7      | Shared I2C1 bus with MPU6050 — verify from J5  |
| RESET  | TP_RST   | —        | ⚠ Not clearly readable from schematic — verify |
| INT    | TP_IN    | —        | ⚠ Not clearly readable from schematic — verify |

## MicroSD Card Slot (sheet 2)

- **Interface**: SPI
- **Signals**: SD_CS, SD_MOSI, SD_MISO, SD_SCL, SD_DET (card detect)

## LEDs

| Label | Colour | GPIO | Function              |
|-------|--------|------|-----------------------|
| D1    | Red    | —    | Power ON/OFF indicator |
| D2    | Blue   | PA1  | User LED              |
| D3    | Green  | PA2  | User LED              |
| D4    | Red    | PA3  | User LED              |

## Push Buttons

| Label | GPIO  | Function                  |
|-------|-------|---------------------------|
| SW1   | NRST  | Reset                     |
| SW2   | PA0   | User button               |
| SW3   | BOOT0 | Boot mode select          |

## Oscillators

| Type | Frequency    | Pins              |
|------|-------------|-------------------|
| HSE  | 8 MHz       | PF0 (IN), PF1 (OUT) |
| LSE  | 32.768 kHz  | PC14 (IN), PC15 (OUT) |

## USB to UART Bridge

- **IC**: CH340N (U5)
- **MCU UART**: USART1 — PA9 (TX), PA10 (RX)
- **Connector**: J2 USB-B Micro

## Debug / Programming Interface

- **SWD connector**: J1 (SWCLK, SWDIO, GND, 3.3V, RESET, SWO)
- **Programming methods**: ST-Link via SWD, or USB bootloader (BOOT0 + RESET)

## Power Supply

- **Input**: 5V via USB or ST-Link
- **Regulator**: U2 LP2985-3.3 (5V → 3.3V)
- **Note**: Do not connect external power to header sockets

# Device Orientation

## Normal Playing Position

The player holds the board with the LCD display facing them. The USB connector is to the right.

## MPU6050 Axis Layout (as labelled on PCB)

| Axis | Direction on board                        |
|------|-------------------------------------------|
| +X   | Points to the right of the board          |
| +Y   | Points toward the top of the board        |
| +Z   | Points out of the board toward the viewer |

## Tilt-to-Paddle Mapping

- The player tilts the board over the **Y axis (Pitch)**.
- When the **right side (+X side) is lifted**, the accelerometer X value goes **negative** (gravity projects onto −X). The paddle must move **right**.
- When the **left side (−X side) is lifted**, the accelerometer X value goes **positive** (gravity projects onto +X). The paddle must move **left**.
- In code: `paddle_velocity ∝ −accel_x` (invert the X channel reading to get the correct direction).

## GC9A01 Display Orientation

### MADCTL Register (0x36)

The GC9A01 orientation is controlled by the MADCTL register.  The relevant
bits are:

| Bit | Name | Effect |
|-----|------|--------|
| 7   | MY   | Mirror Y — flip row address order |
| 6   | MX   | Mirror X — flip column address order |
| 5   | MV   | Swap XY — exchange row/column scan direction |
| 3   | BGR  | Colour byte order: 0 = RGB, 1 = BGR |

Bits ML (4) and MH (2) control refresh scan order and are not exposed because
they have no visible effect on static game rendering.

### Rotation Presets

| Rotation | MV | MX | MY | Kconfig symbol |
|----------|----|----|----|----------------|
| 0°       | 0  | 0  | 0  | `DISPLAY_ROTATION_0` (default) |
| 90° CW   | 1  | 1  | 0  | `DISPLAY_ROTATION_90` |
| 180°     | 0  | 1  | 1  | `DISPLAY_ROTATION_180` |
| 270° CW  | 1  | 0  | 1  | `DISPLAY_ROTATION_270` |

**TBD on hardware.** Start with `DISPLAY_ROTATION_0` (GC9A01 power-on default).
If the image is upside-down or mirrored, use `west build -t menuconfig` and
navigate to **Tilt Brick Breaker → Display → Display Orientation** to try the
other presets without touching source code.

In code, select the preset with:

```c
#if CONFIG_DISPLAY_ROTATION_0
    /* 0° */
#elif CONFIG_DISPLAY_ROTATION_90
    /* 90° CW */
#elif CONFIG_DISPLAY_ROTATION_180
    /* 180° */
#elif CONFIG_DISPLAY_ROTATION_270
    /* 270° CW */
#endif
```

### Circular Safe Area

The GC9A01 module has a round glass.  The 240×240 framebuffer is fully
addressable, but pixels outside the inscribed circle (radius 120, centred at
(120, 120)) are physically masked and never visible.  Rendering outside this
area wastes SPI bandwidth and may produce artefacts at the panel boundary.

Use `CONFIG_DISPLAY_IS_CIRCULAR`, `CONFIG_DISPLAY_CIRCLE_RADIUS`, and
`CONFIG_DISPLAY_CIRCLE_CENTER_X/Y` to keep all rendering within the safe area.

# Software Details

# Coding Standard

## Configuration Macros

Device orientation, tilt mapping, and display settings are configured via
Kconfig and must not be hard-coded anywhere in the source.  Use
`west build -t menuconfig` and navigate to **Tilt Brick Breaker** to change
them.

In code, reference these settings as `CONFIG_TILT_*` and `CONFIG_DISPLAY_*`
symbols, which Zephyr makes available in every translation unit via the
generated `autoconf.h`.

**Exception — `TILT_LPF_ALPHA`:** Kconfig cannot express a float, so the
smoothing factor is stored as an integer percentage (`CONFIG_TILT_LPF_ALPHA_PCT`).
Use the `TILT_LPF_ALPHA` float conversion helper defined in `include/config.h`
instead of using `CONFIG_TILT_LPF_ALPHA_PCT` directly in code.

This rule covers, but is not limited to:

| Setting | Symbol to use in code | menuconfig path |
|---------|----------------------|-----------------|
| Axis selection | `CONFIG_TILT_AXIS` | Tilt Input |
| Axis swap | `CONFIG_TILT_AXIS_SWAP` | Tilt Input |
| Sign inversion | `CONFIG_TILT_INVERT` | Tilt Input |
| Calibration enable | `CONFIG_TILT_CALIBRATION_ENABLE` | Tilt Input |
| Calibration samples | `CONFIG_TILT_CALIBRATION_SAMPLES` | Tilt Input |
| Dead zone | `CONFIG_TILT_DEAD_ZONE_MG` | Tilt Input |
| LPF smoothing | `TILT_LPF_ALPHA` *(float helper from `include/config.h`)* | Tilt Input |
| Max tilt clamp | `CONFIG_TILT_MAX_MG` | Tilt Input |
| Display width | `CONFIG_DISPLAY_WIDTH` | Display → Display Geometry |
| Display height | `CONFIG_DISPLAY_HEIGHT` | Display → Display Geometry |
| Column start offset | `CONFIG_DISPLAY_X_OFFSET` | Display → Display Geometry |
| Row start offset | `CONFIG_DISPLAY_Y_OFFSET` | Display → Display Geometry |
| Rotation preset | `CONFIG_DISPLAY_ROTATION_0` / `_90` / `_180` / `_270` | Display → Display Orientation |
| Mirror X (MX bit) | `CONFIG_DISPLAY_MIRROR_X` | Display → Display Orientation |
| Mirror Y (MY bit) | `CONFIG_DISPLAY_MIRROR_Y` | Display → Display Orientation |
| Swap XY (MV bit) | `CONFIG_DISPLAY_SWAP_XY` | Display → Display Orientation |
| BGR colour order | `CONFIG_DISPLAY_BGR_ORDER` | Display → Display Orientation |
| Circular mask active | `CONFIG_DISPLAY_IS_CIRCULAR` | Display → Circular Display Safe Area |
| Circle radius | `CONFIG_DISPLAY_CIRCLE_RADIUS` | Display → Circular Display Safe Area |
| Circle centre X | `CONFIG_DISPLAY_CIRCLE_CENTER_X` | Display → Circular Display Safe Area |
| Circle centre Y | `CONFIG_DISPLAY_CIRCLE_CENTER_Y` | Display → Circular Display Safe Area |

# Linting

# Unit Testing

# Required Project Features

## 1. Display and UI

- Full color gameplay on the RGB LCD.
- An initial front screen shown immediately after reset of the device:
  - Must display the game name.
  - Must show a 3-second countdown before gameplay starts.
  - Must not use plain text only — use simple custom images or sprites for the front screen.
- During gameplay, show a Heads Up Display (HUD) with:
  - Current score.
  - Lives remaining (player starts with 3 lives).
- An end screen when the game finishes:
  - Show a Win or Lose message.
  - Show the final score.
  - Must not use plain text only — use simple custom images or sprites for the end screen.

## 2. Input and Control

- Use the MPU6050 tilt input to move the paddle left and right.
- Paddle movement must feel smooth and stable:
  - Include a dead zone to prevent drift when the board is near-level.
  - Include smoothing or filtering (e.g. low-pass filter) to reduce jitter.
  - Include startup calibration if needed to establish a neutral reference.

## 3. Game Rules

- Classic brick breaker gameplay: ball, paddle, bricks, and collision detection.
- Player starts with 3 lives. A life is lost when the ball falls below the paddle.
- Win condition: all bricks are cleared.
- Lose condition: all lives are exhausted (lives reach zero).

# Optional Features

# Open Questions

The following values are required by the implementation but have no confirmed
target in the spec. They are set to safe placeholders in Kconfig (change via
`west build -t menuconfig`) and must be tuned on hardware before final release.

| Symbol | Placeholder | What needs confirming |
|--------|-------------|----------------------|
| `CONFIG_TILT_CALIBRATION_SAMPLES` | 32 | How many startup samples give a stable zero reference without an unacceptable boot delay? |
| `CONFIG_TILT_DEAD_ZONE_MG` | 50 mg | What milli-g threshold eliminates drift without making the centre feel unresponsive? |
| `CONFIG_TILT_LPF_ALPHA_PCT` | 20 (= 0.20) | What EMA coefficient balances smoothness against paddle lag at the chosen game loop rate? |
| `CONFIG_TILT_MAX_MG` | 500 mg | What tilt magnitude (milli-g) should map to full paddle speed for a comfortable play feel? |
| `CONFIG_MPU6050_SMPLRT_DIV` | 9 (100 Hz) | What is the target game loop rate? Sensor sample rate should match or exceed it. |

# Implementation Plan

Before implementing any checkpoint, read and follow the game plan file at:
`C:\Users\Admin\.claude\plans\warm-giggling-teapot.md`

That file is the single source of truth for checkpoint ordering, file changes, and
implementation details. If the plan changes during development, update that same
file so future implementations always refer to the latest version.
