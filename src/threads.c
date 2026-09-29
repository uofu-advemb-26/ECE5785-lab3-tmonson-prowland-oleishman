#include <stdio.h>
#include <FreeRTOS.h>
#include <semphr.h>
#include <task.h>
#include <pico/stdlib.h>
#include <pico/multicore.h>
#include <pico/cyw43_arch.h>

#include "testedFuncs.h"

#define MAIN_TASK_PRIORITY (tskIDLE_PRIORITY + 1UL)
#define MAIN_TASK_STACK_SIZE configMINIMAL_STACK_SIZE

#define SIDE_TASK_PRIORITY (tskIDLE_PRIORITY + 1UL)
#define SIDE_TASK_STACK_SIZE configMINIMAL_STACK_SIZE

int counter;
int on;
SemaphoreHandle_t semaphore;

void side_thread(void *params) {
  while (1) {
    side_thread_func(&counter, &semaphore);

    vTaskDelay(100);
  }
}

void main_thread(void *params) {
  while (1) {
    main_thread_func(&counter, &on, &semaphore);

    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, on);

    vTaskDelay(100);
  }
}

int main(void) {
  stdio_init_all();
  hard_assert(cyw43_arch_init() == PICO_OK);
  on = false;
  counter = 0;
  TaskHandle_t main, side;
  semaphore = xSemaphoreCreateCounting(1, 1);
  xTaskCreate(main_thread, "MainThread", MAIN_TASK_STACK_SIZE, NULL,
              MAIN_TASK_PRIORITY, &main);
  xTaskCreate(side_thread, "SideThread", SIDE_TASK_STACK_SIZE, NULL,
              SIDE_TASK_PRIORITY, &side);
  vTaskStartScheduler();
  return 0;
}
