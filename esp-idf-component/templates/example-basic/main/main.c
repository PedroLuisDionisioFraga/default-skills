#include <stdio.h>

#include "{{COMPONENT_NAME}}.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

void app_main(void)
{
  printf("=== {{COMPONENT_TITLE}} Basic Example ===\n");

  // TODO: call the component's API here.

  // Keep app_main alive to avoid the "Returned from app_main()" log.
  while (true)
    vTaskDelay(pdMS_TO_TICKS(1000));
}
