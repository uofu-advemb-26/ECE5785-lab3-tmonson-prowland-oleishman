bool side_thread_func(int *counter, SemaphoreHandle_t *semaphore);
bool main_thread_func(int *counter, int *on, SemaphoreHandle_t *semaphore);
