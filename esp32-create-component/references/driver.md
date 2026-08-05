# Driver Component Pattern

A **driver** is a thin HAL wrapper around one ESP32 peripheral (GPIO, UART,
SPI, I2C, ADC …). It exposes a typed struct with embedded function pointers so
callers can operate the peripheral through a stable interface without depending
on ESP-IDF internals.

Devices depend on drivers; drivers depend only on ESP-IDF hardware components.

---

## File layout

```text
components/drivers/<name>/
├── include/
│   └── <name>_drivers.h
├── <name>_drivers.c
└── CMakeLists.txt
```

Single-source drivers keep the `.c` at the root (`drivers/gpio/gpio_drivers.c`).
Drivers with several sources use `src/` (`drivers/nvs/src/nvs_driver.c`). Both
suffixes exist in this repo — `_drivers` for peripheral HALs, `_driver` for
subsystem wrappers like NVS. Match the neighbour you are closest to rather than
introducing a third spelling.

---

## Header template

```c
/**
 * @file <name>_drivers.h
 * @brief <Peripheral> driver for the ESP32-C6-DevKitC-1 v1.2.
 *
 * Wraps the ESP-IDF <peripheral> HAL and exposes a typed struct with bound
 * operations. Pin/channel aliases follow the board's J1/J3 header silkscreen.
 *
 * @version 0.1
 * @date <date>
 */

#ifndef <NAME>_DRIVERS_H
#define <NAME>_DRIVERS_H

#include <driver/<peripheral>.h>
#include <esp_err.h>

/**
 * @brief Board-specific pin/channel aliases.
 *
 * Document any caveats (strapping pins, shared functions, voltage limits).
 */
typedef enum
{
  /* add aliases that match the board silkscreen */
} <name>_pinout_t;

/**
 * @brief State / logic-level enum (if the peripheral is digital).
 */
typedef enum
{
  <NAME>_STATE_LOW  = 0,
  <NAME>_STATE_HIGH = 1,
} <name>_state_t;

/**
 * @brief <Name> object: configuration + bound operations.
 *
 * Callers populate the public fields (pin, mode, …) then call <name>_init().
 * Fields prefixed with _ are filled in by init() and must not be written by
 * callers afterward.
 */
typedef struct <name>
{
  struct <name>  *next;          /**< Optional linked-list pointer */
  <name>_pinout_t pin;           /**< Physical pin / channel */

  /* Configuration — set before init(), do not modify afterward */
  <name>_state_t  _act_state;    /**< Initial output level */
  <peripheral>_config_t _config; /**< Low-level config snapshot */
  <peripheral>_mode_t   _mode;   /**< Input / output / bidirectional */

  /* ISR support (leave NULL for output-only peripherals). Underscore-prefixed:
     devices also reuse _isr_handler_arg to carry their private context. */
  void (*_isr_handler)(void *);
  void  *_isr_handler_arg;

  /* Bound operations — wired by <name>_init(), call via static functions */
  esp_err_t    (*init)(struct <name> *self);
  esp_err_t    (*set_state)(struct <name> *self, <name>_state_t state);
  <name>_state_t (*get_state)(struct <name> *self);
  esp_err_t    (*toggle)(struct <name> *self);
} <name>_t;

/* Public static functions — prefer these over calling function pointers directly */
esp_err_t      <name>_init(<name>_t *self);
esp_err_t      <name>_write(<name>_t *self, <name>_state_t state);
<name>_state_t <name>_read(<name>_t *self);
esp_err_t      <name>_toggle(<name>_t *self);

/* ISR helpers (only for drivers that support interrupt-driven input) */
esp_err_t <name>_enable_isr(<name>_t *self);
esp_err_t <name>_disable_isr(<name>_t *self);

#endif  // <NAME>_DRIVERS_H
```

---

## Source template

