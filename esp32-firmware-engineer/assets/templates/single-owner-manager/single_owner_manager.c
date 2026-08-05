#include "single_owner_manager.h"

#include <string.h>

#include "driver/gpio.h"
#include "esp_check.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

static const char *TAG = "single_owner";

/* Size the stack from the worker's real locals — config snapshots are often
 * hundreds of bytes — plus headroom for ESP_LOGx's vprintf. Confirm with
 * uxTaskGetStackHighWaterMark() during bring-up rather than guessing twice. */
#define MGR_TASK_STACK 4096
#define MGR_TASK_PRIO  3

/* 0 makes the worker purely reactive (portMAX_DELAY). A non-zero value turns
 * the wait timeout into the periodic housekeeping tick, which is usually
 * better than a software timer: the heavy work stays off the shared FreeRTOS
 * timer daemon, whose stack is small and shared with the rest of ESP-IDF.
 * Note the period then restarts after each pass instead of being fixed. */
#define MGR_TICK_MS 0

#define EVT_APPLY   ((EventBits_t)(1 << 0)) /* recompute and apply desired state */
#define EVT_COMMAND ((EventBits_t)(1 << 1)) /* OPTIONAL: a manual command is pending */

/* The concrete thing the worker writes to hardware. Keep it comparable so the
 * applied cache (zone 4) can short-circuit unchanged passes. */
typedef struct
{
  bool on;
  uint32_t arg; /* e.g. blink period, duty, brightness */
} mgr_output_t;

/* OPTIONAL (zone 3): multi-word configuration this manager mirrors. Replace
 * with your real config, or delete this type together with zone 3 below. */
typedef struct
{
  uint32_t threshold;
  uint32_t window_ms;
} mgr_config_t;

typedef struct
{
  /* --- Zone 1: handles. Written once in init, read-only afterwards. --- */
  single_owner_manager_t *m;
  EventGroupHandle_t events;
  TaskHandle_t worker;

  /* --- Zone 2: desired state. Written by setters on any task, read by the
   * worker. volatile and single aligned words only: those cannot tear, so no
   * lock is needed, and the event bit is what signals the change. --- */
  volatile single_owner_mode_t desired_mode;

  /* OPTIONAL: payload of the pending manual command. Never cleared, only
   * overwritten — EVT_COMMAND is what signals it, and a request landing
   * mid-apply leaves the bit set so the next pass picks up the newer value.
   * Last writer wins, which is the right semantic for an output level. */
  volatile single_owner_cmd_t pending_cmd;

  /* --- Zone 3 (OPTIONAL): multi-word config. Structs tear when a writer
   * replaces one while the worker copies it, so this zone needs a mutex.
   * Serializing the writes did NOT remove that need. The lock is taken only
   * for the copy and released before any hardware I/O. Delete this zone if
   * every shared field fits in zone 2. --- */
  SemaphoreHandle_t cfg_mutex;
  mgr_config_t cfg;

  /* --- Zone 4: applied cache. Worker-owned, so it needs neither volatile nor
   * a lock. Comparing against it is what stops an unrelated re-apply from
   * restarting a running blink and keeps transition logs honest. --- */
  mgr_output_t applied;

  /* Readable projection of zone 4 for getters called on other tasks. The worker
   * is still the only writer, but this is a single aligned word rather than a
   * multi-field struct, so a reader cannot catch it half-updated. Do NOT let
   * getters read `applied` directly: that struct is worker-private for a
   * reason. Publish it AFTER the hardware write, so it never claims a level
   * that is not driven yet. */
  volatile bool published_on;
} mgr_priv_t;

static mgr_priv_t s_priv; /* zero-init; program lifetime */

/* ---------------------------------------------------------------------------
 * Hardware boundary.
 *
 * INVARIANT: _drive_output() runs only on _worker_task. Every other context
 * requests through _request_apply() / _request_command() and returns. That is
 * what makes the read-decide-write sequence in _apply() safe without holding a
 * lock across I/O — nothing else can move the output between the sample and
 * the write. The single exception is the safe-state write in init, which runs
 * before the worker can be asked for anything.
 *
 * In a real project this calls your device component (led_on(), relay_off(),
 * motor_set_duty()) rather than the GPIO driver. The direct gpio_set_level()
 * here only keeps the template compilable on its own.
 * ------------------------------------------------------------------------- */
static void _drive_output(const mgr_output_t *out)
{
  gpio_set_level(s_priv.m->output_gpio, out->on ? 1 : 0);
}

static bool _output_eq(mgr_output_t a, mgr_output_t b)
{
  return a.on == b.on && a.arg == b.arg;
}

