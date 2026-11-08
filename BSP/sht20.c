#include "sht20.h"
#include "bsp.h"

#define SHT20_ADDRESS (0x40u<<1)
#define SHT20_RESET 0xfeu
#define SHT20_TEMPERATURE_NO_HOLD 0xf3u
#define SHT20_HUMIDITY_NO_HOLD 0xf5u

static bool write_command(uint8_t command){
  return HAL_I2C_Master_Transmit(&hi2c1,SHT20_ADDRESS,&command,1,20)==HAL_OK;
}
static uint8_t crc8(const uint8_t *data,uint8_t length){
  uint8_t crc=0;
  for(uint8_t index=0;index<length;++index){
    crc^=data[index];
    for(uint8_t bit=0;bit<8;++bit)
      crc=(crc&0x80u)!=0u?(uint8_t)((crc<<1)^0x31u):(uint8_t)(crc<<1);
  }
  return crc;
}
bool sht20_soft_reset(void){return write_command(SHT20_RESET);}
bool sht20_start_temperature(void){return write_command(SHT20_TEMPERATURE_NO_HOLD);}
bool sht20_start_humidity(void){return write_command(SHT20_HUMIDITY_NO_HOLD);}
bool sht20_read_raw(uint16_t *raw){
  uint8_t data[3]={0};
  if(raw==0 || HAL_I2C_Master_Receive(&hi2c1,SHT20_ADDRESS,data,sizeof(data),20)!=HAL_OK)return false;
  if(crc8(data,2)!=data[2])return false;
  *raw=(uint16_t)(((uint16_t)data[0]<<8)|data[1]);
  *raw&=0xfffcu;
  return true;
}
