#pragma once

#include <cstdint>

struct PowerPolicySnapshot {
  bool screen_sleeping;
  uint32_t sleep_count;
  uint32_t deep_sleep_count;
  uint32_t deep_sleep_ms;
};

void power_policy_set_screen_sleeping(bool sleeping);
bool power_policy_is_screen_sleeping();
PowerPolicySnapshot power_policy_snapshot();
void power_policy_record_deep_sleep(uint32_t elapsed_ms);