/* Turns desired state into a concrete output. Pure: reads zones 2 and 3, never
 * touches hardware. Keeping it separate is what makes the manager testable and
 * keeps _apply() short enough to audit. */
static mgr_output_t _resolve_output(void)
{
  /* OPTIONAL (zone 3): snapshot the multi-word config under the lock, then
   * release before doing anything else. Never hold this across _drive_output(). */
  mgr_config_t cfg;
  xSemaphoreTake(s_priv.cfg_mutex, portMAX_DELAY);
  cfg = s_priv.cfg;
  xSemaphoreGive(s_priv.cfg_mutex);

  mgr_output_t out = {.on = false, .arg = cfg.window_ms};

  switch (s_priv.desired_mode)
  {
    case SINGLE_OWNER_MODE_ON:
      out.on = true;
      break;
    case SINGLE_OWNER_MODE_AUTO:
      out.on = (cfg.threshold > 0); /* replace with the real decision */
      break;
    case SINGLE_OWNER_MODE_OFF:
    default:
      out.on = false;
      break;
  }
  return out;
}

/* The scheduled path: recompute, and touch hardware only on a real change. */
static void _apply(void)
{
  mgr_output_t want = _resolve_output();
  if (_output_eq(want, s_priv.applied))
    return;

  _drive_output(&want);
  s_priv.applied = want;
  s_priv.published_on = want.on;
  ESP_LOGI(TAG, "output -> %s", want.on ? "ON" : "OFF");
}

/* ---------------------------------------------------------------------------
 * OPTIONAL: manual command path. Delete along with EVT_COMMAND, pending_cmd,
 * and single_owner_manager_command() if the mode setter is the only input.
 * ------------------------------------------------------------------------- */
static void _apply_command(void)
{
  mgr_output_t want = s_priv.applied;

  switch (s_priv.pending_cmd)
  {
    case SINGLE_OWNER_CMD_ON:
      want.on = true;
      break;
    case SINGLE_OWNER_CMD_OFF:
      want.on = false;
      break;
    case SINGLE_OWNER_CMD_TOGGLE:
      /* The read-modify-write lives here, on the owning task, precisely so it
       * cannot interleave with _apply() or with another command. A caller
       * doing set(!get()) on its own task would be racing. */
      want.on = !s_priv.applied.on;
      break;
    case SINGLE_OWNER_CMD_NONE:
    default:
      return;
  }

  _drive_output(&want);
  s_priv.applied = want;
  s_priv.published_on = want.on;
  ESP_LOGI(TAG, "manual -> %s", want.on ? "ON" : "OFF");
}
/* --- end OPTIONAL: manual command path --- */

/* ---------------------------------------------------------------------------
 * Request helpers — what every non-owning context calls.
 * ------------------------------------------------------------------------- */
static void _request_apply(void)
{
  xEventGroupSetBits(s_priv.events, EVT_APPLY);
}

/* OPTIONAL: publish the payload BEFORE setting the bit, so a set bit always
 * refers to a value the worker can already see. */
static void _request_command(single_owner_cmd_t cmd)
{
  s_priv.pending_cmd = cmd;
  xEventGroupSetBits(s_priv.events, EVT_COMMAND);
}

/* ---------------------------------------------------------------------------
 * The owner.
 * ------------------------------------------------------------------------- */
static void _worker_task(void *arg)
{
  (void)arg;
  for (;;)
  {
    EventBits_t bits = xEventGroupWaitBits(s_priv.events,
                                           EVT_APPLY | EVT_COMMAND,
                                           pdTRUE,  /* clear on exit */
                                           pdFALSE, /* any bit will do */
                                           MGR_TICK_MS ? pdMS_TO_TICKS(MGR_TICK_MS) : portMAX_DELAY);

    /* OPTIONAL: a manual command is applied but does not itself trigger a
     * recompute — otherwise the scheduled state would immediately undo it.
     * Decide deliberately which one wins in your product. */
    if (bits & EVT_COMMAND)
      _apply_command();

    bool timed_out = ((bits & (EVT_APPLY | EVT_COMMAND)) == 0);
    if (timed_out || (bits & EVT_APPLY))
      _apply();
  }
}

/* ---------------------------------------------------------------------------
 * Public API.
 * ------------------------------------------------------------------------- */
