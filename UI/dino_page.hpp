#pragma once

#include "feature_page.hpp"

class DinoPage final : public FeaturePage {
 public:
  void enter(uint32_t now_ms) override;
  void exit(uint32_t now_ms) override;
  void handle(BspKey event,uint32_t now_ms) override;
  void update(uint32_t now_ms) override;
  void render(uint32_t now_ms) override;

 private:
  enum class State:uint8_t { Ready,Running,GameOver };
  void start(uint32_t now_ms);
  void jump();
  void next_obstacle();
  bool collided() const;

  State state_{State::Ready};
  uint32_t last_update_ms_{};
  uint32_t run_time_ms_{};
  uint32_t score_{};
  uint32_t high_score_{};
  int32_t dino_y_q8_{};
  int32_t velocity_y_q8_{};
  int32_t obstacle_x_q8_{};
  uint8_t obstacle_width_{7};
  uint8_t obstacle_height_{12};
  uint16_t random_{0x5a3cu};
  bool grounded_{true};
};
