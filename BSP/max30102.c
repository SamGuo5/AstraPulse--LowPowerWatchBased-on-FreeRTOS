#include "max30102.h"
#include "bsp.h"

#define MAX30102_ADDRESS (0x57u<<1)
#define REG_INTR_STATUS_1 0x00u
#define REG_INTR_ENABLE_1 0x02u
#define REG_FIFO_WRITE 0x04u
#define REG_FIFO_OVERFLOW 0x05u
#define REG_FIFO_READ 0x06u
#define REG_FIFO_DATA 0x07u
#define REG_FIFO_CONFIG 0x08u
#define REG_MODE_CONFIG 0x09u
#define REG_SPO2_CONFIG 0x0au
#define REG_LED_RED 0x0cu
#define REG_LED_IR 0x0du
#define REG_PART_ID 0xffu

static Max30102Status last_status=MAX30102_STATUS_NO_ACK;
static uint8_t detected_part_id=0xffu;

static bool write_reg(uint8_t reg,uint8_t value){
  const uint8_t data[2]={reg,value};
  for(uint8_t attempt=0;attempt<2u;++attempt){
    if(HAL_I2C_Master_Transmit(&hi2c3,MAX30102_ADDRESS,(uint8_t *)data,2,5)==HAL_OK)return true;
  }
  return false;
}
static bool read_bytes(uint8_t reg,uint8_t *data,uint16_t size){
  for(uint8_t attempt=0;attempt<2u;++attempt){
    uint8_t address=reg;
    if(HAL_I2C_Master_Transmit(&hi2c3,MAX30102_ADDRESS,&address,1,5)==HAL_OK &&
       HAL_I2C_Master_Receive(&hi2c3,MAX30102_ADDRESS,data,size,5)==HAL_OK)return true;
  }
  return false;
}
static bool read_reg(uint8_t reg,uint8_t *value){return read_bytes(reg,value,1);}

static void recover_bus(void){
  (void)HAL_I2C_DeInit(&hi2c3);
  GPIO_InitTypeDef gpio={0};
  gpio.Pin=GPIO_PIN_0|GPIO_PIN_1;gpio.Mode=GPIO_MODE_OUTPUT_OD;
  gpio.Pull=GPIO_PULLUP;gpio.Speed=GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC,&gpio);
  HAL_GPIO_WritePin(GPIOC,GPIO_PIN_0|GPIO_PIN_1,GPIO_PIN_SET);
  HAL_Delay(1);
  for(uint8_t pulse=0;pulse<9u && HAL_GPIO_ReadPin(GPIOC,GPIO_PIN_1)==GPIO_PIN_RESET;++pulse){
    HAL_GPIO_WritePin(GPIOC,GPIO_PIN_0,GPIO_PIN_RESET);HAL_Delay(1);
    HAL_GPIO_WritePin(GPIOC,GPIO_PIN_0,GPIO_PIN_SET);HAL_Delay(1);
  }
  HAL_GPIO_WritePin(GPIOC,GPIO_PIN_1,GPIO_PIN_RESET);HAL_Delay(1);
  HAL_GPIO_WritePin(GPIOC,GPIO_PIN_0,GPIO_PIN_SET);HAL_Delay(1);
  HAL_GPIO_WritePin(GPIOC,GPIO_PIN_1,GPIO_PIN_SET);HAL_Delay(1);
  (void)HAL_I2C_Init(&hi2c3);
  (void)HAL_I2CEx_ConfigAnalogFilter(&hi2c3,I2C_ANALOGFILTER_ENABLE);
  (void)HAL_I2CEx_ConfigDigitalFilter(&hi2c3,0);
}

bool max30102_probe(void){
  if(HAL_I2C_IsDeviceReady(&hi2c3,MAX30102_ADDRESS,2,5)!=HAL_OK){
    recover_bus();
    if(HAL_I2C_IsDeviceReady(&hi2c3,MAX30102_ADDRESS,2,5)!=HAL_OK){
      last_status=MAX30102_STATUS_NO_ACK;return false;
    }
  }
  uint8_t part_id=0xffu;
  if(!read_reg(REG_PART_ID,&part_id)){
    last_status=MAX30102_STATUS_ID_READ_FAILED;return false;
  }
  if(part_id==0x15u || detected_part_id==0xffu)detected_part_id=part_id;
  /* MAX30102 reports 0x15. Some pin-compatible parts report another ID but
     implement the same register map; configuration read-back validates them. */
  last_status=MAX30102_STATUS_OK;return true;
}

