#ifndef WATCH_ASTRA_SHT20_H
#define WATCH_ASTRA_SHT20_H

#include <stdbool.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
bool sht20_soft_reset(void);
bool sht20_start_temperature(void);
bool sht20_start_humidity(void);
bool sht20_read_raw(uint16_t *raw);
#ifdef __cplusplus
}
#endif
#endif
