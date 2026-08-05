# Single-Owner I/O (ESP-IDF)

Use this reference when writing or reviewing any component that drives hardware — a relay, LED, motor, valve, display backlight, heater, or any actuator whose state several parts of the firmware care about.

The problem it solves: firmware grows writers. A relay that only a timer touched at v1 gets a button gesture at v2 and a network command at v3. Each writer looks correct in isolation. Together they corrupt the output in ways that are rare, unreproducible, and only show up in the field.

## 1. Name the owner

Before writing a line of code, answer one question: **which single component owns this hardware resource?**

Every peripheral gets exactly one owner. That owner holds a worker task, and that task is the only code that writes the hardware. Everything else asks.

```
external contexts (periodic timer, network RX, button task, public setters, event callbacks)
       │  request only: publish desired state, then set an event bit
       ▼
[OWNER]   worker task — the only writer
       │  calls the device's imperative API
       ▼
[DEVICE]  led_on() / relay_off() / motor_set_duty()
       ▼
    hardware
```

Where the component you are writing sits determines its shape:

| Position | Shape |
|---|---|
| **Is** the owner | Worker task + event bits + request helpers |
| **Below** the owner | Imperative API (`led_on()`, `relay_off()`). Does not arbitrate. |
| **Above** the owner | Calls the owner's setter. Never touches hardware. |

This is a structural rule, not a headcount. Do not wait until a second writer appears to introduce the owner — by then the second writer is usually a network path that arrives under deadline, and the refactor competes with a feature. Naming the owner at design time costs one sentence in the header comment.

## 2. Arbitrating is not the same as executing

A device below the owner may legitimately have its own timer or task. The distinction that keeps this honest:

- **Executing** — carrying out *one already-decided command* over time. A blink timer toggling a GPIO, a debounce task sampling a pin 50 ms after an edge, a timer ramping PWM toward a target. Fine at any layer.
- **Arbitrating** — choosing between concurrent requests, or performing `read state → decide → write`. Belongs to the owner alone.

A blink timer inside an LED device is executing: it received one command from its single owner and spreads it over time. Nothing else is asking that device for anything, so there is nothing to arbitrate.

The moment a component has to decide *which* of two callers wins, it is arbitrating — and it needs to be the owner, or hand the decision to one.

## 3. The owner's anatomy: four ownership zones

What makes this pattern readable is not the task — it is that every field declares who may touch it. Organize the private struct in zones and say so in comments:

```c
typedef struct
{
  /* --- Zone 1: handles. Written once in init, read-only after. No guard. --- */
  my_manager_t      *m;        /* back-pointer to the owned devices */
  EventGroupHandle_t events;
  TaskHandle_t       worker;

  /* --- Zone 2: desired state. Written by setters on any task, read by the
   * worker. volatile, single aligned words only: those cannot tear, and the
   * event bit is what signals a change. --- */
  volatile bool      desired_on;
  volatile mode_t    desired_mode;

  /* --- Zone 3: multi-word config. Structs tear; this zone needs a mutex.
   * The lock is released before any hardware I/O. Omit if you have none. --- */
  SemaphoreHandle_t  cfg_mutex;
  my_config_t        cfg;

  /* --- Zone 4: applied cache. Worker-owned. Needs neither volatile nor a
   * lock, because exactly one task reads and writes it. --- */
  my_output_t        applied;
} my_manager_priv_t;

static my_manager_priv_t s_priv; /* zero-init; program lifetime */
```

Zone 4 is what stops an unrelated re-render from restarting a running blink, and what keeps the transition log honest. Compare the resolved output against it and return early when nothing changed.

### The four functions

```c
/* Requesters publish the payload BEFORE setting the bit, so a set bit always
 * refers to a value that is already visible. */
static void _request_apply(void)
{
  xEventGroupSetBits(s_priv.events, EVT_APPLY);
}

/* The only place hardware is written. */
static void _apply(void)
{
  my_output_t want = _resolve_output();   /* desired state -> concrete output */
  if (_output_eq(want, s_priv.applied))
    return;

  led_on(&s_priv.m->led);                 /* device API, not gpio_set_level() */
  s_priv.applied = want;
}

/* The owner. A finite timeout doubles as the periodic housekeeping tick;
 * portMAX_DELAY makes the manager purely reactive. */
static void _worker_task(void *arg)
{
  (void)arg;
  for (;;)
  {
    xEventGroupWaitBits(s_priv.events, EVT_APPLY,
                        pdTRUE,   /* clear on exit */
                        pdFALSE,  /* any bit will do */
                        MGR_TICK_MS ? pdMS_TO_TICKS(MGR_TICK_MS) : portMAX_DELAY);
    _apply();
  }
}
```

Public setters stay thin: validate, publish to zone 2 or 3, request, return.

## 4. Choosing the signalling mechanism

