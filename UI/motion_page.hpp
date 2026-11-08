#pragma once
#include "feature_page.hpp"

class MotionPage final : public FeaturePage {
 public:
  void enter(uint32_t now_ms) override;
  void handle(BspKey event,uint32_t now_ms) override;
  void render(uint32_t now_ms) override;
 private:
  bool details_{};
  uint32_t entered_at_{};
};
