#ifndef WATCH_ASTRA_MAX30102_H
#define WATCH_ASTRA_MAX30102_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {uint32_t red;uint32_t ir;} Max30102Sample;
typedef enum {
  MAX30102_STATUS_OK=0,
  MAX30102_STATUS_NO_ACK=1,
  MAX30102_STATUS_ID_READ_FAILED=2,
  MAX30102_STATUS_RESET_FAILED=3,
  MAX30102_STATUS_CONFIG_FAILED=4,
  MAX30102_STATUS_DATA_FAILED=5
} Max30102Status;
bool max30102_probe(void);
bool max30102_reset(void);
bool max30102_configure(void);
bool max30102_set_shutdown(bool shutdown);
bool max30102_read_samples(Max30102Sample *samples,uint8_t capacity,uint8_t *count);
Max30102Status max30102_status(void);
uint8_t max30102_part_id(void);

#ifdef __cplusplus
}
#endif
#endif
