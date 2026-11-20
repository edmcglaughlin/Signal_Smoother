/**
 * smoother.h - "Signal Smoother" real-time DSP pipeline (public interface).
 *
 * Each frame the pipeline:
 *   1. generates a block of a noisy test signal (a tone plus random noise),
 *   2. runs it through a low-pass FIR filter to smooth it,
 *   3. measures the output level (RMS) and reports it.
 *
 * One frame must finish within SM_PERIOD_MS, or the frame is "late" and
 * is counted as a miss. The whole point of the lab is to make every frame
 * finish comfortably inside that budget. All timings are in microseconds.
 */
#ifndef SMOOTHER_H
#define SMOOTHER_H

#include <stdint.h>

/* ---- Signal / analysis configuration ------------------------------- */
#define SM_FS_HZ       8000U     /* sample rate of the test signal       */
#define SM_BLOCK       512U      /* samples processed per frame          */
#define SM_TAPS        128U      /* FIR filter length                    */
#define SM_TONE_HZ     300.0     /* frequency of the clean tone          */
#define SM_CUTOFF_HZ   800.0     /* low-pass cutoff                      */

/* ---- Real-time budget ---------------------------------------------- */
#define SM_PERIOD_MS   10U       /* one frame every 10 ms -> the deadline */

/* Results, exposed as globals so you can read them in the Keil Watch
 * window. All us_* fields are microseconds. */
typedef struct {
    uint32_t us_generate;    /* microseconds in stage 1 (last frame)     */
    uint32_t us_filter;      /* microseconds in stage 2 (last frame)     */
    uint32_t us_level;       /* microseconds in stage 3 (last frame)     */
    uint32_t us_total;       /* microseconds for the whole frame         */
    uint32_t us_worst;       /* worst us_total seen so far               */
    uint32_t frames;         /* frames processed                         */
    uint32_t misses;         /* frames that blew the SM_PERIOD_MS budget */
    float    level;          /* most recent output level (RMS)           */
} SmootherStats;

extern volatile SmootherStats g_sm;
extern uint32_t g_deadline_us;   /* SM_PERIOD_MS expressed in microseconds */

void Smoother_Init(void);        /* set up tables, clear stats            */
void Smoother_RunFrame(void);    /* generate -> filter -> level (timed)   */

#endif /* SMOOTHER_H */
