#pragma once

#include "FreeRTOS.h"
#include "task.h"
#include <cstdint>

struct SystemMonitorSnapshot {
  uint16_t minimum_stack_words;
  uint8_t registered_tasks;
  bool stack_ok;
  bool watchdog_active;
  bool recovered_from_watchdog;
};

void system_monitor_register(TaskHandle_t handle);
void system_monitor_start();
SystemMonitorSnapshot system_monitor_snapshot();
