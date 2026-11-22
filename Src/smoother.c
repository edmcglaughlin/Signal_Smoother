/**
 * smoother.c - "Signal Smoother" real-time DSP pipeline.
 *
 * Generates a noisy tone, low-pass filters it with an FIR, and measures
 * the output level. One frame must finish within SM_PERIOD_MS.
 *
 * Inherited code. It works and the numbers are correct. It is also the
 * thing you are being asked to make fast enough. Measure first.
 */
#include "smoother.h"
#include "fir_lowpass.h"
#include "micros.h"
#include "stm32f4xx.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

/* Working buffers. */
 float32_t in_buf[FIR_BLOCK_SIZE];
 float32_t out_buf[FIR_BLOCK_SIZE];

static uint32_t sample_index;       /* running sample position for the tone */

volatile SmootherStats g_sm;
uint32_t g_deadline_us;

uint32_t sin_cycles = 0;

/* Fill in_buf[] with a clean tone plus uniform noise. */
static void gen_signal(void)
{
    uint32_t i;
			uint32_t start_cycles, end_cycles;
	
    for (i = 0; i < FIR_BLOCK_SIZE; i++) {
        float32_t t     = (float32_t)(sample_index + i) / (float32_t)FIR_FS_HZ;
	
	  __DSB();
    __ISB();
    start_cycles = DWT->CYCCNT;
        // float32_t tone  = 0.8 * arm_sin_f32(2.0 * PI * SM_TONE_HZ * t);
        float32_t tone  = 0.8 * sin(2.0 * PI * SM_TONE_HZ * t);
	  __DSB();
    __ISB();
    end_cycles = DWT->CYCCNT;
		sin_cycles = end_cycles - start_cycles;			
			
			
			
        // float32_t tone  = 0.8 * sin(2.0 * PI * SM_TONE_HZ * t);
        float32_t noise = ((float32_t)rand() / (float32_t)RAND_MAX - 0.5) * 0.5;
        in_buf[i] = tone + noise;
    }
    sample_index += FIR_BLOCK_SIZE;
}

/* Root-mean-square level of out_buf[]. */
static float32_t compute_level(void)
{
    float32_t acc = 0.0;
    uint32_t i;
    for (i = 0; i < FIR_BLOCK_SIZE; i++) {
        acc += out_buf[i] * out_buf[i];
    }
    return sqrt(acc / (float32_t)FIR_BLOCK_SIZE);
}

/* ------------------------------------------------------------------ */
/* Public interface                                                    */
/* ------------------------------------------------------------------ */
volatile uint32_t design;	// ECM  - filter design time
void DWT_Init(void);

void Smoother_Init(void)
{
    memset((void *)&g_sm, 0, sizeof(g_sm));
    sample_index = 0;
    srand(1);
    g_deadline_us = SM_PERIOD_MS * 1000U;   /* 10 ms -> 10000 us */
	  
    // Initialize fir_lowpass module
    uint32_t t = Micros_Now();
    fir_lowpass_init();
    design = Micros_Since(t);	
		DWT_Init();
}

void DWT_Init(void) {
    /* Enable TRC (Trace) in CoreDebug DEMCR */
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;

    /* Reset the cycle counter */
    DWT->CYCCNT = 0;

    /* Enable the cycle counter */
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

void Smoother_RunFrame(void)
{
    uint32_t t0 = Micros_Now();
    uint32_t t;

    /* Stage 1: generate */
    t = Micros_Now();
    gen_signal();
    g_sm.us_generate = Micros_Since(t);

    /* Stage 2: filter */
    t = Micros_Now();
    fir_lowpass_process(in_buf, out_buf);
    g_sm.us_filter = Micros_Since(t);

    /* Stage 3: level */
    t = Micros_Now();
    g_sm.level = (float)compute_level();
    g_sm.us_level = Micros_Since(t);

    /* Bookkeeping */
    g_sm.us_total = Micros_Since(t0);
    if (g_sm.us_total > g_sm.us_worst) {
        g_sm.us_worst = g_sm.us_total;
    }
    g_sm.frames++;
    if (g_sm.us_total > g_deadline_us) {
        g_sm.misses++;
    }
}

