#include "{{COMPONENT_NAME}}.h"

#include "esp_log.h"

static const char *TAG = "{{COMPONENT_NAME}}";

esp_err_t {{COMPONENT_NAME_SNAKE}}_init(void)
{
  ESP_LOGI(TAG, "initialized");
  return ESP_OK;
}
