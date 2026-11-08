#pragma once

#include "feature_page.hpp"
#include "profile_service.hpp"

class PersonalInfoPage final : public FeaturePage {
 public:
  void enter(uint32_t now_ms) override;
  void exit(uint32_t now_ms) override;
  void handle(BspKey event,uint32_t now_ms) override;
  void render(uint32_t now_ms) override;

 private:
  PersonalProfile edit_{};
  uint8_t selected_{};
  bool editing_{};
};
