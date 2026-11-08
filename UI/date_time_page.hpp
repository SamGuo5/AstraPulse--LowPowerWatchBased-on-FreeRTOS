#pragma once

#include "feature_page.hpp"
#include "time_service.hpp"

struct HomeSnapshot;

class DateTimePage final : public FeaturePage {
 public:
  void bind(const HomeSnapshot *snapshot){snapshot_=snapshot;}
  void enter(uint32_t now_ms) override;
  void exit(uint32_t now_ms) override;
  void handle(BspKey event,uint32_t now_ms) override;
  void render(uint32_t now_ms) override;

 private:
  enum class Field:uint8_t { Year,Month,Day,Hour,Minute };
  void begin_edit();
  void adjust(int direction);
  const HomeSnapshot *snapshot_{};
  TimeSnapshot edit_{};
  Field field_{Field::Year};
  bool editing_{};
};
