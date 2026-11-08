#ifndef WATCH_ASTRA_MPU6050_H
#define WATCH_ASTRA_MPU6050_H
#include <stdbool.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct {
    int16_t ax,ay,az;
    int16_t temperature;
    int16_t gx,gy,gz;
} Mpu6050Sample;
bool mpu6050_detect(uint8_t *device_id);
uint8_t mpu6050_last_ack_address(void);
bool mpu6050_device_supported(uint8_t device_id);
bool mpu6050_configure(void);
bool mpu6050_read(Mpu6050Sample *sample);
#ifdef __cplusplus
}
#endif
#endif