```c
/**
 * @file <name>_drivers.c
 * @brief <Peripheral> driver implementation.
 */

#include "<name>_drivers.h"

#include <esp_check.h>
#include <esp_log.h>
#include <stdbool.h>

#define <NAME>_ISR_SERVICE_FLAGS 0

static const char *TAG = "<NAME>";

/* Track one-time process-level resource installation (ISR service, etc.).
   This is safe because ESP-IDF does not allow installing the service twice. */
static bool _isr_service_installed = false;

/* -----------------------------------------------------------------------
 * Static helpers — one per configuration mode.
 * Using static helpers keeps gpio_init() readable and allows the modes
 * to evolve independently without touching the public interface.
 * --------------------------------------------------------------------- */

static esp_err_t _<name>_config_output(<name>_pinout_t pin)
{
  <peripheral>_config_t cfg = {
    .pin_bit_mask = (1ULL << pin),
    .mode         = <PERIPHERAL>_MODE_OUTPUT,
    .pull_up_en   = <PERIPHERAL>_PULLUP_DISABLE,
    .pull_down_en = <PERIPHERAL>_PULLDOWN_DISABLE,
    .intr_type    = <PERIPHERAL>_INTR_DISABLE,
  };
  ESP_RETURN_ON_ERROR(<peripheral>_config(&cfg), TAG, "config failed for pin %d", pin);
  ESP_LOGI(TAG, "Configured pin %d as output", pin);
  return ESP_OK;
}

static esp_err_t _<name>_config_input(<name>_pinout_t pin,
                                       void isr_handler(void *),
                                       void *isr_handler_arg)
{
  <peripheral>_config_t cfg = {
    .pin_bit_mask = (1ULL << pin),
    .mode         = <PERIPHERAL>_MODE_INPUT,
    .pull_up_en   = <PERIPHERAL>_PULLUP_ENABLE,
    .pull_down_en = <PERIPHERAL>_PULLDOWN_DISABLE,
    .intr_type    = <PERIPHERAL>_INTR_NEGEDGE,
  };
  ESP_RETURN_ON_ERROR(<peripheral>_config(&cfg), TAG, "config failed for pin %d", pin);
  ESP_LOGI(TAG, "Configured pin %d as input", pin);

  if (isr_handler == NULL)
    return ESP_OK;

  ESP_RETURN_ON_ERROR(<peripheral>_isr_handler_add(pin, isr_handler, isr_handler_arg),
                      TAG, "isr_handler_add failed for pin %d", pin);
  ESP_LOGI(TAG, "ISR handler registered for pin %d", pin);
  return ESP_OK;
}

/* -----------------------------------------------------------------------
 * Public static operations — these are also bound to function pointers
 * inside <name>_init() so callers can use either style.
 * --------------------------------------------------------------------- */

esp_err_t <name>_write(<name>_t *self, <name>_state_t state)
{
  self->_act_state = state;
  return <peripheral>_set_level(self->pin, (uint32_t)state);
}

<name>_state_t <name>_read(<name>_t *self)
{
  return (<name>_state_t)<peripheral>_get_level(self->pin);
}

esp_err_t <name>_toggle(<name>_t *self)
{
  /* Toggle from the tracked output state (_act_state) rather than reading
     the pin level — output pins have their input buffer disabled. */
  return <name>_write(self,
    self->_act_state == <NAME>_STATE_LOW ? <NAME>_STATE_HIGH : <NAME>_STATE_LOW);
}

esp_err_t <name>_init(<name>_t *self)
{
  ESP_RETURN_ON_FALSE(self != NULL, ESP_ERR_INVALID_ARG, TAG, "self is NULL");

  /* Bind function pointers so callers can dispatch via the struct. */
  self->init      = NULL;
  self->get_state = &<name>_read;
  self->set_state = &<name>_write;
  self->toggle    = &<name>_toggle;

  /* Install the ISR service once per process — safe to call from multiple
     <name>_init() calls because we gate it with the static flag. */
  if (!_isr_service_installed)
  {
    ESP_RETURN_ON_ERROR(<peripheral>_install_isr_service(<NAME>_ISR_SERVICE_FLAGS),
                        TAG, "install_isr_service failed");
    _isr_service_installed = true;
  }

  switch (self->_mode)
  {
    case <PERIPHERAL>_MODE_OUTPUT:
      ESP_RETURN_ON_ERROR(_<name>_config_output(self->pin), TAG, "config_output failed");
      return <name>_write(self, self->_act_state);

    case <PERIPHERAL>_MODE_INPUT:
      return _<name>_config_input(self->pin, self->_isr_handler, self->_isr_handler_arg);

    default:
      ESP_LOGE(TAG, "Invalid mode for pin %d", self->pin);
      return ESP_ERR_INVALID_ARG;
  }
}

esp_err_t <name>_enable_isr(<name>_t *self)
{
  return <peripheral>_intr_enable(self->pin);
}

esp_err_t <name>_disable_isr(<name>_t *self)
{
  return <peripheral>_intr_disable(self->pin);
}
```

---

## CMakeLists.txt

```cmake
idf_component_register(SRCS "<name>_drivers.c"
                    INCLUDE_DIRS "include"
                    REQUIRES esp_driver_<peripheral>)
```

Drivers only depend on the matching ESP-IDF driver component. Never pull in
`freertos` or device components — that would invert the dependency hierarchy.

---

## Key invariants

1. **No malloc** — the struct is owned by the caller (static, stack, or device's private context). Drivers are stateless beyond what lives in `<name>_t`.
2. **Function pointers bound in `init()`** — allows callers to dispatch via `self->set_state(self, val)` or via the named static function; both routes work.
3. **Shared resource guarded by a static flag** — `_isr_service_installed` ensures `gpio_install_isr_service()` runs exactly once even if multiple pins are initialized.
4. **`_act_state` mirrors the driven level** — output pins have their input buffer disabled, so `<peripheral>_get_level()` is unreliable for outputs. Always track the last written level in `_act_state` and use it for toggle.
5. **`init()` returns `esp_err_t` and propagates** — a misconfigured pin is reported, not aborted on. The caller (a device's `init`, or `app_main` via `ESP_ERROR_CHECK`) decides whether it is fatal. A driver that calls `ESP_ERROR_CHECK` itself takes that decision away from every consumer, including tests.

---

## Checklist

- [ ] Include guard: `<NAME>_DRIVERS_H`
- [ ] Pin alias enum documents strapping pins, shared functions, voltage limits
- [ ] Struct has `_act_state`, `_config`, `_mode` with `_` prefix; `pin` is public
- [ ] `_isr_handler` and `_isr_handler_arg` fields present (NULL for output-only)
- [ ] `init()` returns `esp_err_t` and propagates; no `ESP_ERROR_CHECK` inside the driver
- [ ] Function pointers `init`, `set_state`, `get_state`, `toggle` declared in struct
- [ ] Static config helpers, one per mode (`_config_output`, `_config_input`)
- [ ] `_isr_service_installed` flag guards one-time ISR service install
- [ ] `toggle()` uses `_act_state`, not `get_level()`
- [ ] `init()` binds function pointers before configuring the pin
- [ ] CMakeLists `REQUIRES` only `esp_driver_<peripheral>` — no freertos, no devices
- [ ] TAG = peripheral name uppercase, init logs pin number and mode
