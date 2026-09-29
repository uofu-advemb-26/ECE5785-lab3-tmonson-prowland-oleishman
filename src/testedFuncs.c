#include <stdio.h>
#include <FreeRTOS.h>
#include <semphr.h>
#include <task.h>
#include <pico/stdlib.h>
#include <pico/multicore.h>
#include <pico/cyw43_arch.h>

#include "testedFuncs.h"

bool main_thread_func(int *counter, int *on,
                      SemaphoreHandle_t *semaphore) {

  if (xSemaphoreTake(*semaphore, 200) == pdFALSE)
    return false;

  printf("hello world from %s! Count %d\n", "main", (*counter)++);
  (*on) = !(*on);
  xSemaphoreGive(*semaphore);

  return true;
}

bool side_thread_func(int *counter, SemaphoreHandle_t *semaphore) {

  if (xSemaphoreTake(*semaphore, 200) == pdFALSE)
    return false;

  (*counter) += 1;
  printf("hello world from %s! Count %d\n", "thread", *counter);
  xSemaphoreGive(*semaphore);

  return true;
}