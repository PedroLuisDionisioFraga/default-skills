#pragma once

/*
 * Skeleton for a component that OWNS a hardware resource.
 *
 * Ownership means: this component's worker task is the only code that writes
 * the hardware. Every other context — periodic timers, network RX, button
 * tasks, other managers — calls the setters below, which publish a request and
 * return. See references/single-owner-io.md for the reasoning.
 *
 * Adapt: rename single_owner_manager -> your component, replace the example
 * device handles, and delete the blocks marked OPTIONAL that you do not need.
 */

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/* The devices this manager owns. Nothing outside this component may call their
 * APIs — that is what "owner" means. Replace with your real device types. */
typedef struct
{
  /* example: led_t indicator; relay_t load; */
  int output_gpio;
} single_owner_manager_t;

/* Example desired-state enum. Keep values in a single aligned word so the
 * field can be volatile without a lock (see zone 2 in the .c). */
typedef enum
{
  SINGLE_OWNER_MODE_OFF = 0,
  SINGLE_OWNER_MODE_ON,
  SINGLE_OWNER_MODE_AUTO,
} single_owner_mode_t;

/*
 * Creates the event group and worker task, then applies the initial state.
 *
 * Call this before initializing any device whose driver task can immediately
 * fire a callback into these setters (buttons and network interfaces are the
 * usual offenders) — the worker must exist before the first request arrives.
 */
esp_err_t single_owner_manager_init(single_owner_manager_t *self);

/*
 * Setters are ASYNCHRONOUS: they validate, publish the request, and return.
 * The worker applies it within a scheduling slice.
 *
 * Argument validation is still synchronous, so the returned esp_err_t is
 * trustworthy for bad input. Only the hardware write is deferred — which means
 * a getter called immediately after a setter may still read the old value.
 */
esp_err_t single_owner_manager_set_mode(single_owner_manager_t *self, single_owner_mode_t mode);

/* ---------------------------------------------------------------------------
 * OPTIONAL: manual command path. Delete if the desired-state setter above is
 * the only way callers influence the output.
 *
 * Exists so a read-modify-write (toggle) is resolved INSIDE the worker. A
 * caller doing set(!get()) from its own task can be preempted between the read
 * and the write, landing on the wrong level. Handing over an intent instead of
 * a computed level keeps that decision where it is atomic.
 * ------------------------------------------------------------------------- */
typedef enum
{
  SINGLE_OWNER_CMD_NONE = 0,
  SINGLE_OWNER_CMD_ON,
  SINGLE_OWNER_CMD_OFF,
  SINGLE_OWNER_CMD_TOGGLE,
} single_owner_cmd_t;

esp_err_t single_owner_manager_command(single_owner_manager_t *self, single_owner_cmd_t cmd);
/* --- end OPTIONAL: manual command path --- */

/*
 * Reports the last level the worker drove — not a pin readback. Safe from any
 * task: the worker is the only writer and it publishes a single aligned word.
 *
 * Because the setters are asynchronous, a get immediately after a set may still
 * return the previous level. Do not build set(!get()) on top of this; hand the
 * intent over with single_owner_manager_command(SINGLE_OWNER_CMD_TOGGLE) so the
 * read-modify-write happens on the owning task.
 */
bool single_owner_manager_is_on(single_owner_manager_t *self);

#ifdef __cplusplus
}
#endif
