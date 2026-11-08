#include "bsp.h"
#include "watch_hal.hpp"
#include "app.hpp"
static WatchHal watch_hal;
int main() {
  bsp_init();
  if(!HAL::inject(&watch_hal))app_panic("Astra HAL injection");
  app_start();
  app_panic("scheduler returned");
}
extern "C" void Error_Handler(void){app_panic("STM32 HAL");}
