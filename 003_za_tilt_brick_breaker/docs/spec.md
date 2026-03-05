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

# Software Details

# Coding Standard

# Linting

# Unit Testing

# Required Project Features

# Optional Features
