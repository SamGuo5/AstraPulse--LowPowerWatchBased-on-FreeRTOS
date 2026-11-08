#pragma once
#include "home_view_model.hpp"
#include <cstdint>

class WatchFace final {
 public:
  void render(const HomeSnapshot &snapshot) const;
};
