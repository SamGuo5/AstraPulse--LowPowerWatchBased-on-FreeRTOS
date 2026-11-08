#pragma once

#include "feature_page.hpp"

class SystemSettingsPage final : public FeaturePage {
 public:
  void enter(uint32_t now_ms) override;
  void handle(BspKey event,uint32_t now_ms) override;
  void render(uint32_t now_ms) override;

 private:
  void change_selected();
  uint8_t selected_{};
};
