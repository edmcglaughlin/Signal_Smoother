# Signal Smoother (starter)

SENG3400 performance lab. STM32F407 Discovery, Keil RTX5 (CMSIS-RTOS2).

See **KEIL_SETUP.md** for step-by-step project setup. It captures the exact
settings that make this build and run on the Discovery board.

## What it does

One RTX5 thread runs a three-stage DSP pipeline once every `SM_PERIOD_MS`:

1. **Generate** a block of a noisy test signal (a 300 Hz tone plus noise).
2. **Filter** it with a 128-tap low-pass FIR to smooth it.
3. **Level**: measure the RMS of the output and show it on the four user LEDs.

A frame must finish inside `SM_PERIOD_MS` (10 ms). If it does not, the frame
is counted as a miss. Right now it misses, and the CPU never rests.

## Files

- `src/main.c` - RTX5 setup, the pipeline thread, LED display, clock config.
- `src/smoother.c/.h` - the DSP pipeline (generate, filter, level).
- `src/micros.h` - a free-running TIM2 microsecond timer for timing your code.

## Measuring

`Smoother_RunFrame()` times each stage into the global `g_sm`
(`us_generate`, `us_filter`, `us_level`, `us_total`, `us_worst`, `frames`,
`misses`). Every `us_*` value is in **microseconds**. Read them in the Keil
**Watch window** with Periodic Window Update on; no trace or SWO is needed.
`g_deadline_us` is the budget one frame must fit inside: **10,000 us (10 ms)**.

The timer is **TIM2 prescaled to 1 MHz**, so one tick is one microsecond.

## Build

As delivered the project is configured for debugging: optimization is off
(`-O0`). The hardware FPU must be set to **Single Precision** (not "Not Used")
or RTX will fault at start. These are starting conditions, not requirements.

## Your job

Make every frame finish inside the budget with CPU headroom to spare,
without changing what the program computes. Measure first, then improve,
and keep the evidence. See the lab handout for the target numbers, the
per-stage budget, the performance-smells checklist, and the rubric.
