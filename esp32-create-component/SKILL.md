---
name: esp32-create-component
description: >
  Use this skill whenever the user asks to create, scaffold, or review any
  component in this ESP32-IDF project (fw_sbc-wxxx): hardware devices, GPIO/
  peripheral drivers, application entry points, background services, or utility
  libraries. Trigger on requests like "add a buzzer device", "create a UART
  driver", "I need a background task for WiFi", "add a sensor", "implement a
  stepper motor component", or any phrasing that involves writing or reviewing
  a new ESP-IDF component. When in doubt, use this skill — it encodes the exact
  conventions already in use in this codebase.
---

# ESP32 Component Pattern — Router

This project organizes all code as ESP-IDF components under `components/`.
Every component is a self-contained directory with a `CMakeLists.txt` and an
`include/` subfolder.

```text
components/
├── app/       ← managers that own a resource and arbitrate its writers
├── devices/   ← high-level hardware abstractions  (LED, Relay, Button …)
├── drivers/   ← low-level HAL wrappers            (GPIO, NVS …)
├── lib/       ← pure-C utilities                  (time_scheduling, astro_clock …)
└── services/  ← background workers                (mqtt_service, ntp …)
```

---

## Step 1 — Classify the component

Pick the type that best fits. Then read the matching reference file below for
the concrete pattern, naming rules, and checklist.

| Type | When to use | Reference |
|---|---|---|
| **Device** | A physical peripheral the app controls or reads (LED, relay, button, sensor, buzzer, motor …) | `references/device.md` |
| **Driver** | A thin HAL wrapper for an ESP32 peripheral (GPIO, UART, SPI, I2C, ADC …) | `references/driver.md` |
| **Manager** | Owns one or more peripherals and arbitrates between everything that wants to write them (notification, actuator, communication) | see below |
| **Service** | A background FreeRTOS task that manages ongoing work (MQTT, SNTP, OTA …) | `references/service.md` |
| **Lib** | Pure-C utility with no hardware dependency (scheduling, astronomy, CRC …) | `references/lib.md` |
| **App** | The top-level application (`main/`) — wires managers and services together | `references/app.md` |

**Managers have no template file here.** They are the single-owner pattern, and
it is documented in full — with the four ownership zones, the failure modes it
prevents, and both of this repo's implementations as worked examples — in
`.claude/skills/esp32-firmware-engineer/references/single-owner-io.md`. Read that
before writing one. A ready-made scaffold sits in the same skill under
`assets/templates/single-owner-manager/`.

Manager vs service: a **manager** owns hardware and decides who may write it; a
**service** owns a non-hardware resource (a session, a socket, a client handle)
and serializes work against it. Both use a worker task and a private context; the
structural rules are identical.

---

## Common rules (apply to every component type)

These rules hold regardless of which reference file you follow.

### Naming
- Component directory: `<name>/` — snake_case, singular
- Source files: `<name>_<type>.c` / `<name>_<type>.h` (e.g. `relay_device.c`, `gpio_drivers.h`)
- Functions: `<name>_<verb>()` — snake_case, prefixed with the component name
- Public struct: `<name>_t`, declared with a named tag (`typedef struct relay { … } relay_t;`)
- Private context struct: `<name>_priv_t` — see "Private state" below
- State/mode enums: `<NAME>_STATE_<VALUE>` — SCREAMING_SNAKE_CASE
- Internal fields on public structs: leading `_` (e.g. `_gpio`, `_state`) to signal "don't touch"
- Static helpers inside a `.c`: leading `_`, no component prefix (`_render`, `_apply_relay`)

**One prefix family per component.** Types and public functions carry the full
component name; macros and enum members carry one short abbreviation, chosen
once. `mqtt_service` uses `mqtt_service_msg_t` + `MQTT_SVC_MSG_PUBLISH`;
`notification_manager` uses `notification_conn_state_t` + `NOTIF_EVT_CONN_*`.
Three competing prefixes in one component is the most common defect here.

### Private state
- Everything private goes in **one** `<name>_priv_t`, never in scattered file-statics
- Group the fields by who may touch them and comment each group — handles written
  once in init, state shared across tasks, mutex-guarded multi-word config,
  worker-owned caches
- Singleton components (managers, services) use `static <name>_priv_t s_priv;`
- Per-instance components (devices) hang the context off the public struct — the
  LED/relay/button devices stash it in `_gpio._isr_handler_arg`
- Any flag written by one task and read by another must be `volatile`, and must
  fit in a single aligned word; multi-word shared state needs a mutex
- The concurrency reasoning behind this lives in
  `.claude/skills/esp32-firmware-engineer/references/single-owner-io.md`

### Error handling
- `init()` returns `esp_err_t` and propagates failures — the **caller** decides
  fatality, typically `ESP_ERROR_CHECK(relay_init(&r));` in `app_main`
- All operation functions return `esp_err_t`, including callback setters
- Validate arguments with `ESP_RETURN_ON_FALSE(...)`; propagate ESP-IDF errors
  with `ESP_RETURN_ON_ERROR(...)` (both from `<esp_check.h>`)
- Use `ESP_ERR_INVALID_ARG` for bad parameters, `ESP_ERR_INVALID_STATE` when the
  component is not initialised, `ESP_ERR_NO_MEM` for allocation failures
- Every failure path in `init()` rolls back what it already created

### Logging
- Every `.c` file has `static const char *TAG = "<NAME>";`
- Use `ESP_LOGI` for normal events, `ESP_LOGE` for errors
- Always log the GPIO pin number (or resource ID) in `init()` so the boot log is self-documenting

### CMakeLists.txt skeleton
```cmake
idf_component_register(SRCS "<name>_<type>.c"
                    INCLUDE_DIRS "include"
                    REQUIRES <deps>)
```
Only list direct dependencies in `REQUIRES` — never pull in unused components.

Single-source components keep the `.c` at the component root (devices, managers).
Components with more than one source put them under `src/` and list the paths —
services and libs do this, splitting NVS persistence into its own `_store.c`:

```cmake
idf_component_register(
    SRCS "src/<name>_service.c" "src/<name>_service_store.c"
    INCLUDE_DIRS "include"
    REQUIRES <deps>
)
```

---

## Step 2 — Read the reference file

Open the file for your component type and follow its pattern.
The reference files contain full code templates, key invariants, and a done-checklist.
