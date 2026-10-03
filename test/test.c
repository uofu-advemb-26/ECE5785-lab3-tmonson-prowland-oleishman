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

#define DEADLOCK_PRIORITY (tskIDLE_PRIORITY + 1UL)
#define DEADLOCK_STACK_SIZE (configMINIMAL_STACK_SIZE * 4)

int counter;
int on;
SemaphoreHandle_t semaphore;

SemaphoreHandle_t semaphore_a;
SemaphoreHandle_t semaphore_b;

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


typedef struct {
  SemaphoreHandle_t semaphore_a;
  SemaphoreHandle_t semaphore_b;
  TickType_t timeout;
  uint8_t * state;
} DeadlockConfig_t;

void deadlock_thread(void *params) {

  DeadlockConfig_t *config = (DeadlockConfig_t *)params;

  xSemaphoreTake(config->semaphore_a, config->timeout);

  vTaskDelay(100);

  *(config->state) = 1;

  xSemaphoreTake(config->semaphore_b, config->timeout);  

  *(config->state) = 2;
  xSemaphoreGive(&semaphore_a);
  xSemaphoreGive(&semaphore_b);

}

void test_deadlock() {
  semaphore_a = xSemaphoreCreateCounting(1, 1);
  semaphore_b = xSemaphoreCreateCounting(1, 1);
  TEST_ASSERT_NOT_NULL_MESSAGE(semaphore_a, "Failed to create semaphore.");
  TEST_ASSERT_NOT_NULL_MESSAGE(semaphore_b, "Failed to create semaphore.");
  
  uint8_t state_a;
  uint8_t state_b;

  DeadlockConfig_t config_a = {
    .state = &state_a,
    .semaphore_a = semaphore_a,
    .semaphore_b = semaphore_b,
    .timeout = 1000
  };

  DeadlockConfig_t config_b = {
    .state = &state_b,
    .semaphore_a = semaphore_b,
    .semaphore_b = semaphore_a,
    .timeout = 1000
  };

  TaskHandle_t xDeadlock_a;
  TaskHandle_t xDeadlock_b;

  xTaskCreate(deadlock_thread, "Deadlock_a", DEADLOCK_STACK_SIZE, (void*)&config_a, DEADLOCK_PRIORITY, &xDeadlock_a);
  xTaskCreate(deadlock_thread, "Deadlock_b", DEADLOCK_STACK_SIZE, (void*)&config_b, DEADLOCK_PRIORITY, &xDeadlock_b);

  vTaskDelay(200);

  vTaskDelete(xDeadlock_a);
  vTaskDelete(xDeadlock_b);

  TEST_ASSERT_TRUE_MESSAGE(state_a == 1, "Failed to create deadlock.");
  TEST_ASSERT_TRUE_MESSAGE(state_b == 1, "Failed to create deadlock.");

  vSemaphoreDelete(semaphore_a);
  vSemaphoreDelete(semaphore_b);

}

void orphaned_lock(void)
{
    while (1) {
        xSemaphoreTake(semaphore, portMAX_DELAY);
        counter++;
        if (counter % 2) {
            continue;
        }
        //printk("Count %d\n", counter);
        xSemaphoreGive(semaphore);
    }   
}

void non_orphaned_lock(void)
{
    while (1) {
        xSemaphoreTake(semaphore, portMAX_DELAY);
        counter++;
        if (counter % 2) {
            xSemaphoreGive(semaphore);
            continue;
        }
        //printk("Count %d\n", counter);
        xSemaphoreGive(semaphore);
    }   
}

void test_orphaned_lock(){

  TaskHandle_t xOrphaned_lock;
  xTaskCreate(orphaned_lock, "Orphaned_Lock", DEADLOCK_STACK_SIZE, NULL, DEADLOCK_PRIORITY, &xOrphaned_lock);

  vTaskDelay(10);

  bool result = xSemaphoreTake(semaphore, 100);

  vTaskDelete(xOrphaned_lock);

  TEST_ASSERT_FALSE_MESSAGE(result, "Orphaned Lock not created.");

}

void test_non_orphaned_lock(){

  TaskHandle_t xOrphaned_lock;
  xTaskCreate(non_orphaned_lock, "Orphaned_Lock", DEADLOCK_STACK_SIZE, NULL, DEADLOCK_PRIORITY, &xOrphaned_lock);

  vTaskDelay(10);

  bool result = xSemaphoreTake(semaphore, 100);

  vTaskDelete(xOrphaned_lock);

  TEST_ASSERT_TRUE_MESSAGE(result, "Orphaned lock created.");

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

    RUN_TEST(test_deadlock);
    RUN_TEST(test_orphaned_lock);
    RUN_TEST(test_non_orphaned_lock);

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
