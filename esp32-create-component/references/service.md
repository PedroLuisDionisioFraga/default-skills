# Service Component Pattern

A **service** is a background FreeRTOS task that manages ongoing work
independently of `app_main` — an MQTT session, SNTP synchronization, OTA
updates, a sensor polling loop.

A service owns a *resource* (a client handle, a socket, a protocol session) the
same way a manager owns a peripheral: **one worker task is the only code that
touches it**, and every other context asks through a queue or an event group.
Producers never call the underlying library directly.

---

## File layout

```text
components/services/<name>/
├── include/
│   └── <name>_service.h
├── src/
│   ├── <name>_service.c        ← worker task, resource lifecycle, event handler
│   └── <name>_service_store.c  ← Kconfig defaults + NVS persistence (if any)
├── CMakeLists.txt
├── Kconfig.projbuild           ← if the service is configurable
└── README.md
```

Sources live in `src/`. Split persistence into its own `_store.c` as soon as the
service has NVS-backed configuration — it keeps the runtime file about the
resource and nothing else. See `components/services/mqtt_service/` and
`components/services/ntp/`.

---

## Naming: one prefix per component

The single most common defect in this layer is a component that grows three
prefixes. Pick them once, up front:

| Kind | Rule | Example (`mqtt_service`) |
|---|---|---|
| Types | full component name | `mqtt_service_msg_t`, `mqtt_service_config_t` |
| Public functions | full component name + verb | `mqtt_service_publish()` |
| Macros / enum members | one short abbreviation | `MQTT_SVC_EVT_CONNECTED`, `MQTT_SVC_MSG_PUBLISH`, `MQTT_SVC_TOPIC_MAX` |
| Kconfig | untouched IDF namespace | `CONFIG_MQTT_SERVICE_URI` |
| Static helpers | leading `_`, no prefix | `_client_stop_destroy()`, `_handle_msg()` |

Public structs get a named tag (`typedef struct mqtt_service_msg { … }`), never
an anonymous one. Persistence pairs are symmetric: `<name>_service_store_load()`
/ `<name>_service_store_save()` — matching `time_scheduling_store_load/save`.

---

## Header template

```c
/**
 * @file <name>_service.h
 * @brief Service that owns <the resource> and serializes every operation
 *        through a single worker task.
 *
 * Threading model:
 *   - Producers call the operation wrappers, which copy the request into the
 *     queue and return immediately (never block).
 *   - One internal worker task drains the queue and is the only owner of
 *     <the resource> — it creates, starts, stops and destroys it.
 *   - <Any library callback context, e.g. the esp-mqtt event task> reports
 *     transitions; the registered callbacks run there and must stay short.
 *
 * Configuration precedence: Kconfig defaults, then NVS overlaid on top.
 *
 * Singleton: the private context lives in a file-static inside the .c, so only
 * one instance of the service is supported.
 */

#ifndef <NAME>_SERVICE_H
#define <NAME>_SERVICE_H

#include <esp_err.h>
#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

/* Status bits, set on transition on the service's event group — obtained via
 * <name>_service_get_event_group(). Consumers block on these instead of
 * polling. */
#define <NAME>_EVT_READY ((EventBits_t)(1 << 0))
#define <NAME>_EVT_ERROR ((EventBits_t)(1 << 1))

typedef enum
{
  <NAME>_MSG_START = 0,
  <NAME>_MSG_STOP,
  /* one member per operation the worker performs */
} <name>_service_msg_type_t;

/** One queued request. Copied by value, so caller buffers need not outlive the post. */
typedef struct <name>_service_msg
{
  <name>_service_msg_type_t type;
  /* fixed-size payload — see the sizing note under Rules */
} <name>_service_msg_t;

/** Fired from <the callback context> — keep short / non-blocking. */
typedef void (*<name>_service_event_cb_t)(bool ok);

/**
 * @brief Initialize the service: create the event group, queue and worker task.
 *
 * @param config Optional; NULL loads it via <name>_service_store_load().
 * @return ESP_OK (including a second call while already running),
 *         ESP_ERR_NO_MEM on allocation failure.
 */
esp_err_t <name>_service_init(const <name>_service_config_t *config);

/** @brief Bring the resource up (queues a start request). */
esp_err_t <name>_service_start(void);

/** @brief Tear down worker, queue and event group. Idempotent. */
esp_err_t <name>_service_deinit(void);

/**
 * @brief Post a request to the service queue (non-blocking).
 *
 * @return ESP_OK, ESP_ERR_INVALID_ARG if @p msg is NULL,
 *         ESP_ERR_INVALID_STATE if not initialized, ESP_ERR_TIMEOUT if full.
 */
esp_err_t <name>_service_post(const <name>_service_msg_t *msg);

/** @brief The status event group, or NULL if not initialized. */
EventGroupHandle_t <name>_service_get_event_group(void);

/** @brief Register the transition callback (NULL clears it). */
esp_err_t <name>_service_set_event_cb(<name>_service_event_cb_t cb);

#ifdef __cplusplus
}
#endif

#endif  // <NAME>_SERVICE_H
```

