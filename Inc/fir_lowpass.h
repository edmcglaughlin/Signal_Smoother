/**
 * fir_lowpass.h
 *
 * 128-tap low-pass FIR filter for the STM32F407 using CMSIS-DSP.
 *   Sample rate : 8 kHz
 *   Cutoff      : 800 Hz (-6 dB point of the windowed-sinc design)
 *   Taps        : 128
 *   Block size  : 512 samples
 */
#ifndef FIR_LOWPASS_H
#define FIR_LOWPASS_H

#include "arm_math.h"

#ifndef PI
  #define PI               3.14159265358979f
#endif

#define FIR_FS_HZ        8000.0f
#define FIR_CUTOFF_HZ    800.0f
#define FIR_NUM_TAPS     128U
#define FIR_BLOCK_SIZE   512U

/* Design the coefficients and initialise the CMSIS-DSP instance.
 * Call once at start-up, before fir_lowpass_process(). */
void fir_lowpass_init(void);

/* Filter exactly FIR_BLOCK_SIZE samples. Filter state carries over
 * between calls, so consecutive blocks are filtered seamlessly.
 * in and out may not overlap. */
void fir_lowpass_process(float32_t *in, float32_t *out);

/* Read-only access to the designed coefficients (e.g. for debugging). */
const float32_t *fir_lowpass_coeffs(void);

#endif /* FIR_LOWPASS_H */

