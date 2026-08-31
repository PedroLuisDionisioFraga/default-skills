# fw_sbc-wxxx — App architecture (actuator-manager & mqtt-manager)

Read this when implementing or reviewing code in `components/app/` for the street-luminaire controller (`fw_sbc-wxxx`, target `esp32c6`). Full project graph: `docs/architecture.md`.

## Layering (this repo)

| Layer | Role | Examples in this project |
|-------|------|--------------------------|
| **Driver** | Touches silicon (registers, peripherals) | `gpio`, `nvs` |
| **Device** | Physical board part composed from drivers | `relay`, `led`, `button`, `ethernet` |
| **Library** | Hardware-agnostic reusable logic | `mqtt_payloads`, `mqtt_topics`, `event_store`, `time_scheduling`, `astro_clock`, `system_time` |
| **Service** | Stateful product capability on drivers + libs | `mqtt_service`, `sntp_service` |
| **App** | Product wiring and managers | `app-manager`, `actuator-manager`, `mqtt-manager`, `notification`, `communication` |

Managers own **behavior and coordination**, not registers. JSON/topic strings live in libs; the MQTT client lives in `mqtt_service`; the relay lives in `actuator-manager`.

## Split-component pattern (actuator-manager & mqtt-manager)

Both app managers are **one ESP-IDF component, many `.c` siblings**, sharing an internal `src/*_priv.h`. Only `include/*.h` is public.

```
component/
  include/<name>.h          ← public API only
  src/<name>_priv.h         ← cross-TU types + internal prototypes
  src/<name>.c              ← init, wiring, singleton context
  src/<name>_*.c            ← one concern per file
  CMakeLists.txt            ← lists every .c under src/
```

**Rules when adding code:**

- Put new logic in the file that already owns that concern; do not grow the init/wiring file unless it is registration/wiring.
- Shared helpers used by multiple siblings → `*_common.c` or the priv header prototypes.
- File-static buffers for large MQTT/S43/S15 payloads (4 KB+) — never on the stack of the esp-mqtt or esp_timer tasks.
- Internal cross-file functions: `act_*` / `mqtt_mgr_*` prefixes; **no leading `_`** on exported TU symbols (ESP-IDF flat link namespace).
- Single-file helpers: `static _name`.

---

## actuator-manager

**Path:** `components/app/actuator-manager/`  
**Public API:** `include/actuator_manager.h` — `act_manager_*`  
**Singleton:** private context in `actuator_manager_priv.h`; worker owns the relay.

### Responsibility

- Owns **luminaire relay**, **button**, and **`light_op_mode_t`** (NVS key `op_mode`).
- Single worker task (`act_sched_task`) is the **only writer** to the relay (see `references/single-owner-io.md`).
- Dispatches schedule evaluation by mode: 0/1 inline, 2 → `astro_clock`, 3 → `time_scheduling`, 4 → hybrid.
- MQTT B28 override arbitration lives here (`act_evaluate_schedule`), not in mqtt-manager.
- Pushes visual state to `notification-manager` only; does not touch status LEDs directly.

### Source layout

| File | Responsibility |
|------|----------------|
| `src/actuator_manager.c` | Singleton, `act_manager_init()`, lifecycle/status API |
| `src/actuator_manager_relay.c` | Worker task, event bits, relay writes, `act_drive_relay()` |
| `src/actuator_manager_eval.c` | `act_evaluate_schedule()` — decide-and-drive + MQTT override |
| `src/actuator_manager_decision.c` | `act_should_be_on()`, forward sim for `get_next_transitions()` |
| `src/actuator_manager_config.c` | NVS-mirror cache, `set_*` / `get_*` configuration API |
| `src/actuator_manager_store.c` | Persist `op_mode` |
| `src/actuator_manager_button.c` | Hold-band gestures, feedback overlay |
| `src/actuator_manager_history.c` | Transition rings for DEF-C71 S43 |
| `src/actuator_manager_log.c` | Timestamp/format helpers |

### Concurrency

- Requesters (button, MQTT via `act_manager_command_relay`, setters) **set event bits** or update pending commands — they never call `relay_on/off` directly.
- `act_manager_command_relay()` (B28): remote override until next profile transition **or** `CONFIG_ACTUATOR_MQTT_OVERRIDE_TIMEOUT_S` (default 12 h); on release forces persisted mode to Astronomical.
- Maintenance mode refuses remote relay commands (`ESP_ERR_INVALID_STATE`).

### Where to change what

| Task | File |
|------|------|
| New schedule/mode behavior | `actuator_manager_eval.c`, maybe `decision.c` |
| New NVS field for profile | `config.c` + lib store if shared |
| Relay movement side effects (events) | `actuator_manager_relay.c` |
| Button gesture | `actuator_manager_button.c` |
| S43 transition history | `actuator_manager_history.c` |

---

## mqtt-manager

