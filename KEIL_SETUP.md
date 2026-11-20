# Keil setup - Signal Smoother

Tested on the STM32F407 Discovery with the on-board ST-Link and MDK
(uVision 5, AC6 compiler). 

## 1. Packs (one-time, via Pack Installer)

- **ARM::CMSIS** (includes CMSIS-Core and Keil RTX5)
- **Keil::STM32F4xx_DFP**
- **ARM::CMSIS-Compiler** 

## 2. New project

1. Project > New uVision Project, save into its own folder. Copy the `src`
   folder next to it.
2. Device: **STMicroelectronics > STM32F407VGTx**.

## 3. Run-Time Environment (RTE)

Tick:
- **CMSIS > CORE**
- **CMSIS > RTOS2 (API) > Keil RTX5**, Variant **Source**
- **Device > Startup**
- **Device > STM32Cube Framework (API) > Classic**
- **Device > STM32Cube HAL >** Common, Cortex, GPIO, PWR, RCC
- **CMSIS-Compiler > CORE**, and **CMSIS-Compiler > STDOUT > ITM**

Click **Resolve** for anything yellow, then OK.

## 4. Fix the HAL config

Open `RTE/Device/STM32F407VGTx/stm32f4xx_hal_conf.h`:
- **GOTCHA:** set `#define HSE_VALUE  8000000U` (the HAL default is 25 MHz;
  the board's crystal is 8 MHz, and a wrong value makes `SystemCoreClock`
  and the budget wrong).
- Confirm `HAL_MODULE`, `HAL_RCC`, `HAL_GPIO`, `HAL_PWR`, `HAL_CORTEX`
  modules are enabled (not commented out).

## 5. Add the code

- Add `src/main.c` and `src/smoother.c` to Source Group 1.
- Options for Target > C/C++ (AC6) > **Include Paths**: add the `src` folder.

## 6. Target options (the baseline conditions)

- Target tab > **Floating Point Hardware = Single Precision**.
- C/C++ (AC6) tab > **Optimization = -O0** (the baseline).
- Debug tab > **ST-Link Debugger**. Settings > Flash Download > **Reset and
  Run**. 

## 7. Clock source

`main.c` uses `RCC_HSE_ON`. If the board hangs in `SystemClock_Config`
(no LEDs, stuck before the thread), the ST-Link is feeding the clock instead
of a crystal: change that line to **`RCC_HSE_BYPASS`**, rebuild, reflash.

## 8. Build, flash, run

1. Build (F7). If "cannot open smoother.h", fix the include path (step 5).
2. Download (F8). The four LEDs should light to about three bars.

## 9. Measure

1. Start a debug session (Ctrl+F5), then Run (F5).
2. View > Watch Windows > Watch 1. Add `g_sm` (expand it) and
   `g_deadline_us`.
3. View menu > turn on **Periodic Window Update** so the values refresh live
   (read over SWD; no trace needed).
4. Baseline you should see: `frames` and `misses` climbing together (it
   misses every deadline), `us_total` well above `g_deadline_us` (10,000),
   `us_filter` dominating the other stages, and `level` near 0.57.
   To calculate CPU Usage divide g_sm.us_total / g_deadline.us

## Notes

- All `us_*` values are **microseconds** (TIM2 is prescaled to 1 MHz, so one
  tick is one microsecond).
- No SWO or trace is required. If you turn trace on and leave the per-frame
  `printf` in, the SWO will report "Data Overflow"; that is cosmetic and does
  not affect the Watch values. Measuring is done entirely through the Watch
  window.
