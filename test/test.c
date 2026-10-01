#include <stdio.h>
#include <pico/stdlib.h>
#include <stdint.h>
#include <unity.h>
#include "unity_config.h"

#include <FreeRTOS.h>
#include <semphr.h>
#include <task.h>

#include "testedFuncs.h"

// The runner gets the highest priority so it can preempt and inspect
// any threads a test creates (needed for the deadlock tests).
#define TEST_RUNNER_PRIORITY (tskIDLE_PRIORITY + 5UL)
#define TEST_RUNNER_STACK_SIZE (configMINIMAL_STACK_SIZE * 4)

#define TEST_TIMEOUT_TICKS 10

int counter;
int on;
SemaphoreHandle_t semaphore;

void setUp(void) {
  semaphore = xSemaphoreCreateCounting(1, 1);
  TEST_ASSERT_NOT_NULL_MESSAGE(semaphore, "Failed to create semaphore.");
  counter = 0;
  on = 0;
}

void tearDown(void) {
  vSemaphoreDelete(semaphore);
}

void test_mainThread() {
  TEST_ASSERT_TRUE_MESSAGE(main_thread_func(&counter, &on, semaphore, 0),
                           "Main thread failed to take semaphore.");

  TEST_ASSERT_EQUAL_INT_MESSAGE(1, counter,
                                "Main thread didn't increment count.");

  TEST_ASSERT_EQUAL_INT_MESSAGE(1, on, "Main thread didn't toggle 'on'.");

  TEST_ASSERT_EQUAL_INT_MESSAGE(1, uxSemaphoreGetCount(semaphore),
                                "Main thread didn't release the semaphore.");
}

void test_mainThreadSemaphore() {
  TEST_ASSERT_TRUE_MESSAGE(xSemaphoreTake(semaphore, 0) == pdTRUE,
                           "Tester failed to setup semaphore.");

  TEST_ASSERT_FALSE_MESSAGE(main_thread_func(&counter, &on, semaphore, 0),
                            "Main Thread still took the semaphore.");

  TEST_ASSERT_EQUAL_INT_MESSAGE(0, counter,
                                "Main thread still incremented count.");

  TEST_ASSERT_EQUAL_INT_MESSAGE(0, on, "Main thread still toggled 'on'.");
}

void test_mainThreadTimeout() {
  TEST_ASSERT_TRUE_MESSAGE(xSemaphoreTake(semaphore, 0) == pdTRUE,
                           "Tester failed to setup semaphore.");

  TickType_t start = xTaskGetTickCount();
  bool result =
      main_thread_func(&counter, &on, semaphore, TEST_TIMEOUT_TICKS);
  TickType_t elapsed = xTaskGetTickCount() - start;

  TEST_ASSERT_FALSE_MESSAGE(result, "Main Thread still took the semaphore.");

  TEST_ASSERT_GREATER_OR_EQUAL_UINT32_MESSAGE(
      TEST_TIMEOUT_TICKS, elapsed,
      "Main thread gave up before the timeout expired.");

  TEST_ASSERT_EQUAL_INT_MESSAGE(0, counter,
                                "Main thread still incremented count.");

  TEST_ASSERT_EQUAL_INT_MESSAGE(0, on, "Main thread still toggled 'on'.");
}

void test_sideThread() {
  TEST_ASSERT_TRUE_MESSAGE(side_thread_func(&counter, semaphore, 0),
                           "Side thread failed to take semaphore.");

  TEST_ASSERT_EQUAL_INT_MESSAGE(1, counter,
                                "Side thread didn't increment count.");

  TEST_ASSERT_EQUAL_INT_MESSAGE(1, uxSemaphoreGetCount(semaphore),
                                "Side thread didn't release the semaphore.");
}

void test_sideThreadSemaphore() {
  TEST_ASSERT_TRUE_MESSAGE(xSemaphoreTake(semaphore, 0) == pdTRUE,
                           "Tester failed to setup semaphore.");

  TEST_ASSERT_FALSE_MESSAGE(side_thread_func(&counter, semaphore, 0),
                            "Side Thread still took the semaphore.");

  TEST_ASSERT_EQUAL_INT_MESSAGE(0, counter,
                                "Side thread still incremented count.");
}

void test_sideThreadTimeout() {
  TEST_ASSERT_TRUE_MESSAGE(xSemaphoreTake(semaphore, 0) == pdTRUE,
                           "Tester failed to setup semaphore.");

  TickType_t start = xTaskGetTickCount();
  bool result = side_thread_func(&counter, semaphore, TEST_TIMEOUT_TICKS);
  TickType_t elapsed = xTaskGetTickCount() - start;

  TEST_ASSERT_FALSE_MESSAGE(result, "Side Thread still took the semaphore.");

  TEST_ASSERT_GREATER_OR_EQUAL_UINT32_MESSAGE(
      TEST_TIMEOUT_TICKS, elapsed,
      "Side thread gave up before the timeout expired.");

  TEST_ASSERT_EQUAL_INT_MESSAGE(0, counter,
                                "Side thread still incremented count.");
}

void runner_thread(void *params) {
  while (1) {
    vTaskDelay(pdMS_TO_TICKS(5000)); // Give time for TTY to attach.
    printf("Start tests\n");
    UNITY_BEGIN();

    RUN_TEST(test_mainThread);
    RUN_TEST(test_mainThreadSemaphore);
    RUN_TEST(test_mainThreadTimeout);

    RUN_TEST(test_sideThread);
    RUN_TEST(test_sideThreadSemaphore);
    RUN_TEST(test_sideThreadTimeout);
    vTaskDelay(pdMS_TO_TICKS(5000));
    UNITY_END();
  }
}

int main(void) {
  stdio_init_all();
  xTaskCreate(runner_thread, "TestRunner", TEST_RUNNER_STACK_SIZE, NULL,
              TEST_RUNNER_PRIORITY, NULL);
  vTaskStartScheduler();
  return 0;
}
