#include "time_service.hpp"

#include "FreeRTOS.h"
#include "bsp.h"
#include "task.h"
#include "system_monitor_service.hpp"
#include "power_policy_service.hpp"

namespace {
constexpr uint32_t backup_magic=0x57544332u;  // "WTC2"
TimeSnapshot snapshot{},pending{};
bool pending_write{};
StaticTask_t task_control;
StackType_t task_stack[384];

constexpr uint8_t digit(char value){return static_cast<uint8_t>(value-'0');}
constexpr uint8_t build_hour(){return digit(__TIME__[0])*10u+digit(__TIME__[1]);}
constexpr uint8_t build_minute(){return digit(__TIME__[3])*10u+digit(__TIME__[4]);}
constexpr uint8_t build_second(){return digit(__TIME__[6])*10u+digit(__TIME__[7]);}
constexpr uint16_t build_year(){return digit(__DATE__[7])*1000u+digit(__DATE__[8])*100u+digit(__DATE__[9])*10u+digit(__DATE__[10]);}
constexpr uint8_t build_day(){return (__DATE__[4]==' '?0u:digit(__DATE__[4])*10u)+digit(__DATE__[5]);}
constexpr uint8_t build_month(){
  return __DATE__[0]=='J'&&__DATE__[1]=='a'?1u:
         __DATE__[0]=='F'?2u:__DATE__[0]=='M'&&__DATE__[2]=='r'?3u:
         __DATE__[0]=='A'&&__DATE__[1]=='p'?4u:__DATE__[0]=='M'?5u:
         __DATE__[0]=='J'&&__DATE__[2]=='n'?6u:__DATE__[0]=='J'?7u:
         __DATE__[0]=='A'?8u:__DATE__[0]=='S'?9u:__DATE__[0]=='O'?10u:
         __DATE__[0]=='N'?11u:12u;
}
bool leap(uint16_t year){return year%4u==0u&&(year%100u!=0u||year%400u==0u);}
uint8_t days_in_month(uint16_t year,uint8_t month){
  static constexpr uint8_t days[]={31,28,31,30,31,30,31,31,30,31,30,31};
  return month==2u&&leap(year)?29u:days[month-1u];
}
uint8_t weekday(uint16_t year,uint8_t month,uint8_t day){
  static constexpr uint8_t offsets[]={0,3,2,5,0,3,5,1,4,6,2,4};
  if(month<3u)--year;
  return static_cast<uint8_t>((year+year/4u-year/100u+year/400u+offsets[month-1u]+day)%7u);
}
bool valid(const TimeSnapshot &value){
  return value.year>=2000u&&value.year<=2099u&&value.month>=1u&&value.month<=12u&&
         value.day>=1u&&value.day<=days_in_month(value.year,value.month)&&
         value.hour<24u&&value.minute<60u&&value.second<60u;
}
uint8_t rtc_weekday(uint8_t value){return value==0u?RTC_WEEKDAY_SUNDAY:value;}
uint8_t app_weekday(uint8_t value){return value==RTC_WEEKDAY_SUNDAY?0u:value;}

bool write_rtc(TimeSnapshot value){
  if(!valid(value))return false;
  value.weekday=weekday(value.year,value.month,value.day);
  RTC_TimeTypeDef time{};RTC_DateTypeDef date{};
  time.Hours=value.hour;time.Minutes=value.minute;time.Seconds=value.second;
  time.DayLightSaving=RTC_DAYLIGHTSAVING_NONE;time.StoreOperation=RTC_STOREOPERATION_RESET;
  date.Year=static_cast<uint8_t>(value.year-2000u);date.Month=value.month;date.Date=value.day;
  date.WeekDay=rtc_weekday(value.weekday);
  if(HAL_RTC_SetTime(&hrtc,&time,RTC_FORMAT_BIN)!=HAL_OK ||
     HAL_RTC_SetDate(&hrtc,&date,RTC_FORMAT_BIN)!=HAL_OK)return false;
  HAL_RTCEx_BKUPWrite(&hrtc,RTC_BKP_DR0,backup_magic);return true;
}

bool read_rtc(TimeSnapshot &value){
  RTC_TimeTypeDef time{};RTC_DateTypeDef date{};
  if(HAL_RTC_GetTime(&hrtc,&time,RTC_FORMAT_BIN)!=HAL_OK ||
     HAL_RTC_GetDate(&hrtc,&date,RTC_FORMAT_BIN)!=HAL_OK)return false;
  value={static_cast<uint16_t>(2000u+date.Year),date.Month,date.Date,app_weekday(date.WeekDay),
         time.Hours,time.Minutes,time.Seconds,true};
  value.valid=valid(value);return value.valid;
}

void publish(const TimeSnapshot &value){taskENTER_CRITICAL();snapshot=value;taskEXIT_CRITICAL();}

void initialize_clock(){
  TimeSnapshot value{};
  const bool initialized=HAL_RTCEx_BKUPRead(&hrtc,RTC_BKP_DR0)==backup_magic;
  if(!initialized||!read_rtc(value)){
    value={build_year(),build_month(),build_day(),0,build_hour(),build_minute(),build_second(),true};
    value.weekday=weekday(value.year,value.month,value.day);
    (void)write_rtc(value);
  }
  if(read_rtc(value))publish(value);
}

void time_task(void *){
  TickType_t wake=xTaskGetTickCount();
  for(;;){
    taskENTER_CRITICAL();const bool write=pending_write;const TimeSnapshot requested=pending;pending_write=false;taskEXIT_CRITICAL();
    if(write)(void)write_rtc(requested);
    TimeSnapshot current{};if(read_rtc(current))publish(current);
    xTaskDelayUntil(&wake,pdMS_TO_TICKS(power_policy_is_screen_sleeping()?1000u:200u));
  }
}
}

void time_service_start(){
  initialize_clock();
  TaskHandle_t handle=xTaskCreateStatic(time_task,"time",384,nullptr,2,task_stack,&task_control);
  configASSERT(handle);system_monitor_register(handle);
}
TimeSnapshot time_service_snapshot(){taskENTER_CRITICAL();const TimeSnapshot copy=snapshot;taskEXIT_CRITICAL();return copy;}
bool time_service_set(TimeSnapshot value){
  if(!valid(value))return false;
  value.weekday=weekday(value.year,value.month,value.day);
  taskENTER_CRITICAL();pending=value;pending_write=true;taskEXIT_CRITICAL();return true;
}
