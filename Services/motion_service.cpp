#include "motion_service.hpp"

#include "FreeRTOS.h"
#include "bsp.h"
#include "mpu6050.h"
#include "queue.h"
#include "task.h"
#include "system_monitor_service.hpp"
#include "power_policy_service.hpp"
#include <algorithm>
#include <cmath>
#include <cstdlib>

namespace {
struct RawSample {Mpu6050Sample value;TickType_t tick;};
MotionSnapshot snapshot{};
QueueHandle_t sample_queue;
StaticQueue_t queue_control;
uint8_t queue_storage[32*sizeof(RawSample)];
StaticTask_t acquisition_control,processing_control;
StackType_t acquisition_stack[512],processing_stack[640];

void device_state(MotionDeviceState state,uint8_t id,bool error){
  const BspI2c4Diagnostics diagnostics=bsp_i2c4_diagnostics();
  const uint8_t ack_address=mpu6050_last_ack_address();
  taskENTER_CRITICAL();snapshot.device_state=state;snapshot.device_id=id;snapshot.ack_address=ack_address;
  snapshot.i2c_initialized=diagnostics.initialized;snapshot.scl_high=diagnostics.scl_high;
  snapshot.sda_high=diagnostics.sda_high;snapshot.enable_high=diagnostics.enable_high;
  snapshot.i2c_error=diagnostics.hal_error;
  snapshot.connected=state==MotionDeviceState::Running;
  if(error&&snapshot.error_count<0xffffu)++snapshot.error_count;
  taskEXIT_CRITICAL();
}
void queue_sample(const RawSample &sample){
  if(xQueueSend(sample_queue,&sample,0)==pdTRUE)return;
  RawSample discarded{};(void)xQueueReceive(sample_queue,&discarded,0);
  (void)xQueueSend(sample_queue,&sample,0);
}
void acquisition_task(void *){
  uint8_t recovery_count=0;
  vTaskDelay(pdMS_TO_TICKS(150));
  for(;;){
    if(power_policy_is_screen_sleeping()){vTaskDelay(pdMS_TO_TICKS(1000));continue;}
    uint8_t id=0;device_state(MotionDeviceState::Starting,0,false);
    if(!mpu6050_detect(&id)){
      device_state(MotionDeviceState::Offline,0,true);
      if(++recovery_count>=3u){(void)bsp_i2c4_recover();recovery_count=0;}
      vTaskDelay(pdMS_TO_TICKS(2000));continue;
    }
    if(!mpu6050_device_supported(id)){
      device_state(MotionDeviceState::Unsupported,id,true);
      vTaskDelay(pdMS_TO_TICKS(2000));continue;
    }
    if(!mpu6050_configure()){
      device_state(MotionDeviceState::Fault,id,true);(void)bsp_i2c4_recover();
      vTaskDelay(pdMS_TO_TICKS(1000));continue;
    }
    recovery_count=0;device_state(MotionDeviceState::Running,id,false);
    TickType_t wake=xTaskGetTickCount();uint8_t failures=0;
    for(;;){
      if(power_policy_is_screen_sleeping()){
        vTaskDelay(pdMS_TO_TICKS(1000));wake=xTaskGetTickCount();continue;
      }
      RawSample sample{};sample.tick=xTaskGetTickCount();
      if(mpu6050_read(&sample.value)){failures=0;queue_sample(sample);}
      else{
        taskENTER_CRITICAL();if(snapshot.error_count<0xffffu)++snapshot.error_count;taskEXIT_CRITICAL();
        if(++failures>=3u){device_state(MotionDeviceState::Fault,id,false);break;}
      }
      xTaskDelayUntil(&wake,pdMS_TO_TICKS(10));
    }
    (void)bsp_i2c4_recover();vTaskDelay(pdMS_TO_TICKS(500));
  }
}

void processing_task(void *){
  int32_t gravity_mg=1000,activity_mg=0;bool armed=true;
  TickType_t last_peak=0,last_step=0;uint8_t cadence=0;
  for(;;){
    RawSample sample{};
    if(xQueueReceive(sample_queue,&sample,portMAX_DELAY)!=pdTRUE)continue;
    const int32_t ax=(int32_t)sample.value.ax*1000/8192;
    const int32_t ay=(int32_t)sample.value.ay*1000/8192;
    const int32_t az=(int32_t)sample.value.az*1000/8192;
    const double square=(double)sample.value.ax*sample.value.ax+(double)sample.value.ay*sample.value.ay+
                        (double)sample.value.az*sample.value.az;
    const int32_t magnitude=(int32_t)(std::sqrt(square)*1000.0/8192.0);
    gravity_mg+=(magnitude-gravity_mg)/50;
    const int32_t linear=std::abs(magnitude-gravity_mg);
    activity_mg+=(linear-activity_mg)/8;
    const int32_t threshold=160+std::min<int32_t>(100,activity_mg/3);
    uint32_t add_steps=0;
    if(linear<70)armed=true;
    if(armed&&linear>threshold){
      const TickType_t interval=sample.tick-last_peak;
      if(last_peak!=0&&interval>=pdMS_TO_TICKS(280)&&interval<=pdMS_TO_TICKS(1500)){
        if(cadence==1u){add_steps=2u;cadence=2u;}else{add_steps=1u;if(cadence<255u)++cadence;}
        last_step=sample.tick;
      }else cadence=1u;
      last_peak=sample.tick;armed=false;
    }
    if(last_peak!=0&&sample.tick-last_peak>pdMS_TO_TICKS(2000))cadence=0;
    const uint8_t activity=(uint8_t)std::min<int32_t>(100,activity_mg*100/350);
    MotionLevel level=MotionLevel::Still;
    if(last_step!=0&&sample.tick-last_step<pdMS_TO_TICKS(2000))level=MotionLevel::Walking;
    else if(activity>=55u)level=MotionLevel::Active;
    taskENTER_CRITICAL();
    snapshot.steps+=add_steps;++snapshot.sample_count;
    snapshot.ax_mg=(int16_t)std::max<int32_t>(-32768,std::min<int32_t>(32767,ax));
    snapshot.ay_mg=(int16_t)std::max<int32_t>(-32768,std::min<int32_t>(32767,ay));
    snapshot.az_mg=(int16_t)std::max<int32_t>(-32768,std::min<int32_t>(32767,az));
    snapshot.magnitude_mg=(uint16_t)std::max<int32_t>(0,std::min<int32_t>(65535,magnitude));
    snapshot.activity_percent=activity;snapshot.level=level;
    taskEXIT_CRITICAL();
  }
}
}

void motion_service_start(){
  taskENTER_CRITICAL();snapshot.device_state=MotionDeviceState::Starting;snapshot.level=MotionLevel::Still;taskEXIT_CRITICAL();
  sample_queue=xQueueCreateStatic(32,sizeof(RawSample),queue_storage,&queue_control);configASSERT(sample_queue);
  TaskHandle_t acquisition=xTaskCreateStatic(acquisition_task,"motion_io",512,nullptr,4,acquisition_stack,&acquisition_control);
  TaskHandle_t processing=xTaskCreateStatic(processing_task,"motion",640,nullptr,2,processing_stack,&processing_control);
  configASSERT(acquisition);configASSERT(processing);
  system_monitor_register(acquisition);system_monitor_register(processing);
}
MotionSnapshot motion_service_snapshot(){
  taskENTER_CRITICAL();const MotionSnapshot copy=snapshot;taskEXIT_CRITICAL();return copy;
}