---

## Source template

The private context replaces every scattered file-static. Group the fields by
who may touch them and say so in comments — that is what makes the concurrency
reviewable:

```c
#include "<name>_service.h"

#include <esp_check.h>
#include <esp_log.h>
#include <freertos/queue.h>
#include <freertos/task.h>

#define <NAME>_QUEUE_DEPTH 16
#define <NAME>_TASK_STACK  4096
#define <NAME>_TASK_PRIO   5

static const char *TAG = "<NAME>_SVC";

typedef struct
{
  /* --- Handles. Created in init, read-only afterwards. --- */
  QueueHandle_t      queue;
  TaskHandle_t       task;
  EventGroupHandle_t events;
  <resource_handle_t> resource;  /* created/destroyed only on the worker */

  /* --- Config. Written once in init, read-only afterwards. --- */
  <name>_service_config_t cfg;

  /* --- Flags written by one task and read by others: volatile is the
   * barrier. Single aligned words written whole, never read-modify-written,
   * so no lock is needed on top. --- */
  volatile bool running;
  volatile bool ready;

  /* --- Consumer callbacks, fired from <the callback context>. --- */
  <name>_service_event_cb_t event_cb;
} <name>_service_priv_t;

static <name>_service_priv_t s_priv; /* zero-init; program lifetime */

/* -----------------------------------------------------------------------
 * Worker task — drains the queue; the single owner of the resource.
 * --------------------------------------------------------------------- */
static void _<name>_task(void *arg)
{
  (void)arg;
  ESP_LOGI(TAG, "<Name> service started");
  <name>_service_msg_t msg;

  while (s_priv.running)
  {
    if (xQueueReceive(s_priv.queue, &msg, portMAX_DELAY) != pdTRUE)
      continue;
    if (!s_priv.running)
      break;
    _handle_msg(&msg);
  }

  _resource_stop_destroy();
  ESP_LOGI(TAG, "<Name> service stopped");
  s_priv.task = NULL;   /* deinit polls this to confirm the exit */
  vTaskDelete(NULL);
}

/* -----------------------------------------------------------------------
 * Public API
 * --------------------------------------------------------------------- */
esp_err_t <name>_service_init(const <name>_service_config_t *config)
{
  if (s_priv.running)
    return ESP_OK;  /* idempotent */

  if (config)
    s_priv.cfg = *config;
  else
    ESP_RETURN_ON_ERROR(<name>_service_store_load(&s_priv.cfg), TAG, "store_load failed");

  s_priv.events = xEventGroupCreate();
  if (!s_priv.events)
    return ESP_ERR_NO_MEM;

  s_priv.queue = xQueueCreate(<NAME>_QUEUE_DEPTH, sizeof(<name>_service_msg_t));
  if (!s_priv.queue)
  {
    vEventGroupDelete(s_priv.events);
    s_priv.events = NULL;
    return ESP_ERR_NO_MEM;
  }

  s_priv.running = true;
  if (xTaskCreate(_<name>_task, "<name>_svc", <NAME>_TASK_STACK, NULL,
                  <NAME>_TASK_PRIO, &s_priv.task) != pdPASS)
  {
    ESP_LOGE(TAG, "xTaskCreate(<name>_svc) failed");
    s_priv.running = false;
    vQueueDelete(s_priv.queue);
    s_priv.queue = NULL;
    vEventGroupDelete(s_priv.events);
    s_priv.events = NULL;
    return ESP_ERR_NO_MEM;
  }

  return ESP_OK;
}

esp_err_t <name>_service_post(const <name>_service_msg_t *msg)
{
  ESP_RETURN_ON_FALSE(msg != NULL, ESP_ERR_INVALID_ARG, TAG, "msg is NULL");
  if (!s_priv.queue)
    return ESP_ERR_INVALID_STATE;
  return xQueueSend(s_priv.queue, msg, 0) == pdTRUE ? ESP_OK : ESP_ERR_TIMEOUT;
}

esp_err_t <name>_service_deinit(void)
{
  if (!s_priv.running)
    return ESP_OK;  /* idempotent */

  s_priv.running = false;

  /* Unblock the worker if it is parked on an empty queue. Never delete the
   * task from outside: it must run its own cleanup before exiting. */
  <name>_service_msg_t wake = {.type = <NAME>_MSG_STOP};
  xQueueSend(s_priv.queue, &wake, 0);

  for (int i = 0; i < 50 && s_priv.task != NULL; i++)
    vTaskDelay(pdMS_TO_TICKS(20));

  if (s_priv.queue)
  {
    vQueueDelete(s_priv.queue);
    s_priv.queue = NULL;
  }
  if (s_priv.events)
  {
    vEventGroupDelete(s_priv.events);
    s_priv.events = NULL;
  }
  s_priv.event_cb = NULL;
  return ESP_OK;
}
```

