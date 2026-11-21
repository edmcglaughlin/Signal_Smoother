/**
 * fir_lowpass.c
 *
 * Low-pass FIR filter on the STM32F407 (Cortex-M4F) using CMSIS-DSP.
 *
 * Build requirements:
 *   - Defines: ARM_MATH_CM4, __FPU_PRESENT=1
 *   - Compiler flags: -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard
 *   - Link: libarm_cortexM4lf_math.a (or build CMSIS-DSP from source), and -lm
 *
 * Design: windowed-sinc (Hamming), normalised for unity gain at DC.
 *   - Transition band ~ 3.3 * fs / N  = ~206 Hz
 *   - Stop-band attenuation ~ 53 dB
 *   - Linear phase, group delay = (N - 1) / 2 = 63.5 samples (~7.9 ms)
 */
#include "fir_lowpass.h"
#include <math.h>

/* CMSIS-DSP requires a state buffer of numTaps + blockSize - 1 samples. */
static float32_t s_coeffs[FIR_NUM_TAPS];
static float32_t s_state[FIR_NUM_TAPS + FIR_BLOCK_SIZE - 1U];
static arm_fir_instance_f32 s_fir;

/**
 * Windowed-sinc low-pass design.
 * h[i] = sin(wc * m) / (pi * m) * w[i],  m = i - (N-1)/2
 * The coefficients are symmetric, so the time-reversed order that
 * arm_fir_f32 expects is the same as the natural order.
 */
static void fir_design_lowpass(float32_t *h, uint32_t n,
                               float32_t fc, float32_t fs)
{
    const float32_t wc  = 2.0f * PI * fc / fs;          /* rad/sample */
    const float32_t mid = 0.5f * (float32_t)(n - 1U);
    float32_t sum = 0.0f;

    for (uint32_t i = 0U; i < n; i++) {
        const float32_t m = (float32_t)i - mid;
        const float32_t ideal = (fabsf(m) < 1e-6f)
                                ? (wc / PI)
                                : (sinf(wc * m) / (PI * m));
        const float32_t window = 0.54f - 0.46f *
            cosf(2.0f * PI * (float32_t)i / (float32_t)(n - 1U));

        h[i] = ideal * window;
        sum += h[i];
    }

    /* Normalise so the passband (DC) gain is exactly 1. */
    for (uint32_t i = 0U; i < n; i++) {
        h[i] /= sum;
    }
}

void fir_lowpass_init(void)
{
    fir_design_lowpass(s_coeffs, FIR_NUM_TAPS, FIR_CUTOFF_HZ, FIR_FS_HZ);

    arm_fir_init_f32(&s_fir,
                     (uint16_t)FIR_NUM_TAPS,
                     s_coeffs,
                     s_state,
                     FIR_BLOCK_SIZE);
}

void fir_lowpass_process(float32_t *in, float32_t *out)
{
    arm_fir_f32(&s_fir, in, out, FIR_BLOCK_SIZE);
}

const float32_t *fir_lowpass_coeffs(void)
{
    return s_coeffs;
}



