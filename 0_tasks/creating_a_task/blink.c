/*
 * Ngenda Henry, June 2020
 *
 * This work is placed in the public domain. No warranty, or guarantee
 * is expressed or implied. When you use this source code, you do so
 * with full responsibility and at your own risk.
 */

#include <FreeRTOS.h>
#include <task.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>


void
vApplicationStackOverflowHook(xTaskHandle *pxTask,signed portCHAR *pcTaskName)
{
  (void)pxTask;
  (void)pcTaskName;
  for(;;);
}

static void clock_setup(void)
{
  rcc_clock_setup_pll(&rcc_hse_8mhz_3v3[RCC_CLOCK_3V3_168MHZ]);
  rcc_periph_clock_enable(RCC_GPIOD);
}

static void gpio_setup(void)
{
  gpio_mode_setup(GPIOD, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO12);
}

static void blinkTask(void *args) {
  const portTickType xDelay = 2*portTICK_RATE_MS;
  while (1) {
    gpio_set(GPIOD, GPIO12);
    vTaskDelay( xDelay );
    gpio_clear(GPIOD, GPIO12);
    vTaskDelay( xDelay );
  }
}

int main(void)
{
  clock_setup();
  gpio_setup();

  xTaskCreate(blinkTask, "blink", 100, NULL, configMAX_PRIORITIES-1, NULL);
  vTaskStartScheduler();
  while(1);
  return 0;
}
