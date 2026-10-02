/**
 * Project: ZeroHAL
 * File: os_scheduler.h
 * Description: Cooperative Task Scheduler
 */
#ifndef OS_SCHEDULER_H
#define OS_SCHEDULER_H

#include <stdint.h>

#define MAX_TASKS 8

typedef struct {
    void (*task_func)(void); // Function pointer to the task
    uint32_t interval_ms;    // Run interval in milliseconds
    uint32_t last_run_ms;    // Timestamp of last execution
    uint8_t active;          // Task state
} os_task_t;

void os_scheduler_init(void);
int os_add_task(void (*func)(void), uint32_t interval);
void os_run_scheduler(void);

#endif /* OS_SCHEDULER_H */
