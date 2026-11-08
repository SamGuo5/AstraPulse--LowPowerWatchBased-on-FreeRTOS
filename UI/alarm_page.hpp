#pragma once

#include "alarm_service.hpp"
#include "feature_page.hpp"

class AlarmPage final : public FeaturePage {
 public:
  void enter(uint32_t now_ms) override;
  void exit(uint32_t now_ms) override;
  void handle(BspKey event,uint32_t now_ms) override;
  void render(uint32_t now_ms) override;
  void render_ringing(uint32_t now_ms,const AlarmSnapshot &snapshot) const;

 private:
  enum class Field:uint8_t { Hour,Minute,Enabled };
  void adjust(int direction);
  uint8_t selected_{};
  Field field_{Field::Hour};
  AlarmConfig edit_{};
  bool editing_{};
};
