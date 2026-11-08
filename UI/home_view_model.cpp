#include "home_view_model.hpp"
#include "climate_service.hpp"
#include "health_service.hpp"
#include "time_service.hpp"
#include "power_service.hpp"
#include "alarm_service.hpp"
#include "motion_service.hpp"

namespace {
void update_time(HomeSnapshot &target,const TimeSnapshot &source){
  if(!source.valid)return;
  target.year=source.year;target.month=source.month;target.day=source.day;target.weekday=source.weekday;
  target.hour=source.hour;target.minute=source.minute;target.second=source.second;
}
}

void HomeViewModel::begin(uint32_t now_ms) {
  (void)now_ms;
  snapshot_={};
  snapshot_.year=2026;snapshot_.month=1;snapshot_.day=1;snapshot_.weekday=4;
  update_time(snapshot_,time_service_snapshot());
}

void HomeViewModel::update(uint32_t now_ms) {
  (void)now_ms;
  update_time(snapshot_,time_service_snapshot());
  const PowerSnapshot power=power_service_snapshot();
  snapshot_.battery_valid=power.valid;snapshot_.battery_low=power.low;
  if(power.valid)snapshot_.battery_percent=power.battery_percent;
  const AlarmSnapshot alarm=alarm_service_snapshot();
  snapshot_.alarm_enabled=alarm.alarms[0].enabled||alarm.alarms[1].enabled;
  const MotionSnapshot motion=motion_service_snapshot();
  if(motion.connected){snapshot_.steps=motion.steps;snapshot_.activity_percent=motion.activity_percent;}
  const ClimateSnapshot climate=climate_service_snapshot();
  snapshot_.climate_valid=climate.valid;
  if(climate.valid){
    snapshot_.temperature_c=static_cast<int8_t>((climate.temperature_tenths+(climate.temperature_tenths>=0?5:-5))/10);
    snapshot_.humidity_percent=static_cast<uint8_t>((climate.humidity_tenths+5u)/10u);
  }
  const HealthSnapshot health=health_service_snapshot();
  snapshot_.heart_valid=health.heart_valid;snapshot_.spo2_valid=health.spo2_valid;
  if(health.heart_valid)snapshot_.heart_rate=health.heart_rate;
  if(health.spo2_valid)snapshot_.spo2_percent=health.spo2_percent;
}