| Mechanism | Use when | Why |
|---|---|---|
| Event group | the request is "recompute" or a level | setting the same bit N times collapses into one pass; nothing to overflow |
| Queue | each request carries distinct payload that must not be lost | preserves order and content, at the cost of depth and overflow handling |
| Mutex | shared multi-word state (config snapshots) | prevents torn structs — never held across hardware I/O |

Event groups coalesce for free, which is exactly right when the worker recomputes the full desired output every pass: ten requests during one pass need one more pass, not ten. Reach for a queue when losing an intermediate request would lose information — a command log, a sequence of distinct movements.

Serializing the writes does **not** remove the need for a mutex on multi-word state. The worker still copies a config struct while a setter may be replacing it; that copy tears without a lock. Take the lock, copy to a local, release, then do the I/O against the local.

## 5. The failure modes this prevents

These are the reasons the pattern is worth a task's stack. Understanding them is what lets you recognize the problem in code that does not look like this.

**Check-then-act across a blocking call.** A worker samples the hardware state, logs a few lines, then decides based on the sample. `ESP_LOGx` writes to the UART and blocks when the TX buffer fills — a guaranteed context switch. By the time the decision executes, the sample is stale. FreeRTOS can preempt between any two instructions, so shortening the gap does not close it.

**Non-idempotent read-modify-write from a non-owner.** `set(!get())` — a toggle — is the classic. Two contexts toggling concurrently can land on the same level instead of opposite ones, and a toggle interleaved with an absolute write silently inverts. Writing an absolute level (`set(true)`) is idempotent and degrades gracefully; a toggle does not. Keep every read-modify-write inside the owner.

**A lock held across hardware I/O.** The tempting fix for the first two is to widen an existing mutex over the whole sequence. That puts driver calls, and often another component's state, under your lock: priority inversion when a low-priority holder blocks a high-priority reader, and deadlock if the called component ever calls back. The mutex guards *data*; interleaved *sequences* need serialization instead.

**Narrowing the window instead of closing it.** Removing the logs between sample and act makes the race rare rather than absent. That is worse than leaving it: it stops reproducing on the bench and starts appearing monthly in the field.

## 6. Trade-offs to state honestly

Do not present this pattern as free. When you apply it, tell the user:

- **Setters become asynchronous.** They return once the request is posted; the worker applies within a scheduling slice. A getter called immediately after a setter may still observe the previous value.
- **Validation and persistence stay synchronous.** Argument checks and NVS writes still run on the caller's task, so the returned `esp_err_t` remains trustworthy — only the hardware apply is deferred. Say this explicitly, because "asynchronous" otherwise reads as "errors are lost".
- **One task's stack.** Size it from the worker's real locals (config snapshots are often hundreds of bytes) plus `vprintf` headroom, then confirm with `uxTaskGetStackHighWaterMark()`.
- **Periodic phase shifts** if the wait timeout replaces a software timer: the period restarts after each pass rather than being fixed, and an on-demand request resets it.

## 7. Init order

The worker and its event group must exist before anything that can request. In practice that means creating them before initializing any device whose driver task can fire a callback immediately (buttons and network interfaces are the usual offenders), and before the first request of your own.

Seed zone 4 with a value the resolver can never produce, so the first pass always writes real hardware state instead of assuming the reset default.

A direct hardware write during init — driving an output to a known safe level — is fine and worth a comment saying why: it runs before the first request, so nothing can race it.

## Worked examples

Two implementations in this repository converged on this shape independently. Read them when the abstract version is not enough:

- **`components/app/managers/notification/notification_manager.c`** — the simpler variant. Four LEDs, several producer subsystems, bits only and no payload. Shows the zone comments almost verbatim, `_out_eq`/`_apply` change detection per LED, the impossible-value seed, and a purely reactive `portMAX_DELAY` worker.

- **`components/app/actuator-manager/actuator_manager.c`** — the full variant. Adds a payload field for schedule-bypassing manual commands (including a `TOGGLE` resolved inside the worker), a finite wait timeout that replaced a software timer, a mutex coexisting for the multi-word schedule snapshot, and an explicit INVARIANT comment above `_apply_relay()` naming who may call it. `docs/state-machine-time-table.md` traces the same failure modes against real line numbers, including why the stale sample in the check-then-act is safe once the invariant holds.

## Review checklist

- Can more than one context write this peripheral? Name the owner; if there is none, that is the finding.
- Does any `read → decide → write` sequence span a blocking call (logging, NVS, network, mutex acquisition)?
- Is there a toggle or other read-modify-write outside the owner?
- Is a lock held across a driver call or across another component's API?
- Do the desired-state fields shared across tasks fit in a single aligned word? Multi-word ones need a mutex, or they tear.
- Does the worker compare against an applied cache, or does it rewrite hardware every pass?
- Are the worker and its event group created before any context that can request?
- Are asynchronous setters documented as such, including which errors are still reported synchronously?
