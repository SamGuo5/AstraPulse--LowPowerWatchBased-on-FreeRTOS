#include "app.hpp"
#include "bsp.h"
#include "watch_ui.hpp"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "climate_service.hpp"
#include "health_service.hpp"
#include "time_service.hpp"
#include "power_service.hpp"
#include "alarm_service.hpp"
#include "motion_service.hpp"
#include "settings_service.hpp"
#include "storage_service.hpp"
#include "profile_service.hpp"
#include "sound_service.hpp"
#include "display_settings_service.hpp"
#include "system_monitor_service.hpp"
#include "power_policy_service.hpp"

namespace {
QueueHandle_t input_queue;
StaticQueue_t input_queue_control;
uint8_t input_queue_storage[16*sizeof(BspKey)];
StaticTask_t input_control,ui_control,idle_control;
StackType_t input_stack[512],ui_stack[2304],idle_stack[configMINIMAL_STACK_SIZE];
WatchUi ui;
TaskHandle_t input_task_handle{};

void input_task(void *) {
  BspKey candidate=BSP_KEY_NONE,stable=BSP_KEY_NONE;uint8_t count=0;
  for(;;) {
    const bool sleeping=power_policy_is_screen_sleeping();
    if(sleeping){
      /* Remove notifications generated while ordinary polling was active,
         then rely on the falling-edge EXTI interrupt instead of a 20 ms poll. */
      (void)ulTaskNotifyTake(pdTRUE,0);
      BspKey raw=bsp_key_raw();
      if(raw==BSP_KEY_NONE){
        (void)ulTaskNotifyTake(pdTRUE,portMAX_DELAY);
        vTaskDelay(pdMS_TO_TICKS(20));raw=bsp_key_raw();
      }
      if(raw!=BSP_KEY_NONE&&stable!=raw){
        stable=raw;candidate=raw;count=2u;(void)xQueueSend(input_queue,&stable,0);
        /* Give the lower-priority UI task time to leave screen-sleep state. */
        vTaskDelay(pdMS_TO_TICKS(50));
      }
      continue;
    }
    const uint8_t stable_count=sleeping?2u:5u;
    BspKey raw=bsp_key_raw();
    if(raw==candidate){if(count<stable_count)count++;}else{candidate=raw;count=0;}
    if(count==stable_count && stable!=candidate){stable=candidate;if(stable!=BSP_KEY_NONE)xQueueSend(input_queue,&stable,0);}
    vTaskDelay(pdMS_TO_TICKS(sleeping?20u:5u));
  }
}

void ui_task(void *) {
  ui.begin();TickType_t frame_wake=xTaskGetTickCount();
  for(;;) {
    BspKey event{};
    if(power_policy_is_screen_sleeping()){
      /* No periodic framebuffer work while the panel is off. A key unblocks
         this wait immediately; the one-second timeout lets an alarm wake UI. */
      if(xQueueReceive(input_queue,&event,pdMS_TO_TICKS(1000))==pdTRUE)ui.handle(event);
      while(xQueueReceive(input_queue,&event,0)==pdTRUE)ui.handle(event);
      ui.render();frame_wake=xTaskGetTickCount();continue;
    }
    while(xQueueReceive(input_queue,&event,0)==pdTRUE)ui.handle(event);
    ui.render();
    /* Keep OLED transfer time inside the 33 ms budget. Waiting 33 ms after
       each transfer nearly halved the launcher's effective frame rate. */
    xTaskDelayUntil(&frame_wake,pdMS_TO_TICKS(33));
  }
}
}

extern "C" volatile const char *app_fault_reason;
volatile const char *app_fault_reason;
extern "C" void app_panic(const char *reason){app_fault_reason=reason;bsp_led_set(1);__disable_irq();for(;;)__NOP();}
extern "C" void HardFault_Handler(void){app_panic("HardFault");}
extern "C" void MemManage_Handler(void){app_panic("MemManage");}
extern "C" void BusFault_Handler(void){app_panic("BusFault");}
extern "C" void UsageFault_Handler(void){app_panic("UsageFault");}
extern "C" void SecureFault_Handler(void){app_panic("SecureFault");}
extern "C" void vApplicationStackOverflowHook(TaskHandle_t, char *name){app_panic(name);}
extern "C" void bsp_key_wakeup_callback(void){
  if(input_task_handle==nullptr)return;
  BaseType_t wake=pdFALSE;vTaskNotifyGiveFromISR(input_task_handle,&wake);portYIELD_FROM_ISR(wake);
}
extern "C" void vApplicationIdleHook(void){
  /* Screen-off idle is handled by vPortSuppressTicksAndSleep/STOP2. */
  if(!power_policy_is_screen_sleeping()){__DSB();__WFI();}
}
extern "C" void vApplicationGetIdleTaskMemory(StaticTask_t **control,StackType_t **stack,uint32_t *size){
  *control=&idle_control;*stack=idle_stack;*size=configMINIMAL_STACK_SIZE;
}
void app_start() {
  input_queue=xQueueCreateStatic(16,sizeof(BspKey),input_queue_storage,&input_queue_control);configASSERT(input_queue);
  storage_service_start();
  time_service_start();
  settings_service_start();
  profile_service_start();
  display_settings_service_start();
  sound_service_start();
  alarm_service_start();
  power_service_start();
  climate_service_start();
  health_service_start();
  motion_service_start();
  input_task_handle=xTaskCreateStatic(input_task,"input",512,nullptr,3,input_stack,&input_control);
  TaskHandle_t display=xTaskCreateStatic(ui_task,"ui",2304,nullptr,1,ui_stack,&ui_control);
  configASSERT(input_task_handle);configASSERT(display);
  system_monitor_register(input_task_handle);system_monitor_register(display);
  system_monitor_start();
  vTaskStartScheduler();
}
