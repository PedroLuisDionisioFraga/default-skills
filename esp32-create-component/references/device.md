# Device Component Pattern

A **device** is a high-level abstraction for a physical peripheral.
It owns a `gpio_t`, delegates all pin I/O to the GPIO driver, and hides
FreeRTOS internals (timers, tasks, queues) behind a clean `<name>_*` API.

---

## Sub-classify: Actuator vs. Sensor

| Sub-type           | Examples                  | Characteristics                                          |
|--------------------|---------------------------|----------------------------------------------------------|
| **Actuator**       | LED, Relay, Buzzer, Servo | Output-only; may support cyclic/timed driving            |
| **Sensor / Input** | Button, encoder, PIR      | Input; needs ISR + task + debounce; fires user callbacks |

Follow the **Actuator** template below for output devices.
For sensors, use the same struct/private-context skeleton but add ISR, task,
and queue — read `components/devices/button/` as the canonical reference.

---

## File layout

```text
components/devices/<name>/
├── include/
│   └── <name>_device.h
├── <name>_device.c
└── CMakeLists.txt
```

---

## Header template

```c
/**
 * @file <name>_device.h
 * @brief <One-line description>. Built on top of the GPIO driver.
 */

#ifndef <NAME>_DEVICE_H
#define <NAME>_DEVICE_H

#include <esp_err.h>
#include "gpio_drivers.h"

typedef enum
{
  <NAME>_STATE_OFF     = 0,  /* zero-init is always a safe default */
  <NAME>_STATE_ON      = 1,
  <NAME>_STATE_CYCLING = 2,  /* only if cyclic/timed operation is supported */
} <name>_state_t;

typedef struct <name>
{
  gpio_t         _gpio;   /* callers set only ._gpio.pin before calling init */
  <name>_state_t _state;  /* read via getter; never written directly by callers */
} <name>_t;

esp_err_t <name>_init(<name>_t *self);
esp_err_t <name>_on(<name>_t *self);
esp_err_t <name>_off(<name>_t *self);
esp_err_t <name>_stop(<name>_t *self);

/* Include only if the device supports timer-driven cyclic output. */
esp_err_t <name>_cycle(<name>_t *self, uint32_t frequency_hz, uint32_t times);

/* Include only if callers need to read the current state. */
<name>_state_t <name>_get_state(<name>_t *self);

#endif  // <NAME>_DEVICE_H
```

---

## Source template (Actuator)

