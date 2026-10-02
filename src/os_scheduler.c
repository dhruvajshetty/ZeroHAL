/**
 * Project: ZeroHAL
 * File: os_scheduler.c
 * Description: Cooperative Task Scheduler Implementation
 */
#include "zero_hal.h"

static os_task_t task_list[MAX_TASKS];

void os_scheduler_init(void) {
    for (int i = 0; i < MAX_TASKS; i++) {
        task_list[i].active = 0;
    }
}

int os_add_task(void (*func)(void), uint32_t interval) {
    for (int i = 0; i < MAX_TASKS; i++) {
        if (!task_list[i].active) {
            task_list[i].task_func = func;
            task_list[i].interval_ms = interval;
            task_list[i].last_run_ms = millis();
            task_list[i].active = 1;
            return i; // Return task ID
        }
    }
    return -1; // OS Task List is full
}

void os_run_scheduler(void) {
    uint32_t current_time = millis();
    for (int i = 0; i < MAX_TASKS; i++) {
        if (task_list[i].active) {
            // Check if it's time to execute the task
            if ((current_time - task_list[i].last_run_ms) >= task_list[i].interval_ms) {
                task_list[i].task_func();
                task_list[i].last_run_ms = current_time; // Update last run timestamp
            }
        }
    }
}
