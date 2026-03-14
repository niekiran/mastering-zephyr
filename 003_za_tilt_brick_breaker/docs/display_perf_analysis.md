# Display Rendering Performance Analysis & Optimization Plan

## Current Performance Bottlenecks

### 1. MCU runs at 8 MHz — no PLL enabled (CRITICAL) — FIXED

**File**: `zephyr/boards/others/fastbit_stm32nano/fastbit_stm32nano_stm32f303xc.dts:65-70`

The board DTS used the 8 MHz HSI oscillator directly — no PLL. All buses
(AHB, APB1, APB2) were running at 8 MHz.

#### Fix 1a: HSE-based PLL → 72 MHz (NOT YET TESTED)

PLL via external HSE crystal: 8 MHz HSE × 9 = 72 MHz SYSCLK.
Overlay: `&clk_hse` (status okay) + `&pll` (prediv=1, mul=9, clocks=&clk_hse) + `&rcc` (72 MHz).

**Status**: Not yet tested on hardware. Needs verification of whether the
Fastbit STM32 Nano board has an external HSE crystal populated.

#### Fix 1b: HSI-based PLL → 64 MHz (APPLIED — VERIFIED WORKING)

PLL via internal HSI oscillator: 8 MHz HSI / 2 (fixed F3 divider) × 16 = 64 MHz SYSCLK.
Overlay: `&clk_hsi` (status okay) + `&pll` (prediv=1, mul=16, clocks=&clk_hsi) + `&rcc` (64 MHz).
No external crystal needed.

- SYSCLK = 64 MHz, AHB = 64 MHz
- APB1 = 32 MHz (prescaler 2), APB2 = 64 MHz (prescaler 1)

**Debugging notes (critical lessons learned):**

1. `&clk_hsi` must have `status = "okay"` — it is `disabled` by default in the
   SoC DTSI (`stm32f3.dtsi`). Without this, the PLL has no input clock and the
   MCU hangs.

2. **Must use `west build -p` (pristine) when changing `clock-frequency`.**
   `west build -t clean` only removes object files but keeps the CMake cache.
   `CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC` (derived from DTS `clock-frequency`)
   stays stale, causing the flash wait state calculation in
   `clock_stm32_ll_common.c` to use the old frequency. At >48 MHz SYSCLK this
   means 0 or 1 WS instead of the required 2 WS, and the MCU reads corrupted
   instructions from flash.
   STM32F303 flash wait states: 0 WS ≤24 MHz, 1 WS ≤48 MHz, 2 WS ≤72 MHz.

### 2. SPI1 clock was only 4 MHz (CRITICAL) — FIXED by Fix 1b

- SPI1 is on APB2 bus. Was: APB2 = 8 MHz, SPI = 4 MHz (DIV2)
- Overlay requests `mipi-max-frequency = <DT_FREQ_M(60)>` but prescaler is power-of-2
- Now: APB2 = 64 MHz, SPI = **32 MHz** (DIV2)
- Full 240x240 RGB565 frame = 115,200 bytes = 921,600 bits
- Old: ~230 ms per frame (~4.3 fps)
- New: ~29 ms per frame (~35 fps)

### 3. No DMA on SPI (MODERATE) — NOT YET FIXED

- `CONFIG_SPI_STM32_DMA` not in `prj.conf`
- No DMA channels assigned to `&spi1` in overlay
- Every `display_write()` is CPU-blocking polling transfer
- CPU cannot run game logic during pixel push

**Fix**: Add `CONFIG_DMA=y` + `CONFIG_SPI_STM32_DMA=y` to prj.conf, add DMA
channels to `&spi1` in overlay, enable `&dma1`. Channel numbers need verification
against STM32F303 reference manual DMA request mapping.

### 4. Full-screen redraw every frame (MAJOR) — NOT YET FIXED

**File**: `src/display.c:363-372` — `screen_draw_gameplay()`

Every frame calls `screen_clear()` (240 SPI writes, 115 KB) then redraws
everything. Only ball, paddle, score, and changed bricks need updating.

**Fix**: Dirty-region rendering — track previous positions, erase old + draw new.
Typical frame: ~4-5 KB vs 115 KB (~25x less SPI data).

### 5. Per-pixel SPI transactions in text/shapes (MAJOR) — NOT YET FIXED

**File**: `src/display.c:58-65` — `draw_pixel()`

Each pixel triggers a full `display_write()` with mutex lock, CASET+RASET
commands, and 2-byte data. `draw_char()` at scale=1 calls `draw_pixel()` 35
times per character.

**Fix**: Batch into row-wide `commit_line()` calls. Reduces scale=1 char from
35 SPI calls to 7; scale=2 from 70 fill_rect calls to 14 commit_line calls.

### 6. screen_clear() uses 240 individual line writes (MINOR)

Could send larger blocks (8-16 lines) to reduce per-transfer overhead.
RAM limited (48 KB SRAM, 16-line buffer = 7,680 bytes).

## Optimization Fixes — Ranked by Impact

1. **Fix 1a: PLL via HSE → 72 MHz** — ~9x SPI speedup. **NOT YET TESTED.**
   **Fix 1b: PLL via HSI → 64 MHz** — ~8x SPI speedup, config-only. **APPLIED.**
2. **Fix 2: Dirty-region rendering** — ~25x less SPI data per frame. TODO.
3. **Fix 3: Batch text rendering** — ~10x for text drawing. TODO.
4. **Fix 4: SPI DMA** — frees CPU during transfers. TODO.

## Key Files

| Purpose | Path |
|---------|------|
| Board DTS | `zephyr/boards/others/fastbit_stm32nano/fastbit_stm32nano_stm32f303xc.dts` |
| Project overlay | `boards/fastbit_stm32nano_stm32f303xc.overlay` |
| Project config | `prj.conf` |
| Display rendering | `src/display.c` |
| STM32F3 SoC DTSI | `zephyr/dts/arm/st/f3/stm32f3.dtsi` (SPI1 at APB2 line 236) |
| SPI prescaler logic | `zephyr/drivers/spi/spi_ll_stm32.c` (lines 617-690) |
| GC9A01 display driver | `zephyr/drivers/display/display_gc9x01x.c` (supports partial updates) |
| MIPI DBI SPI driver | `zephyr/drivers/mipi_dbi/mipi_dbi_spi.c` |
| F0/F3 PLL binding | `zephyr/dts/bindings/clock/st,stm32f0-pll-clock.yaml` (prediv + mul) |
| F3 RCC binding | `zephyr/dts/bindings/clock/st,stm32f3-rcc.yaml` |

## Performance Math

- 240x240 RGB565 = 115,200 bytes per full frame
- At 32 MHz SPI (Fix 1b): 115,200 x 8 / 32,000,000 = ~28.8 ms (full frame)
- With dirty-region: ~4-5 KB per frame = ~1 ms (gameplay updates only)
