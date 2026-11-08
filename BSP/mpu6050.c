#include "mpu6050.h"
#include "bsp.h"

enum {
    reg_sample_rate=0x19,reg_config=0x1a,reg_gyro_config=0x1b,reg_accel_config=0x1c,
    reg_fifo_enable=0x23,reg_int_enable=0x38,reg_accel_data=0x3b,
    reg_user_control=0x6a,reg_power_1=0x6b,reg_power_2=0x6c,reg_who_am_i=0x75
};
static uint16_t active_address=0x68u<<1;
static uint8_t last_ack_address;

static bool read_register(uint8_t reg,uint8_t *data,uint16_t length){
    return bsp_i2c4_ready()&&HAL_I2C_Mem_Read(&hi2c4,active_address,reg,I2C_MEMADD_SIZE_8BIT,data,length,5)==HAL_OK;
}
static bool write_register(uint8_t reg,uint8_t value){
    return bsp_i2c4_ready()&&HAL_I2C_Mem_Write(&hi2c4,active_address,reg,I2C_MEMADD_SIZE_8BIT,&value,1,5)==HAL_OK;
}
static int16_t signed_word(const uint8_t *data){return (int16_t)(((uint16_t)data[0]<<8)|data[1]);}

bool mpu6050_detect(uint8_t *device_id){
    if(device_id==0||!bsp_i2c4_ready())return false;
    last_ack_address=0;
    const uint8_t addresses[]={0x68u,0x69u};
    for(uint8_t i=0;i<2u;++i){
        active_address=(uint16_t)addresses[i]<<1;uint8_t id=0;
        if(HAL_I2C_IsDeviceReady(&hi2c4,active_address,1,3)!=HAL_OK)continue;
        if(last_ack_address==0)last_ack_address=addresses[i];
        if(read_register(reg_who_am_i,&id,1)){*device_id=id;return true;}
    }
    *device_id=0;return false;
}
uint8_t mpu6050_last_ack_address(void){return last_ack_address;}
bool mpu6050_device_supported(uint8_t id){return id==0x68u||id==0x70u||id==0x71u||id==0x73u;}
bool mpu6050_configure(void){
    uint8_t verify=0;
    if(!write_register(reg_power_1,0x80u))return false;
    HAL_Delay(100);
    if(!write_register(reg_power_1,0x01u)||!write_register(reg_power_2,0x00u))return false;
    if(!write_register(reg_config,0x03u)||!write_register(reg_sample_rate,0x09u))return false;
    if(!write_register(reg_gyro_config,0x08u)||!write_register(reg_accel_config,0x08u))return false;
    if(!write_register(reg_fifo_enable,0x00u)||!write_register(reg_int_enable,0x00u)||
       !write_register(reg_user_control,0x00u))return false;
    return read_register(reg_power_1,&verify,1)&&(verify&0x40u)==0u;
}
bool mpu6050_read(Mpu6050Sample *sample){
    if(sample==0)return false;
    uint8_t data[14];
    if(!read_register(reg_accel_data,data,sizeof(data)))return false;
    sample->ax=signed_word(data);sample->ay=signed_word(data+2);sample->az=signed_word(data+4);
    sample->temperature=signed_word(data+6);
    sample->gx=signed_word(data+8);sample->gy=signed_word(data+10);sample->gz=signed_word(data+12);
    return true;
}
