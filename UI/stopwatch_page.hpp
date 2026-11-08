#pragma once

#include "feature_page.hpp"

class StopwatchPage final : public FeaturePage {
 public:
  void handle(BspKey event,uint32_t now_ms) override;
  void render(uint32_t now_ms) override;

 private:
  uint32_t elapsed(uint32_t now_ms) const;
  uint32_t accumulated_ms_{};
  uint32_t running_since_ms_{};
  bool running_{};
};
