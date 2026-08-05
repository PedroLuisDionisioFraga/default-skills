# Library Component Pattern

A **lib** is a pure-C utility component with no hardware dependency — ring
buffers, CRC/checksum functions, config parsers, string utilities, protocol
encoders/decoders, math helpers, etc.

Libs depend only on the C standard library. No ESP-IDF hardware components,
no FreeRTOS (unless the lib is explicitly a concurrency utility).

---

## File layout

```text
components/lib/<name>/
├── include/
│   └── <name>.h
├── src/
│   ├── <name>.c
│   └── <name>_store.c   ← optional: NVS persistence, if the lib has any
├── CMakeLists.txt
└── Kconfig.projbuild    ← if the lib is configurable
```

Sources live under `src/`. When a lib owns data that must survive a reboot, put
the NVS layer in its own `<name>_store.c` with a symmetric
`<name>_store_load()` / `<name>_store_save()` pair — the core stays pure and
testable, the store is the only file that knows about flash. See
`components/lib/time_scheduling/` and `components/lib/astro_clock/`.

---

## Header template

```c
/**
 * @file <name>.h
 * @brief <One-line description of what the library provides>.
 */

#ifndef <NAME>_H
#define <NAME>_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

/* Default shape: caller-owned value struct, fixed capacity, no allocation.
   The caller supplies the storage (static, stack, or inside a manager's private
   context) and the lib only operates on it. */
typedef struct <name>
{
  <entry_t> entries[<NAME>_MAX_ENTRIES];
  size_t    count;
} <name>_t;

/**
 * @brief Validate @p self against the library's invariants.
 *
 * @return true if the object is usable; false otherwise.
 */
bool <name>_validate(const <name>_t *self);

/* Add the library's core operations here — pure functions over @p self. */
bool <name>_evaluate(const <name>_t *self, <input_t> in, <output_t> *out);

#endif  // <NAME>_H
```

For stateless utility functions (no per-instance state), just declare plain
functions:

```c
uint32_t <name>_crc32(const uint8_t *data, size_t len);
int      <name>_encode(const char *in, char *out, size_t out_len);
```

Reach for the **opaque handle** (`typedef struct <name> <name>_t;` with the
definition hidden in the `.c`, plus `create()`/`destroy()`) only when the
instance genuinely needs heap and a variable lifetime — a growable ring buffer,
a parser with dynamic capacity. Every lib in this repo today uses the
caller-owned form: fixed capacity is predictable on a microcontroller, and it
keeps the lib free of allocation-failure paths.

---

## Source template

```c
#include "<name>.h"

#include <string.h>

/* Pure functions over the caller's object — no file-static mutable state, so
   the lib is reentrant and unit-testable on the host. */

bool <name>_validate(const <name>_t *self)
{
  if (self == NULL || self->count > <NAME>_MAX_ENTRIES)
    return false;

  /* check the library's invariants */
  return true;
}

bool <name>_evaluate(const <name>_t *self, <input_t> in, <output_t> *out)
{
  if (!<name>_validate(self) || out == NULL)
    return false;

  /* core logic */
  return true;
}
```

The optional `src/<name>_store.c` is the only file that touches NVS:

```c
#include "<name>.h"
#include "nvs_driver.h"

#define KEY_<NAME> "<name>"   /* NVS keys are <= 15 chars */

esp_err_t <name>_store_load(<name>_t *out);
esp_err_t <name>_store_save(const <name>_t *self);
```

It serializes the whole object under one key, so a load always fully
repopulates it and a corrupt value falls back to an empty object rather than
failing. `time_scheduling_store.c` is the reference implementation.

---

## CMakeLists.txt

```cmake
idf_component_register(SRCS "src/<name>.c"
                    INCLUDE_DIRS "include"
                    REQUIRES "")   # no ESP-IDF hardware deps
```

With a store, add it and its one dependency:

```cmake
idf_component_register(SRCS "src/<name>.c" "src/<name>_store.c"
                    INCLUDE_DIRS "include"
                    REQUIRES nvs)
```

If the lib is header-only, omit the `SRCS` line:

```cmake
idf_component_register(INCLUDE_DIRS "include")
```

---

## Rules

1. **No hardware includes** — never `#include <driver/gpio.h>` or any ESP-IDF hardware header in a lib. If you need hardware, you have a device, not a lib.
2. **Caller-owned storage by default** — fixed-capacity value structs, no `malloc`. The opaque handle is the exception, for genuinely dynamic lifetimes.
3. **NULL-safe everywhere** — every public function validates its pointers and returns `false` / an error code rather than faulting. A `destroy()`, if you have one, accepts NULL.
4. **No logging** — libs are silent. If error signaling is needed, return a bool or an error code; let the caller log. (`system_time` predates this rule and still logs; do not copy it.)
5. **No global mutable state** — libs must be reentrant. Pass context explicitly.
6. **The lib does not decide policy** — it evaluates, validates, serializes. Choosing *when* to act, and arbitrating between callers, belongs to the manager above it. `time_scheduling` has no notion of an operation mode; `actuator_manager` dispatches to it.

---

## Checklist

- [ ] Header guard: `<NAME>_H`
- [ ] Sources under `src/`; NVS persistence split into `src/<name>_store.c`
- [ ] No ESP-IDF hardware includes in header or source
- [ ] Caller-owned fixed-capacity struct (or a justified opaque handle)
- [ ] Every public function NULL-checks its pointers
- [ ] Persistence named `<name>_store_load` / `<name>_store_save`, one NVS key per object
- [ ] No `ESP_LOGx` calls — libs are silent
- [ ] No global mutable state
- [ ] No policy decisions — the manager above chooses when to call the lib
- [ ] CMakeLists `REQUIRES` is empty or contains only standard/portable deps
