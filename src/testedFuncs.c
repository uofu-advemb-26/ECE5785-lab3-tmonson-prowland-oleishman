#include <stdio.h>

#include "testedFuncs.h"

bool main_thread_func(int *counter, int *on, SemaphoreHandle_t semaphore,
                      TickType_t timeout) {

  if (xSemaphoreTake(semaphore, timeout) == pdFALSE)
    return false;
  {
    printf("hello world from %s! Count %d\n", "main", (*counter)++);
    (*on) = !(*on);
  }
  xSemaphoreGive(semaphore);

  return true;
}

bool side_thread_func(int *counter, SemaphoreHandle_t semaphore,
                      TickType_t timeout) {

  if (xSemaphoreTake(semaphore, timeout) == pdFALSE)
    return false;
  {
    (*counter) += 1;
    printf("hello world from %s! Count %d\n", "thread", *counter);
  }
  xSemaphoreGive(semaphore);

  return true;
}
