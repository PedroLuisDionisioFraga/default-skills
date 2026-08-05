#include "fan_controller.h"

#include <esp_check.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>
#include <freertos/timers.h>

#include "relay_device.h"
#include "temp_sensor.h"

static const char *TAG = "FAN_CTRL";

#define POLL_PERIOD_MS 30000

typedef struct
{
  relay_t *fan;
  temp_sensor_t *sensor;
  TimerHandle_t poll_timer;

  SemaphoreHandle_t cfg_mutex;
  fan_profile_t profile; /* on_celsius, off_celsius, min_runtime_s */

  bool last_reported_on;
} fan_priv_t;

static fan_priv_t s_priv;

static void _drive_fan(bool on)
{
  if (on)
    relay_on(s_priv.fan);
  else
    relay_off(s_priv.fan);
}

static void _update(void)
{
  xSemaphoreTake(s_priv.cfg_mutex, portMAX_DELAY);
  fan_profile_t profile = s_priv.profile;
  xSemaphoreGive(s_priv.cfg_mutex);

  float temp_c = 0.0f;
  if (temp_sensor_read(s_priv.sensor, &temp_c) != ESP_OK)
  {
    ESP_LOGW(TAG, "sensor read failed, leaving fan as-is");
    return;
  }

  bool is_on = (relay_get_state(s_priv.fan) == RELAY_STATE_ON);

  bool want;
  if (temp_c >= profile.on_celsius)
    want = true;
  else if (temp_c <= profile.off_celsius)
    want = false;
  else
    want = is_on; /* hysteresis band: hold current state */

  if (want != s_priv.last_reported_on)
  {
    ESP_LOGI(TAG,
             "fan eval: temp=%.2fC on_at=%.2f off_at=%.2f -> want=%s (fan=%s)",
             temp_c,
             profile.on_celsius,
             profile.off_celsius,
             want ? "ON" : "OFF",
             is_on ? "ON" : "OFF");
    s_priv.last_reported_on = want;
  }

  if (want != is_on)
  {
    _drive_fan(want);
    ESP_LOGI(TAG, "fan -> %s", want ? "ON" : "OFF");
  }
}

static void _poll_timer_cb(TimerHandle_t t)
{
  (void)t;
  _update();
}

esp_err_t fan_controller_init(relay_t *fan, temp_sensor_t *sensor)
{
  ESP_RETURN_ON_FALSE(fan != NULL && sensor != NULL, ESP_ERR_INVALID_ARG, TAG, "NULL arg");

  s_priv.fan = fan;
  s_priv.sensor = sensor;
  s_priv.cfg_mutex = xSemaphoreCreateMutex();
  ESP_RETURN_ON_FALSE(s_priv.cfg_mutex != NULL, ESP_ERR_NO_MEM, TAG, "mutex failed");

  ESP_RETURN_ON_ERROR(relay_init(fan), TAG, "relay init failed");
  relay_off(fan);

  s_priv.poll_timer = xTimerCreate("fan_poll", pdMS_TO_TICKS(POLL_PERIOD_MS), pdTRUE, NULL, _poll_timer_cb);
  ESP_RETURN_ON_FALSE(s_priv.poll_timer != NULL, ESP_ERR_NO_MEM, TAG, "timer failed");
  xTimerStart(s_priv.poll_timer, 0);

  _update();
  return ESP_OK;
}

/* Called from the MQTT RX task when a new profile arrives from the server. */
esp_err_t fan_controller_set_profile(const fan_profile_t *profile)
{
  ESP_RETURN_ON_FALSE(profile != NULL, ESP_ERR_INVALID_ARG, TAG, "NULL profile");
  ESP_RETURN_ON_FALSE(profile->on_celsius > profile->off_celsius, ESP_ERR_INVALID_ARG, TAG, "bad hysteresis");

  xSemaphoreTake(s_priv.cfg_mutex, portMAX_DELAY);
  s_priv.profile = *profile;
  xSemaphoreGive(s_priv.cfg_mutex);

  ESP_LOGI(TAG, "profile set: on=%.2fC off=%.2fC", profile->on_celsius, profile->off_celsius);
  _update();
  return ESP_OK;
}

/* Called from the button task on a maintenance gesture. */
esp_err_t fan_controller_toggle(void)
{
  bool turn_on = (relay_get_state(s_priv.fan) != RELAY_STATE_ON);
  _drive_fan(turn_on);
  ESP_LOGI(TAG, "manual toggle -> %s", turn_on ? "ON" : "OFF");
  return ESP_OK;
}

/* Called from the MQTT RX task for a forced override. */
esp_err_t fan_controller_force(bool on)
{
  _drive_fan(on);
  ESP_LOGI(TAG, "forced -> %s", on ? "ON" : "OFF");
  return ESP_OK;
}
