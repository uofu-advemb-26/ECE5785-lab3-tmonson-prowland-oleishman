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

int counter;
int on;
SemaphoreHandle_t semaphore;

void setUp(void) {
}

void tearDown(void) {
}

void test_setup() {
}

void test_mainThread() {
  semaphore = xSemaphoreCreateCounting(1, 1);
  counter = 0;
  on = 0;

  TEST_ASSERT_TRUE_MESSAGE(main_thread_func(&counter, &on, semaphore, 0) ==
                               true,
                           "Main thread failed to take semaphore.");

  TEST_ASSERT_TRUE_MESSAGE(counter == 1,
                           "Main thread didn't increment count.");

  TEST_ASSERT_TRUE_MESSAGE(on == 1, "Main thread didn't toggle 'on'.");
}

void test_mainThreadSemaphore() {
  semaphore = xSemaphoreCreateCounting(1, 1);
  counter = 0;
  on = 0;

  TEST_ASSERT_TRUE_MESSAGE(xSemaphoreTake(semaphore, 0) == pdTRUE,
                           "Tester failed to setup semaphore.");

  TEST_ASSERT_TRUE_MESSAGE(main_thread_func(&counter, &on, semaphore, 0) ==
                               false,
                           "Main Thread still took the semaphore.");

  TEST_ASSERT_TRUE_MESSAGE(counter == 0,
                           "Main thread still incremented count.");

  TEST_ASSERT_TRUE_MESSAGE(on == 0, "Main thread still toggled 'on'.");
}

void test_sideThread() {
  semaphore = xSemaphoreCreateCounting(1, 1);
  counter = 0;

  TEST_ASSERT_TRUE_MESSAGE(side_thread_func(&counter, semaphore, 0) == true,
                           "Side thread failed to take semaphore.");

  TEST_ASSERT_TRUE_MESSAGE(counter == 1,
                           "Side thread didn't increment count.");
}

void test_sideThreadSemaphore() {
  semaphore = xSemaphoreCreateCounting(1, 1);
  counter = 0;

  TEST_ASSERT_TRUE_MESSAGE(xSemaphoreTake(semaphore, 0) == pdTRUE,
                           "Tester failed to setup semaphore.");

  TEST_ASSERT_TRUE_MESSAGE(side_thread_func(&counter, semaphore, 0) == false,
                           "Side Thread still took the semaphore.");

  TEST_ASSERT_TRUE_MESSAGE(counter == 0,
                           "Side thread still incremented count.");
}

void test_variable_assignment() {
  int x = 1;
  TEST_ASSERT_TRUE_MESSAGE(x == 1, "Variable assignment failed.");
}

void test_multiplication(void) {
  int x = 30;
  int y = 6;
  int z = x / y;
  TEST_ASSERT_TRUE_MESSAGE(
      z == 5, "Multiplication of two integers returned incorrect value.");
}

void runner_thread(void *params) {
  while (1) {
    vTaskDelay(pdMS_TO_TICKS(5000)); // Give time for TTY to attach.
    printf("Start tests\n");
    UNITY_BEGIN();

    RUN_TEST(test_variable_assignment);
    RUN_TEST(test_multiplication);

    RUN_TEST(test_mainThread);
    RUN_TEST(test_mainThreadSemaphore);

    RUN_TEST(test_sideThread);
    RUN_TEST(test_sideThreadSemaphore);
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