```c
#include "<name>_device.h"

#include <esp_check.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/timers.h>
#include <stdlib.h>

static const char *TAG = "<NAME>";

/* Private context — heap-allocated once in init(), never exposed to callers.
   Stored in _isr_handler_arg because output pins have no ISR (field is free). */
typedef struct
{
  <name>_t     *<name>;
  TimerHandle_t cycle_timer;   /* NULL until first cycle() call; reused after */
  uint32_t      cycle_count;   /* remaining half-cycles; 0 = infinite */
} <name>_priv_t;

static <name>_priv_t *_priv(<name>_t *self)
{
  return (<name>_priv_t *)self->_gpio._isr_handler_arg;
}

static void _<name>_cycle_cb(TimerHandle_t xTimer)
{
  <name>_priv_t *priv = (<name>_priv_t *)pvTimerGetTimerID(xTimer);
  gpio_toggle(&priv-><name>->_gpio);
  if (priv->cycle_count > 0)
  {
    priv->cycle_count--;
    if (priv->cycle_count == 0)
      <name>_stop(priv-><name>);
  }
}

esp_err_t <name>_init(<name>_t *self)
{
  ESP_RETURN_ON_FALSE(self != NULL, ESP_ERR_INVALID_ARG, TAG, "self is NULL");

  self->_gpio._mode      = GPIO_MODE_OUTPUT;
  self->_gpio._act_state = GPIO_STATE_LOW;
  ESP_RETURN_ON_ERROR(gpio_init(&self->_gpio), TAG, "gpio_init failed");

  <name>_priv_t *priv = malloc(sizeof(<name>_priv_t));
  ESP_RETURN_ON_FALSE(priv != NULL, ESP_ERR_NO_MEM, TAG, "priv alloc failed");
  priv-><name>       = self;
  priv->cycle_count  = 0;
  priv->cycle_timer  = NULL;

  self->_gpio._isr_handler_arg = priv;
  self->_state = <NAME>_STATE_OFF;

  ESP_LOGI(TAG, "<Name> on GPIO%d initialized", self->_gpio.pin);
  return ESP_OK;
}

esp_err_t <name>_stop(<name>_t *self)
{
  <name>_priv_t *priv = _priv(self);
  if (priv->cycle_timer)
    xTimerStop(priv->cycle_timer, 0);
  priv->cycle_count = 0;

  gpio_write(&self->_gpio, GPIO_STATE_LOW);
  self->_state = <NAME>_STATE_OFF;
  return ESP_OK;
}

esp_err_t <name>_on(<name>_t *self)
{
  <name>_stop(self);   /* stop any running timer before changing output */
  gpio_write(&self->_gpio, GPIO_STATE_HIGH);
  self->_state = <NAME>_STATE_ON;
  return ESP_OK;
}

esp_err_t <name>_off(<name>_t *self)
{
  return <name>_stop(self);
}

esp_err_t <name>_cycle(<name>_t *self, uint32_t frequency_hz, uint32_t times)
{
  if (frequency_hz == 0)
    return ESP_ERR_INVALID_ARG;

  <name>_stop(self);

  <name>_priv_t *priv = _priv(self);
  uint32_t half_ms   = 500 / frequency_hz;
  priv->cycle_count  = (times == 0) ? 0 : times * 2;

  if (!priv->cycle_timer)
    priv->cycle_timer = xTimerCreate("<name>_cycle", pdMS_TO_TICKS(half_ms),
                                     pdTRUE, priv, _<name>_cycle_cb);

  gpio_write(&self->_gpio, GPIO_STATE_HIGH);
  self->_state = <NAME>_STATE_CYCLING;

  xTimerChangePeriod(priv->cycle_timer, pdMS_TO_TICKS(half_ms), 0);

  ESP_LOGI(TAG, "<Name> GPIO%d cycling at %luHz, %lu times",
           self->_gpio.pin, frequency_hz, times);
  return ESP_OK;
}

<name>_state_t <name>_get_state(<name>_t *self)
{
  return self->_state;
}
```

---

## CMakeLists.txt

```cmake
idf_component_register(SRCS "<name>_device.c"
                    INCLUDE_DIRS "include"
                    REQUIRES esp_driver_gpio gpio freertos)
```

Drop `freertos` only if the device uses no timers or tasks (rare for actuators).

---

## Key invariants

1. **`stop()` before `on()`** — prevents ghost timer toggles after state changes.
2. **Private context in `_gpio._isr_handler_arg`** — output pins never use ISR, so the field is free; keeps the public struct FreeRTOS-free. This is the per-instance form of the `<name>_priv_t` rule in `SKILL.md`.
3. **Timer created lazily, reused via `xTimerChangePeriod`** — no allocation cost on every `cycle()` call.
4. **`init()` returns `esp_err_t`, like every other function** — it propagates; the caller decides fatality (`ESP_ERROR_CHECK(led_init(&led));` in `app_main`). A device must not abort the boot on its own.
5. **`_OFF = 0`** — zero-initialized structs are in a safe state before `init()` runs.

---

## Checklist

- [ ] Include guard: `<NAME>_DEVICE_H`
- [ ] Struct fields with `_` prefix for all implementation details
- [ ] State enum: `_OFF = 0` first
- [ ] `init()` returns `esp_err_t`: validates `self`, sets mode/act_state, calls `gpio_init()`, mallocs priv (NULL-checked), stores in `_gpio._isr_handler_arg`
- [ ] `stop()`: stops timer, resets count, writes LOW, state = OFF
- [ ] `on()`: calls `stop()` first, writes HIGH
- [ ] `off()`: delegates to `stop()`
- [ ] `cycle()`: validates freq > 0, calls `stop()`, lazy timer create, starts HIGH, changes period
- [ ] `freertos` in CMakeLists `REQUIRES` if timers/tasks used
- [ ] TAG = device name uppercase, `init()` logs GPIO pin
