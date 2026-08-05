# App Component Pattern

The **app** (`main/`) is the top-level entry point. It owns the top-level
instances, calls their `init()` functions in the correct order, wires the
callbacks between them, and then keeps the main task alive with `portMAX_DELAY`.
It does not contain hardware logic — that belongs in devices and drivers.

**Once a manager exists, `main.c` owns managers, not the devices under them.**
A manager owns its peripherals (the notification-manager owns the four LEDs, the
actuator-manager owns the relay and button), so `main.c` populates the pins on
the manager's embedded device structs and hands the whole manager to `init()`.
It never calls `led_on()` or `relay_off()` itself — that would add a second
writer to hardware the manager arbitrates. Use the template below verbatim only
for a flat app with no manager layer.

---

## File layout

```text
main/
├── main.c
└── CMakeLists.txt
```

---

## main.c structure

```c
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <inttypes.h>

/* Include one header per device used. */
#include "gpio_drivers.h"
#include "<device_a>_device.h"
#include "<device_b>_device.h"

/*
 * Board-level overview comment:
 *   - <Device A> : <what it does>  -> <PIN>
 *   - <Device B> : <what it does>  -> <PIN>
 *
 * Describe non-obvious wiring choices or lockout windows here.
 */

/* -----------------------------------------------------------------------
 * Pin assignments — one #define per peripheral.
 * Use symbolic names from gpio_pinout_t (IO4, IO9, …) not raw numbers.
 * --------------------------------------------------------------------- */
#define <DEVICE_A>_PIN  IO11
#define <DEVICE_B>_PIN  IO9

/* -----------------------------------------------------------------------
 * Device instances — static storage, initialized in app_main().
 * Callers only need to set the pin before init(); everything else is
 * handled internally by the device.
 * --------------------------------------------------------------------- */
static <device_a>_t <device_a> = {
  ._gpio = {.pin = <DEVICE_A>_PIN},
};

static <device_b>_t <device_b> = {
  ._gpio = {.pin = <DEVICE_B>_PIN},
  /* populate any public configuration fields here (debounce_ms, on_press …) */
};

static const char *TAG = "<app_name>";

/* -----------------------------------------------------------------------
 * Callbacks — only if a device fires events (e.g. button on_press).
 * Keep callbacks thin: update state, log, delegate to a device.
 * --------------------------------------------------------------------- */
static void on_<event>(/* device type */ *self)
{
  /* e.g. toggle an actuator, log a reading */
}

/* -----------------------------------------------------------------------
 * app_main — ESP-IDF entry point.
 * Order: init all devices → start cyclic operations → log banner → sleep.
 * --------------------------------------------------------------------- */
void app_main(void)
{
  ESP_LOGI(TAG, "Application started");

  <device_a>_init(&<device_a>);
  <device_b>_init(&<device_b>);

  /* Start any cyclic operations after all inits are complete. */
  <device_a>_cycle(&<device_a>, 1, 0);   /* example: blink/pulse indefinitely */

  ESP_LOGI(TAG,
           "<DEVICE_A>=GPIO%d  <DEVICE_B>=GPIO%d",
           <DEVICE_A>_PIN, <DEVICE_B>_PIN);
  ESP_LOGI(TAG, "Minimum free heap: %" PRIu32 " bytes",
           esp_get_minimum_free_heap_size());

  /* Main task has nothing else to do — let device tasks and timers run. */
  while (1)
    vTaskDelay(portMAX_DELAY);
}
```

---

## CMakeLists.txt

```cmake
idf_component_register(SRCS "main.c"
                    INCLUDE_DIRS "."
                    REQUIRES
                        esp_log
                        freertos
                        gpio
                        <device_a>
                        <device_b>)
```

---

## Rules

1. **No hardware logic in `main.c`** — if you find yourself calling `gpio_write()` directly in `app_main()`, move it into a device. If a manager owns that peripheral, call the manager's setter instead of the device.
2. **Init order matters** — NVS and other shared subsystems first, then managers, then anything that registers a callback into them. Register callbacks *before* starting whatever can fire them, so no event slips past unnoticed.
3. **Static instances** — app-level objects are `static` globals with only their public fields (pins, config) populated. No heap allocation at the top level.
4. **Wrap `init()` in `ESP_ERROR_CHECK`** — `init()` returns `esp_err_t`; `app_main` is where the decision "this failure is fatal" belongs.
5. **Callbacks stay thin** — an `on_press` or `on_connected` callback should call one manager function and return. It runs on someone else's task.
6. **End with `portMAX_DELAY`** — `app_main()` must not return; returning terminates the task and triggers a watchdog reset.

---

## Checklist

- [ ] One `#define` per pin, using `gpio_pinout_t` symbolic names
- [ ] All instances are `static` with only `.pin` (and public config) populated
- [ ] Managers, not raw devices, where a manager owns the peripheral
- [ ] `app_main()` inits in dependency order (NVS → managers → consumers)
- [ ] Every `init()` wrapped in `ESP_ERROR_CHECK`
- [ ] Callbacks registered before the producer that fires them is started
- [ ] Cyclic operations started after all `init()` calls complete
- [ ] Log banner prints all pin assignments and heap size
- [ ] `while(1) vTaskDelay(portMAX_DELAY)` at the end
- [ ] No direct ESP-IDF GPIO or timer calls — all hardware goes through device APIs
