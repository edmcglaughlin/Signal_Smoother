/**
 * micros.h - free-running microsecond timer using TIM2.
 *
 * TIM2 is a 32-bit timer on APB1. With this project's clock tree the timer
 * clock is 84 MHz; we prescale it to tick once per microsecond, so TIM2->CNT
 * is a running microsecond count that wraps about every 71 minutes.
 *
 * Why TIM2 and not the Cortex-M4 DWT cycle counter: the DWT lives in the
 * core's private debug space and only works when the debug/trace unit is
 * powered and enabled, which is not reliable across ST-Link setups on the
 * Discovery board. TIM2 lives in normal peripheral space and always works.
 *
 * Call Micros_Init() once at start-up, then bracket code with
 * Micros_Now() / Micros_Since() to get elapsed microseconds.
 */
#ifndef MICROS_H
#define MICROS_H

#include "stm32f4xx.h"
#include <stdint.h>

#define TIMER_INPUT_HZ   84000000U                          /* APB1 timer clock */
#define US_PRESCALER     ((TIMER_INPUT_HZ / 1000000U) - 1U) /* -> 1 MHz, 1 us   */

static inline void Micros_Init(void)
{
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;   /* clock TIM2                */
    (void)RCC->APB1ENR;                   /* short delay after enable  */
    TIM2->PSC = US_PRESCALER;             /* tick once per microsecond */
    TIM2->ARR = 0xFFFFFFFFU;              /* free-running 32-bit        */
    TIM2->EGR = TIM_EGR_UG;               /* load PSC/ARR               */
    TIM2->CR1 = TIM_CR1_CEN;              /* start counting             */
}

static inline uint32_t Micros_Now(void)
{
    return TIM2->CNT;
}

/* Elapsed microseconds since 'start'. Correct across one wrap because the
 * subtraction is done in unsigned 32-bit arithmetic. */
static inline uint32_t Micros_Since(uint32_t start)
{
    return TIM2->CNT - start;
}

#endif /* MICROS_H */
