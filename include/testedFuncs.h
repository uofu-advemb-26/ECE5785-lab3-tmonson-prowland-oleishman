#pragma once

#include <stdbool.h>
#include <FreeRTOS.h>
#include <semphr.h>

bool side_thread_func(int *counter, SemaphoreHandle_t semaphore,
                      TickType_t timeout);
bool main_thread_func(int *counter, int *on, SemaphoreHandle_t semaphore,
                      TickType_t timeout);