esp_err_t single_owner_manager_init(single_owner_manager_t *self)
{
  ESP_RETURN_ON_FALSE(self != NULL, ESP_ERR_INVALID_ARG, TAG, "self is NULL");

  s_priv.m = self;
  s_priv.desired_mode = SINGLE_OWNER_MODE_OFF;
  s_priv.pending_cmd = SINGLE_OWNER_CMD_NONE;

  gpio_config_t io = {
    .pin_bit_mask = 1ULL << self->output_gpio,
    .mode = GPIO_MODE_OUTPUT,
    .pull_up_en = GPIO_PULLUP_DISABLE,
    .pull_down_en = GPIO_PULLDOWN_DISABLE,
    .intr_type = GPIO_INTR_DISABLE,
  };
  ESP_RETURN_ON_ERROR(gpio_config(&io), TAG, "gpio_config failed");

  /* OPTIONAL (zone 3) */
  s_priv.cfg_mutex = xSemaphoreCreateMutex();
  ESP_RETURN_ON_FALSE(s_priv.cfg_mutex != NULL, ESP_ERR_NO_MEM, TAG, "xSemaphoreCreateMutex failed");
  memset(&s_priv.cfg, 0, sizeof(s_priv.cfg));

  s_priv.events = xEventGroupCreate();
  ESP_RETURN_ON_FALSE(s_priv.events != NULL, ESP_ERR_NO_MEM, TAG, "xEventGroupCreate failed");

  /* Seed zone 4 with a value _resolve_output() can never produce, so the first
   * pass writes real hardware state instead of trusting the reset default. */
  s_priv.applied = (mgr_output_t){.on = true, .arg = UINT32_MAX};

  /* The worker blocks on its first wait with no bit set, so creating it here
   * applies nothing yet — it only makes the request path usable before any
   * device below can call back into us. */
  if (xTaskCreate(_worker_task, "single_owner", MGR_TASK_STACK, NULL, MGR_TASK_PRIO, &s_priv.worker) != pdPASS)
  {
    ESP_LOGE(TAG, "xTaskCreate(single_owner) failed");
    return ESP_ERR_NO_MEM;
  }

  /* Known safe level. The one hardware write outside the worker: it runs
   * before the first request, so nothing can race it. Publish it too, so a
   * getter called before the worker's first pass reports the driven level
   * rather than a stale default. */
  gpio_set_level(self->output_gpio, 0);
  s_priv.published_on = false;

  /* Initialize devices that can fire callbacks only AFTER the worker exists:
   *   ESP_RETURN_ON_ERROR(button_init(&self->button), TAG, "button init failed");
   */

  _request_apply();
  ESP_LOGI(TAG, "init: gpio=%d", self->output_gpio);
  return ESP_OK;
}

esp_err_t single_owner_manager_set_mode(single_owner_manager_t *self, single_owner_mode_t mode)
{
  ESP_RETURN_ON_FALSE(self != NULL, ESP_ERR_INVALID_ARG, TAG, "self is NULL");
  ESP_RETURN_ON_FALSE(mode <= SINGLE_OWNER_MODE_AUTO, ESP_ERR_INVALID_ARG, TAG, "invalid mode %d", (int)mode);

  /* Validation is synchronous, so this return value is trustworthy. Only the
   * hardware apply is deferred to the worker. */
  s_priv.desired_mode = mode;
  _request_apply();
  return ESP_OK;
}

/* OPTIONAL: manual command path. */
esp_err_t single_owner_manager_command(single_owner_manager_t *self, single_owner_cmd_t cmd)
{
  ESP_RETURN_ON_FALSE(self != NULL, ESP_ERR_INVALID_ARG, TAG, "self is NULL");
  ESP_RETURN_ON_FALSE(cmd != SINGLE_OWNER_CMD_NONE, ESP_ERR_INVALID_ARG, TAG, "no command");

  _request_command(cmd);
  return ESP_OK;
}

bool single_owner_manager_is_on(single_owner_manager_t *self)
{
  if (self == NULL)
    return false;

  /* Reports the last level the worker drove, not a pin readback. Two reasons,
   * and the first one is a correctness bug waiting to happen:
   *
   *   1. The pin is configured GPIO_MODE_OUTPUT, which leaves the input buffer
   *      disabled — gpio_get_level() then returns 0 no matter what is being
   *      driven. Reading back the pin requires GPIO_MODE_INPUT_OUTPUT, which
   *      costs you the ability to detect a shorted/loaded output anyway.
   *   2. Even with readback enabled, the hardware is the owner's business. A
   *      getter that samples it invites callers to build set(!get()) on top,
   *      which is the read-modify-write this whole pattern exists to prevent —
   *      use single_owner_manager_command(SINGLE_OWNER_CMD_TOGGLE) instead.
   */
  return s_priv.published_on;
}

/*
 * No deinit / vTaskDelete on purpose: managers usually live for the whole
 * program, and killing the worker mid-_apply() leaves hardware in an undefined
 * state. If you genuinely need to stop, add an EVT_STOP bit, break out of the
 * loop, drive the safe level, and let the task delete itself with
 * vTaskDelete(NULL) — never from another context.
 */
