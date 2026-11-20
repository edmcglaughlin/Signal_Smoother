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
#include "micros.h"
#include "stm32f4xx.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

#define PI 3.14159265358979323846

/* Working buffers. */
static double in_buf[SM_BLOCK];
static double out_buf[SM_BLOCK];
static double coeffs[SM_TAPS];
static double history[SM_TAPS];     /* tail of the previous block        */
static uint32_t sample_index;       /* running sample position for the tone */

volatile SmootherStats g_sm;
uint32_t g_deadline_us;

/* ------------------------------------------------------------------ */
/* Stage helpers                                                       */
/* ------------------------------------------------------------------ */

/* Design a windowed-sinc low-pass filter into coeffs[]. */
static void design_lowpass(void)
{
    double fc = SM_CUTOFF_HZ / (double)SM_FS_HZ;   /* normalized cutoff  */
    double mid = (double)(SM_TAPS - 1) / 2.0;
    uint32_t k;

    for (k = 0; k < SM_TAPS; k++) {
        double m = (double)k - mid;
        double h;
        if (m == 0.0) {
            h = 2.0 * fc;
        } else {
            h = sin(2.0 * PI * fc * m) / (PI * m);
        }
        /* Hamming window */
        double w = 0.54 - 0.46 * cos(2.0 * PI * (double)k / (double)(SM_TAPS - 1));
        coeffs[k] = h * w;
    }
}

/* Fill in_buf[] with a clean tone plus uniform noise. */
static void gen_signal(void)
{
    uint32_t i;
    for (i = 0; i < SM_BLOCK; i++) {
        double t     = (double)(sample_index + i) / (double)SM_FS_HZ;
        double tone  = 0.8 * sin(2.0 * PI * SM_TONE_HZ * t);
        double noise = ((double)rand() / (double)RAND_MAX - 0.5) * 0.5;
        in_buf[i] = tone + noise;
    }
    sample_index += SM_BLOCK;
}

/* Convolve in_buf[] with coeffs[] into out_buf[], carrying history across
 * frames so the filter output is continuous. */
static void fir_filter(void)
{
    uint32_t n, k;
    for (n = 0; n < SM_BLOCK; n++) {
        double acc = 0.0;
        for (k = 0; k < SM_TAPS; k++) {
            int32_t idx = (int32_t)n - (int32_t)k;
            double x;
            if (idx >= 0) {
                x = in_buf[idx];
            } else {
                x = history[SM_TAPS + idx];   /* reach into previous block */
            }
            acc += coeffs[k] * x;
        }
        out_buf[n] = acc;
    }
    /* Save the last SM_TAPS samples as history for next frame. */
    for (k = 0; k < SM_TAPS; k++) {
        history[k] = in_buf[SM_BLOCK - SM_TAPS + k];
    }
}

/* Root-mean-square level of out_buf[]. */
static double compute_level(void)
{
    double acc = 0.0;
    uint32_t i;
    for (i = 0; i < SM_BLOCK; i++) {
        acc += out_buf[i] * out_buf[i];
    }
    return sqrt(acc / (double)SM_BLOCK);
}

/* ------------------------------------------------------------------ */
/* Public interface                                                    */
/* ------------------------------------------------------------------ */
void Smoother_Init(void)
{
    memset((void *)&g_sm, 0, sizeof(g_sm));
    memset(history, 0, sizeof(history));
    sample_index = 0;
    srand(1);
    g_deadline_us = SM_PERIOD_MS * 1000U;   /* 10 ms -> 10000 us */
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
    design_lowpass();
    fir_filter();
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
