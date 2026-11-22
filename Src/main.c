/**
 * main.c - "Signal Smoother" lab, STM32F407 Discovery, Keil RTX5.
 *
 * One RTX5 thread runs the DSP pipeline once per SM_PERIOD_MS and shows
 * the output level on the four user LEDs. printf is retargeted to the
 * debugger (ITM/SWO or a UART) in your project settings.
 *
 * Clocks: HSE 8 MHz -> SYSCLK 168 MHz (AHB 168, APB1 42, APB2 84).
 */
#include "stm32f4xx_hal.h"
#include "cmsis_os2.h"
#include "smoother.h"
#include "micros.h"
#include <stdio.h>

#define LED_GREEN   GPIO_PIN_12
#define LED_ORANGE  GPIO_PIN_13
#define LED_RED     GPIO_PIN_14
#define LED_BLUE    GPIO_PIN_15
#define LED_PORT    GPIOD

static void SystemClock_Config(void);
static void LED_Init(void);
void Error_Handler(void);

uint32_t scaled = 0;

/* Light 0..4 LEDs in proportion to the signal level. */
static void display_level(float level)
{
    uint32_t bars = (uint32_t)(level * 6.0f);   /* scale to roughly 0..4 */
	  bars = (uint32_t)((level - 0.56f) * 500.0f);
	  scaled = bars;
	
    if (bars > 4U) bars = 4U;
    HAL_GPIO_WritePin(LED_PORT, LED_GREEN,  bars >= 1U ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LED_PORT, LED_ORANGE, bars >= 2U ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LED_PORT, LED_RED,    bars >= 3U ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LED_PORT, LED_BLUE,   bars >= 4U ? GPIO_PIN_SET : GPIO_PIN_RESET);
}





uint32_t run_frame;
uint32_t loop_time;

static void smoother_thread(void *arg)
{
    (void)arg;
	  
	  uint32_t sm_period_ticks = (SM_PERIOD_MS * osKernelGetTickFreq()) / 1000U;
	  uint32_t top; 
    for (;;) {
        uint32_t start = osKernelGetTickCount();
        uint32_t next_wake_tick = osKernelGetTickCount() + sm_period_ticks;;
				top = Micros_Now();

				Smoother_RunFrame();

        /* A debug print in the hot path: performance smell "your own probes".
         * Left commented because, depending on how STDOUT is retargeted, it can
         * block or halt the thread (semihosting breakpoint, or a full ITM FIFO
         * when trace is on). To experience the smell, uncomment it ONLY with
         * STDOUT routed to ITM and the debugger's Trace left OFF. Measuring is
         * done in the Watch window, so you do not need it. */
        /* printf("level = %f   frame = %lu us   misses = %lu\n",
                  g_sm.level, (unsigned long)g_sm.us_total,
                  (unsigned long)g_sm.misses); */

        display_level(g_sm.level);
			  run_frame = Micros_Since(top);
								
        // Sleep until that exact tick is reached
        osDelayUntil(next_wake_tick);
			  loop_time = Micros_Since(top);
    }
}

int main(void)
{
    HAL_Init();
    SystemClock_Config();
  	FLASH->ACR |= FLASH_ACR_PRFTEN | FLASH_ACR_ICEN | FLASH_ACR_DCEN;

    SystemCoreClockUpdate();
    LED_Init();
    Micros_Init();
    Smoother_Init();

    osKernelInitialize();

    const osThreadAttr_t attr = {
        .name       = "smoother",
        .stack_size = 2048U,
        .priority   = osPriorityNormal,
    };
    osThreadNew(smoother_thread, NULL, &attr);

    osKernelStart();
    for (;;) { }   /* never reached */
}

/* ------------------------------------------------------------------ */
static void SystemClock_Config(void)
{
    RCC_OscInitTypeDef osc = {0};
    RCC_ClkInitTypeDef clk = {0};

    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

    osc.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    osc.HSEState       = RCC_HSE_ON;
    osc.PLL.PLLState   = RCC_PLL_ON;
    osc.PLL.PLLSource  = RCC_PLLSOURCE_HSE;
    osc.PLL.PLLM       = 8;
    osc.PLL.PLLN       = 336;
    osc.PLL.PLLP       = RCC_PLLP_DIV2;
    osc.PLL.PLLQ       = 7;
    if (HAL_RCC_OscConfig(&osc) != HAL_OK) Error_Handler();

    clk.ClockType      = RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK |
                         RCC_CLOCKTYPE_PCLK1  | RCC_CLOCKTYPE_PCLK2;
    clk.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
    clk.AHBCLKDivider  = RCC_SYSCLK_DIV1;
    clk.APB1CLKDivider = RCC_HCLK_DIV4;
    clk.APB2CLKDivider = RCC_HCLK_DIV2;
    if (HAL_RCC_ClockConfig(&clk, FLASH_LATENCY_5) != HAL_OK) Error_Handler();
}

static void LED_Init(void)
{
    GPIO_InitTypeDef gpio = {0};
    __HAL_RCC_GPIOD_CLK_ENABLE();
    gpio.Pin   = LED_GREEN | LED_ORANGE | LED_RED | LED_BLUE;
    gpio.Mode  = GPIO_MODE_OUTPUT_PP;
    gpio.Pull  = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(LED_PORT, &gpio);
}

void Error_Handler(void)
{
    __disable_irq();
    __HAL_RCC_GPIOD_CLK_ENABLE();
    HAL_GPIO_WritePin(LED_PORT, LED_RED, GPIO_PIN_SET);
    for (;;) { }
}