**Path:** `components/app/managers/mqtt-manager/`  
**Public API:** `include/mqtt_manager.h` — only `mqtt_manager_init(notifier, actuator)`  
**Internal context:** `g_mqtt_mgr` in `src/mqtt_manager_common.c` (notifier + actuator pointers).

### Responsibility

DEF-C71 **protocol wiring** only — no hardware, no worker task, no mutex:

- Registers `k_topics[]` and inbound handlers with `mqtt_service`.
- Maps MQTT session up/down → `NOTIF_ERR_MQTT` on the notification-manager.
- Inbound: parse (`mqtt_payloads`) → call actuator or `event_store` → publish response via `mqtt_service_publish_topic`.
- Outbound S15: subscribe to `event_store` notify callback, debounce, publish spontaneous events.

**All inbound handlers run synchronously on the esp-mqtt event task** — no queue between RX and handler. B43 (`act_manager_set_profile`) is the deliberate exception that may block on NVS on that task.

### Source layout

| File | Responsibility |
|------|----------------|
| `src/mqtt_manager.c` | `k_topics`, `mqtt_manager_init()`, `_on_mqtt_conn` (LED + delegate S15 flush) |
| `src/mqtt_manager_s15.c` | S15 outbound: `event_store` notify → debounce 100 ms → batch ≤8 → publish |
| `src/mqtt_manager_handlers.c` | Inbound B28, B43, S44, S17 |
| `src/mqtt_manager_handler_s43.c` | Inbound S43 (largest handler + file-static 4 KB buffers) |
| `src/mqtt_manager_common.c` | `mqtt_mgr_log_rx`, `mqtt_mgr_publish_ack`, `mqtt_mgr_publish_parse_fail`, `mqtt_mgr_record_event` |

### DEF-C71 topic table

| Topic | Spec | Handler / notes |
|-------|------|-----------------|
| `relay` | B28 | `mqtt_mgr_on_relay_request` → `act_manager_command_relay()` |
| `new_configuration` | B43 | `mqtt_mgr_on_new_configuration` → `act_manager_set_profile()` |
| `current_configuration` | S43 | `mqtt_mgr_on_current_configuration` → actuator getters + `astro_clock` |
| `status` | S44 | `mqtt_mgr_on_status_request` |
| `event` (query) | S17 | `mqtt_mgr_on_event_query_request` → `event_store_query()` |
| `event` (spontaneous) | S15 | No down topic; published from `mqtt_manager_s15.c` |

Topics: `EDP/down|up/<gateway_id>/<suffix>`. QoS from §6 of `docs/DEF-C71-XXX_CONTROLADOR_IP_MQTT.md` (not Annex C).

### Events (S15 / S17 / event_store)

```
Producer (actuator, comm, app boot, B28/B43 handler)
  → event_store_append()     [requires SNTP-valid wall clock]
  → NVS + optional notify_cb
  → mqtt_manager_s15.c queues + debounces
  → mqtt_service_publish_topic(MQTT_TOPIC_EVENT_SPONTANEOUS)

CMS query (S17)
  → mqtt_mgr_on_event_query_request
  → event_store_query()
  → mqtt_payload_build_event_query_response()
```

- `mqtt_mgr_record_event()` in handlers records side effects of MQTT commands (B28/B43).
- Record → payload conversion: `mqtt_payload_event_from_store()` in lib `mqtt_payloads`.
- S15 flush on MQTT reconnect: `mqtt_mgr_s15_on_mqtt_conn(true)` from `_on_mqtt_conn`.

### Where to change what

| Task | File |
|------|------|
| New inbound DEF-C71 command | New handler `.c` or `handlers.c`, wire in `k_topics` |
| S15 batching/debounce/reconnect | `mqtt_manager_s15.c` |
| Shared ack / parse-error logging | `mqtt_manager_common.c` |
| JSON shape only | `components/lib/mqtt_payloads/` |
| Topic strings/QoS | `k_topics` in `mqtt_manager.c` + `mqtt_topics` lib |

---

## Boot order (relevant to MQTT/events)

From `app_manager_init()`:

1. `nvs_driver_init`
2. `event_store_init`
3. `notification_manager_init` → `act_manager_init` → seed profile setters
4. `sntp_service_set_first_sync_cb` (before network)
5. **`mqtt_manager_init`** — must be **before** `mqtt_service_init()` (inside `communication_manager_init`)
6. `communication_manager_init` — Ethernet, SNTP, starts MQTT

On first SNTP sync: `_on_clock_valid` → boot events (`POWER_UP`, etc.) can append to `event_store`.

---

## Related docs & tests

- `docs/architecture.md` — full dependency graph and boot sequence
- `docs/mqtt-onboarding.md` — MQTT RX path narrative (update paths if they still cite old monolithic `mqtt_manager.c`)
- `components/app/managers/mqtt-manager/README.md`
- `components/app/actuator-manager/README.md`
- `tests/regression/mqtt-manager/` — B28/B43/S43/S44 corpus (`run_payloads.py`)
