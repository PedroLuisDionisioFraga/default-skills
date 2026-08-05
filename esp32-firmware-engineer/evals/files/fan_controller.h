#pragma once

#include <stdbool.h>

#include "esp_err.h"
#include "relay_device.h"
#include "temp_sensor.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
  float on_celsius;
  float off_celsius;
  uint32_t min_runtime_s;
} fan_profile_t;

esp_err_t fan_controller_init(relay_t *fan, temp_sensor_t *sensor);
esp_err_t fan_controller_set_profile(const fan_profile_t *profile);
esp_err_t fan_controller_toggle(void);
esp_err_t fan_controller_force(bool on);

#ifdef __cplusplus
}
#endif
