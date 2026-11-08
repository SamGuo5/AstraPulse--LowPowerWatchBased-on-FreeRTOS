#pragma once
#include "feature_page.hpp"

class ClimatePage final : public FeaturePage {
 public:
  void handle(BspKey event,uint32_t now_ms) override;
  void render(uint32_t now_ms) override;

 private:
  bool fahrenheit_{};
};
