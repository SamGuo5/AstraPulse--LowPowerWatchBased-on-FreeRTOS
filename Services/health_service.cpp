#include "health_service.hpp"

#include "FreeRTOS.h"
#include "max30102.h"
#include "queue.h"
#include "task.h"
#include "system_monitor_service.hpp"
#include "power_policy_service.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>

namespace {
constexpr uint16_t window_size=400;
constexpr uint16_t sample_rate=100;
/* Contact is intentionally latched after acquisition starts. A wearable PPG
   signal can briefly collapse because of motion or pressure; that must pause
   sampling, not throw the UI back to the placement screen. */
constexpr uint32_t finger_attach_threshold=5000;
constexpr uint8_t finger_attach_samples=20;
constexpr uint8_t sample_queue_length=64;
HealthSnapshot snapshot{};
QueueHandle_t sample_queue;
StaticQueue_t sample_queue_control;
uint8_t sample_queue_storage[sample_queue_length*sizeof(Max30102Sample)];
StaticTask_t acquisition_control,processing_control;
StackType_t acquisition_stack[512],processing_stack[768];
uint32_t ring_ir[window_size]{},ring_red[window_size]{};
uint32_t window_ir[window_size]{},window_red[window_size]{};
int32_t filtered_ir[window_size]{},filtered_red[window_size]{};
uint16_t write_index{},sample_count{},samples_since_analysis{};
volatile bool restart_requested{};

template<typename T>T clamp_value(T value,T low,T high){return std::max(low,std::min(high,value));}

void reset_capture(){
  write_index=0;sample_count=0;samples_since_analysis=0;
}

void clear_measurement(){
  reset_capture();
  taskENTER_CRITICAL();
  snapshot.finger_detected=false;snapshot.heart_valid=false;snapshot.spo2_valid=false;
  snapshot.progress_percent=0;snapshot.signal_quality=0;
  std::memset(snapshot.waveform,7,sizeof(snapshot.waveform));
  taskEXIT_CRITICAL();
}

void set_connection(bool connected){
  taskENTER_CRITICAL();
  snapshot.connected=connected;
  snapshot.sensor_status=static_cast<uint8_t>(max30102_status());
  snapshot.sensor_id=max30102_part_id();
  if(!connected)snapshot.finger_detected=false;
  taskEXIT_CRITICAL();
}

bool is_connected(){
  taskENTER_CRITICAL();const bool connected=snapshot.connected;taskEXIT_CRITICAL();return connected;
}

bool take_restart_request(){
  taskENTER_CRITICAL();
  const bool requested=restart_requested;restart_requested=false;
  taskEXIT_CRITICAL();return requested;
}

void publish_capture(bool finger,uint32_t raw_ir){
  taskENTER_CRITICAL();
  snapshot.finger_detected=finger;
  snapshot.raw_ir=raw_ir;
  snapshot.progress_percent=static_cast<uint8_t>(sample_count*100u/window_size);
  taskEXIT_CRITICAL();
}

void copy_window(){
  const uint16_t start=write_index;
  for(uint16_t index=0;index<window_size;++index){
    const uint16_t source=static_cast<uint16_t>((start+index)%window_size);
    window_ir[index]=ring_ir[source];window_red[index]=ring_red[source];
  }
}

void analyze(){
  copy_window();
  uint64_t sum_ir=0,sum_red=0;
  for(uint16_t index=0;index<window_size;++index){sum_ir+=window_ir[index];sum_red+=window_red[index];}
  const int32_t mean_ir=static_cast<int32_t>(sum_ir/window_size);
  const int32_t mean_red=static_cast<int32_t>(sum_red/window_size);
  int32_t minimum=INT32_MAX,maximum=INT32_MIN;
  double square_ir=0.0,square_red=0.0;
  for(uint16_t index=0;index<window_size;++index){
    int64_t smooth_ir=0,smooth_red=0;uint8_t points=0;
    const uint16_t begin=index>2u?index-2u:0u;
    const uint16_t end=std::min<uint16_t>(window_size-1u,index+2u);
    for(uint16_t sample=begin;sample<=end;++sample){smooth_ir+=window_ir[sample];smooth_red+=window_red[sample];++points;}
    filtered_ir[index]=static_cast<int32_t>(smooth_ir/points)-mean_ir;
    filtered_red[index]=static_cast<int32_t>(smooth_red/points)-mean_red;
    minimum=std::min(minimum,filtered_ir[index]);maximum=std::max(maximum,filtered_ir[index]);
    square_ir+=(double)filtered_ir[index]*filtered_ir[index];
    square_red+=(double)filtered_red[index]*filtered_red[index];
  }

  const int32_t amplitude=maximum-minimum;
  const uint32_t perfusion_bp=mean_ir>0
      ?static_cast<uint32_t>((uint64_t)amplitude*10000u/(uint32_t)mean_ir):0u;
  const int32_t threshold=minimum+(amplitude*58)/100;
  uint16_t peaks[12]{};uint8_t peak_count=0;bool above=false;
  int32_t candidate_value=INT32_MIN;uint16_t candidate_index=0;
  for(uint16_t index=2;index<window_size-2;++index){
    if(filtered_ir[index]>threshold){
      if(!above){above=true;candidate_value=filtered_ir[index];candidate_index=index;}
      else if(filtered_ir[index]>candidate_value){candidate_value=filtered_ir[index];candidate_index=index;}
    }else if(above){
      if(peak_count==0u || static_cast<uint16_t>(candidate_index-peaks[peak_count-1u])>=30u){
        if(peak_count<12u)peaks[peak_count++]=candidate_index;
      }else if(candidate_value>filtered_ir[peaks[peak_count-1u]])peaks[peak_count-1u]=candidate_index;
      above=false;
    }
  }

  bool heart_valid=peak_count>=3u;uint8_t heart_rate=0,quality=0;
  if(heart_valid){
    uint32_t interval_sum=0;
    for(uint8_t index=1;index<peak_count;++index)interval_sum+=peaks[index]-peaks[index-1u];
    const uint16_t interval=static_cast<uint16_t>(interval_sum/(peak_count-1u));
    uint32_t deviation=0;
    for(uint8_t index=1;index<peak_count;++index)
      deviation+=static_cast<uint32_t>(std::abs((int)(peaks[index]-peaks[index-1u])-(int)interval));
    deviation/=peak_count-1u;
    const int stability=interval>0?clamp_value(100-(int)(deviation*220u/interval),0,100):0;
    const int perfusion=clamp_value(((int)perfusion_bp-5)*100/145,0,100);
    quality=static_cast<uint8_t>((stability*3+perfusion*2)/5);
    const uint32_t calculated=interval>0?sample_rate*60u/interval:0u;
    heart_valid=calculated>=40u&&calculated<=200u&&quality>=20u;
    heart_rate=static_cast<uint8_t>(clamp_value<uint32_t>(calculated,0u,255u));
  }

  bool spo2_valid=false;uint8_t spo2=0;
  if(heart_valid && mean_ir>0 && mean_red>0 && square_ir>0.0){
    const float ac_ir=std::sqrt(static_cast<float>(square_ir/window_size));
    const float ac_red=std::sqrt(static_cast<float>(square_red/window_size));
    const float ratio=(ac_red/static_cast<float>(mean_red))/(ac_ir/static_cast<float>(mean_ir));
    const float calculated=110.0f-25.0f*ratio;
    spo2_valid=ratio>=0.35f&&ratio<=1.60f&&calculated>=70.0f&&calculated<=100.5f;
    spo2=static_cast<uint8_t>(clamp_value<int>(static_cast<int>(calculated+0.5f),70,100));
  }

  uint8_t waveform[64];
  const uint16_t wave_start=window_size-64u;
  int32_t wave_min=INT32_MAX,wave_max=INT32_MIN;
  for(uint16_t index=wave_start;index<window_size;++index){
    wave_min=std::min(wave_min,filtered_ir[index]);wave_max=std::max(wave_max,filtered_ir[index]);
  }
  const int32_t wave_range=std::max<int32_t>(1,wave_max-wave_min);
  for(uint8_t index=0;index<64u;++index)
    waveform[index]=static_cast<uint8_t>(14-(filtered_ir[wave_start+index]-wave_min)*14/wave_range);

  taskENTER_CRITICAL();
  if(heart_valid){
    if(snapshot.heart_valid && std::abs((int)heart_rate-(int)snapshot.heart_rate)<30)
      heart_rate=static_cast<uint8_t>((snapshot.heart_rate*3u+heart_rate+2u)/4u);
    snapshot.heart_rate=heart_rate;snapshot.heart_valid=true;
  }
  if(spo2_valid){
    if(snapshot.spo2_valid)spo2=static_cast<uint8_t>((snapshot.spo2_percent*3u+spo2+2u)/4u);
    snapshot.spo2_percent=spo2;snapshot.spo2_valid=true;
  }
  snapshot.signal_quality=quality;std::memcpy(snapshot.waveform,waveform,sizeof(waveform));
  taskEXIT_CRITICAL();
}

bool initialize_sensor(){
  if(!max30102_probe()||!max30102_reset())return false;
  if(!max30102_configure())return false;
  set_connection(true);return true;
}

void acquisition_task(void *){
  bool connected=false;
  bool sensor_sleeping=false;
  uint8_t failures=0;
  uint16_t empty_polls=0;
  TickType_t wake=xTaskGetTickCount();
  for(;;){
    if(power_policy_is_screen_sleeping()){
      if(connected&&!sensor_sleeping){
        sensor_sleeping=max30102_set_shutdown(true);
        xQueueReset(sample_queue);
      }
      vTaskDelay(pdMS_TO_TICKS(250));wake=xTaskGetTickCount();continue;
    }
    if(sensor_sleeping){
      const bool resumed=max30102_set_shutdown(false)&&max30102_configure();
      sensor_sleeping=false;xQueueReset(sample_queue);wake=xTaskGetTickCount();
      if(!resumed){connected=false;set_connection(false);continue;}
    }
    if(!connected){
      connected=initialize_sensor();
      if(!connected){set_connection(false);vTaskDelay(pdMS_TO_TICKS(1000));wake=xTaskGetTickCount();continue;}
      failures=0;empty_polls=0;xQueueReset(sample_queue);
    }
    Max30102Sample samples[4];uint8_t count=0;
    if(!max30102_read_samples(samples,4,&count)){
      taskENTER_CRITICAL();if(snapshot.error_count<0xffffu)++snapshot.error_count;taskEXIT_CRITICAL();
      if(failures<100u)++failures;
      if(failures>=100u){connected=false;set_connection(false);}
    }else{
      failures=0;
      if(count==0u){
        if(empty_polls<100u)++empty_polls;
        if(empty_polls>=100u){
          /* Mode 3 at 100 Hz must produce data continuously. Recover a FIFO
             or conversion state that stopped without reporting an I2C error. */
          (void)max30102_reset();
          (void)max30102_configure();
          empty_polls=0;
        }
      }else empty_polls=0;
      for(uint8_t index=0;index<count;++index){
        if(xQueueSend(sample_queue,&samples[index],0)!=pdTRUE){
          Max30102Sample discarded{};
          (void)xQueueReceive(sample_queue,&discarded,0);
          (void)xQueueSend(sample_queue,&samples[index],0);
        }
      }
    }
    xTaskDelayUntil(&wake,pdMS_TO_TICKS(10));
  }
}

void processing_task(void *){
  bool finger_present=false,was_connected=false;
  uint8_t finger_on_count=0;
  uint32_t contact_ir=0;
  for(;;){
    if(power_policy_is_screen_sleeping()){vTaskDelay(pdMS_TO_TICKS(250));continue;}
    const bool connected=is_connected();
    if(!connected){
      if(was_connected){finger_present=false;finger_on_count=0;contact_ir=0;reset_capture();}
      was_connected=false;vTaskDelay(pdMS_TO_TICKS(20));continue;
    }
    if(!was_connected){
      was_connected=true;finger_present=false;finger_on_count=0;contact_ir=0;reset_capture();
    }
    if(take_restart_request()){
      finger_present=false;finger_on_count=0;contact_ir=0;clear_measurement();xQueueReset(sample_queue);
    }

    Max30102Sample sample{};
    if(xQueueReceive(sample_queue,&sample,pdMS_TO_TICKS(20))!=pdTRUE){
      publish_capture(finger_present,contact_ir);continue;
    }
    do{
      taskENTER_CRITICAL();++snapshot.sample_count;taskEXIT_CRITICAL();
      contact_ir=contact_ir==0u?sample.ir:(contact_ir*7u+sample.ir)/8u;
      if(!finger_present){
        if(contact_ir>=finger_attach_threshold){
          if(finger_on_count<finger_attach_samples)++finger_on_count;
        }else finger_on_count=0;
        if(finger_on_count<finger_attach_samples)continue;
        finger_present=true;clear_measurement();
      }
      ring_ir[write_index]=sample.ir;ring_red[write_index]=sample.red;
      write_index=static_cast<uint16_t>((write_index+1u)%window_size);
      if(sample_count<window_size)++sample_count;
      ++samples_since_analysis;
    }while(xQueueReceive(sample_queue,&sample,0)==pdTRUE);

    publish_capture(finger_present,contact_ir);
    if(sample_count==window_size && samples_since_analysis>=100u){samples_since_analysis=0;analyze();}
  }
}
}

void health_service_start(){
  sample_queue=xQueueCreateStatic(sample_queue_length,sizeof(Max30102Sample),sample_queue_storage,&sample_queue_control);
  configASSERT(sample_queue);
  TaskHandle_t acquisition=xTaskCreateStatic(acquisition_task,"ppg_io",512,nullptr,4,acquisition_stack,&acquisition_control);
  TaskHandle_t processing=xTaskCreateStatic(processing_task,"health",768,nullptr,2,processing_stack,&processing_control);
  configASSERT(acquisition);configASSERT(processing);
  system_monitor_register(acquisition);system_monitor_register(processing);
}
void health_service_restart(){taskENTER_CRITICAL();restart_requested=true;taskEXIT_CRITICAL();}
HealthSnapshot health_service_snapshot(){
  taskENTER_CRITICAL();const HealthSnapshot copy=snapshot;taskEXIT_CRITICAL();return copy;
}