bool max30102_reset(void){
  if(!write_reg(REG_MODE_CONFIG,0x40u)){
    last_status=MAX30102_STATUS_RESET_FAILED;return false;
  }
  for(uint8_t attempt=0;attempt<20u;++attempt){
    HAL_Delay(5);
    uint8_t mode=0xffu;
    if(read_reg(REG_MODE_CONFIG,&mode) && (mode&0x40u)==0u){last_status=MAX30102_STATUS_OK;return true;}
  }
  last_status=MAX30102_STATUS_RESET_FAILED;return false;
}

bool max30102_configure(void){
  uint8_t status[2];
  uint8_t mode=0,spo2=0;
  const bool configured=
      read_bytes(REG_INTR_STATUS_1,status,2) &&
      write_reg(REG_INTR_ENABLE_1,0x00u) &&
      write_reg(REG_FIFO_WRITE,0x00u) && write_reg(REG_FIFO_OVERFLOW,0x00u) &&
      /* Keep rollover disabled. Otherwise a full FIFO has equal read/write
         pointers and is indistinguishable from an empty FIFO without also
         consulting the overflow counter. */
      write_reg(REG_FIFO_READ,0x00u) && write_reg(REG_FIFO_CONFIG,0x00u) &&
      write_reg(REG_SPO2_CONFIG,0x27u) && write_reg(REG_LED_RED,0x24u) &&
      write_reg(REG_LED_IR,0x24u) && write_reg(REG_MODE_CONFIG,0x03u) &&
      read_reg(REG_MODE_CONFIG,&mode) && read_reg(REG_SPO2_CONFIG,&spo2) &&
      (mode&0x07u)==0x03u && spo2==0x27u;
  last_status=configured?MAX30102_STATUS_OK:MAX30102_STATUS_CONFIG_FAILED;
  return configured;
}

bool max30102_set_shutdown(bool shutdown){
  uint8_t mode=0;
  if(!read_reg(REG_MODE_CONFIG,&mode))return false;
  const uint8_t requested=shutdown?(uint8_t)(mode|0x80u):(uint8_t)(mode&~0x80u);
  if(!write_reg(REG_MODE_CONFIG,requested))return false;
  uint8_t verify=0;
  if(!read_reg(REG_MODE_CONFIG,&verify))return false;
  const bool stopped=(verify&0x80u)!=0u;
  if(stopped!=shutdown)return false;
  last_status=MAX30102_STATUS_OK;return true;
}

bool max30102_read_samples(Max30102Sample *samples,uint8_t capacity,uint8_t *count){
  uint8_t write_pointer=0,read_pointer=0,overflow=0;
  if(samples==0 || count==0 || capacity==0)return false;
  if(!read_reg(REG_FIFO_WRITE,&write_pointer)||!read_reg(REG_FIFO_OVERFLOW,&overflow)||
     !read_reg(REG_FIFO_READ,&read_pointer)){
    last_status=MAX30102_STATUS_DATA_FAILED;return false;
  }
  uint8_t available=(uint8_t)((write_pointer-read_pointer)&0x1fu);
  if(available==0u && overflow!=0u)available=32u;
  if(overflow!=0u)(void)write_reg(REG_FIFO_OVERFLOW,0x00u);
  if(available>capacity)available=capacity;
  /* Short transactions proved more reliable on this board than draining the
     complete FIFO in one transfer. At 100 Hz, four samples still give 40 ms
     of scheduling headroom. */
  if(available>4u)available=4u;
  *count=available;
  if(available==0)return true;
  uint8_t bytes[4u*6u];
  if(!read_bytes(REG_FIFO_DATA,bytes,(uint16_t)available*6u)){
    last_status=MAX30102_STATUS_DATA_FAILED;return false;
  }
  for(uint8_t index=0;index<available;++index){
    const uint8_t *data=&bytes[index*6u];
    samples[index].red=(((uint32_t)data[0]&0x03u)<<16)|((uint32_t)data[1]<<8)|data[2];
    samples[index].ir=(((uint32_t)data[3]&0x03u)<<16)|((uint32_t)data[4]<<8)|data[5];
  }
  last_status=MAX30102_STATUS_OK;return true;
}

Max30102Status max30102_status(void){return last_status;}
uint8_t max30102_part_id(void){return detected_part_id;}