Publish a state transition in **one** place, so the flag, the event bits and the
callback can never disagree:

```c
static void _set_ready(bool ready)
{
  if (s_priv.ready == ready)
    return;

  s_priv.ready = ready;
  if (s_priv.events)
  {
    xEventGroupClearBits(s_priv.events, ready ? <NAME>_EVT_ERROR : <NAME>_EVT_READY);
    xEventGroupSetBits(s_priv.events, ready ? <NAME>_EVT_READY : <NAME>_EVT_ERROR);
  }
  if (s_priv.event_cb)
    s_priv.event_cb(ready);
}
```

---

## CMakeLists.txt

```cmake
idf_component_register(
    SRCS "src/<name>_service.c" "src/<name>_service_store.c"
    INCLUDE_DIRS "include"
    REQUIRES freertos esp_event log <other deps>
)
```

---

## Rules

1. **One worker owns the resource.** Every `create` / `start` / `stop` /
   `destroy` call happens on the worker task and nowhere else. Producers post.
2. **Private context, not scattered statics.** A single `<name>_service_priv_t
   s_priv` with ownership comments per field group. Loose file-statics leave no
   place to document who writes what, which is where the races hide.
3. **`volatile` on every flag crossed between tasks.** A plain `bool` written by
   a callback task and read by `app_main` may be cached in a register. Only
   single aligned words qualify; multi-word state needs a mutex.
4. **`start()` and `deinit()` are idempotent** — a second call returns `ESP_OK`.
5. **No blocking in `post()`** — timeout `0`; the caller decides what a full
   queue means.
6. **The worker deletes itself.** Set `running = false`, wake it, and poll its
   handle. Never `vTaskDelete()` a worker from outside: it skips the cleanup.
7. **Operations return `esp_err_t`** — including callback setters. Only `init()`
   on hardware devices returns `void`.
8. **Size the queue message deliberately.** It is copied by value, so
   `depth * sizeof(msg)` is allocated up front — a 512-byte payload at depth 16
   costs ~10 KB of RAM. State the number in the header if it is large.

---

## Worked examples

- **`components/services/mqtt_service/`** — the full shape: private context with
  ownership zones, worker-owned client handle, event-group + callback status
  sinks, `src/` split with `mqtt_service_store.c` for the Kconfig/NVS config.
- **`components/services/ntp/`** — the simpler variant: no queue, driven by
  ESP-IDF's SNTP event callbacks, with a one-shot first-sync callback.

For the concurrency reasoning behind all of this — the four ownership zones, the
check-then-act failure mode, when an event group beats a queue — read
`.claude/skills/esp32-firmware-engineer/references/single-owner-io.md`.

---

## Checklist

- [ ] All private state in one `<name>_service_priv_t s_priv`, no loose statics
- [ ] Field groups carry ownership comments (who writes, who reads, what guards)
- [ ] `volatile` on every cross-task flag; mutex on any multi-word shared state
- [ ] One prefix family: `<name>_service_*` types/functions, one `<NAME>_*` macro abbreviation
- [ ] Public structs use a named tag, not `typedef struct { … }`
- [ ] Sources under `src/`; NVS config split into `<name>_service_store.c`
- [ ] Persistence named `<name>_service_store_load` / `_store_save`
- [ ] `init()` creates event group, then queue, then task; rolls back on each failure
- [ ] Task loop uses `xQueueReceive(portMAX_DELAY)` — no busy-wait
- [ ] `deinit()` sets `running = false`, wakes the worker, waits for it to clear its handle
- [ ] Stack size, priority and queue depth are named constants
- [ ] Header documents the threading model, callback context and singleton nature
- [ ] CMakeLists lists only direct dependencies
