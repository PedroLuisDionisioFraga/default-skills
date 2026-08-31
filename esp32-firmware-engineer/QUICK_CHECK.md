# ESP32 Firmware Engineer Skill: One-Page Quick-Check (New ESP-IDF Project)

Use this before doing implementation, review, debugging, or bring-up work with this skill.

## 1) Project Identity

- [ ] Confirm exact target variant: `esp32`, `esp32s3`, `esp32c3`, `esp32c6`, etc.
- [ ] Confirm board name/revision and flash size.
- [ ] Confirm ESP-IDF version in use.
- [ ] Confirm intended behavior and acceptance criteria.

## 2) Hardware Context (Blocker If Missing)

- [ ] Capture pin map for all in-scope peripherals.
- [ ] Capture electrical constraints (voltage levels, pull-ups, transceivers, strapping pins).
- [ ] List connected devices (sensors/displays/radios) and interfaces (I2C/SPI/UART/TWAI/etc.).
- [ ] Confirm available debug transport (USB CDC, USB-Serial-JTAG, external USB-UART).

## 3) Configuration and Layout

- [ ] Review `sdkconfig` / `sdkconfig.defaults` constraints relevant to the task.
- [ ] Review partition table CSV and confirm flash usage strategy.
- [ ] If OTA is required, confirm OTA layout, rollback behavior, and storage requirements.
- [ ] Confirm PSRAM availability/mode if memory/perf is in scope.

## 4) Dependency and Compatibility Proof

- [ ] List all external frameworks/components with exact versions (ESP-ADF, ESP-SR, LVGL, managed components, etc.).
- [ ] Produce concrete compatibility evidence for the full stack (matrix/manifest/release notes/lock file).
- [ ] Run `scripts/check_plugin_compatibility.py` if available.

## 5) Toolchain Readiness

- [ ] Verify ESP-IDF environment is actually active.
- [ ] Verify `idf.py --version` succeeds.
- [ ] Use `idf.py` directly from the project root — see the note below.

> **This project (`fw_sbc-wxxx`) does not use shell wrappers.** `scripts/` holds
> only `sync-submodules.bat`. The `scripts/*.sh` shipped inside this skill assume
> they were copied to `<project>/scripts/` and derive `PROJECT_DIR` from their own
> location, so running them from `.claude/skills/` points `idf.py` at a directory
> with no `CMakeLists.txt`. They also detect serial ports via `/dev/*` globs only,
> which never matches on Windows (`COM*`). Do not invoke them here — call `idf.py`
> from the project root instead.

## 6) Execution Plan by Task Type

- [ ] Classify task: `write`, `review`, `debug`, or `bring-up`.
- [ ] Read minimum relevant files first (`main`, target component(s), headers, `CMakeLists.txt`, `sdkconfig`, partition CSV, logs).
- [ ] State assumptions explicitly when any context is incomplete.

## 7) Implementation/Review Safety Checks

- [ ] Keep changes small and ESP-IDF idiomatic.
- [ ] Treat ISR safety, FreeRTOS context correctness, and watchdog behavior as first-class.
- [ ] Name the owning component for every hardware output; confirm its worker task is the only writer and that no `read -> decide -> write` sequence spans a blocking call (`references/single-owner-io.md`).
- [ ] For app-manager changes in this repo, confirm the edit lands in the correct split file (`references/fw-sbc-wxxx-app-architecture.md`).
- [ ] Propagate/check `esp_err_t` and add actionable logs (`ESP_LOGx` tags + error context).
- [ ] Verify resource lifecycle (init/deinit, handlers, semaphores, sockets/NVS/driver handles).
- [ ] Validate pin/peripheral conflicts, timing/clock assumptions, and bus ownership.

## 8) Build, Flash, Monitor Baseline

Run from the project root, in a shell where ESP-IDF is exported:

```bash
idf.py --version
idf.py build
idf.py -p COM<N> flash
idf.py -p COM<N> monitor
```

`idf.py fullclean build` only when a failure looks stale or config-related — a
full rebuild of this project is slow and rarely the actual fix.

Project specifics:

- Target is `esp32c6` (`CONFIG_IDF_TARGET` in `sdkconfig`); do not run `set-target`.
- Ports are `COM<N>` on Windows, not `/dev/tty*`.
- `CMakeLists.txt` sets `MINIMAL_BUILD ON`, so only `main` and its dependency
  closure compile. A component nothing depends on will not be built — and its
  errors will not surface.
- Passwords live in the gitignored `sdkconfig.defaults.local`; a fresh clone
  builds without it but connects to nothing.

- [ ] Capture serial port and baud assumptions.
- [ ] If failure appears stale/config-related, rerun clean build and document why.

## 9) Debugging Checklist (When Applicable)

- [ ] Separate failure stage: build-time, flash-time, boot-time, runtime.
- [ ] For panic/reset: capture reset reason, panic output, and preceding logs.
- [ ] Prefer targeted instrumentation (logs/counters/asserts) before subsystem rewrites.
- [ ] If fault is not localizable, request a minimal repro or known-good reference snippet.

## 10) Completion Criteria

- [ ] Build passes using project workflow.
- [ ] Task-specific validation completed (logs/tests/flash/monitor checks as applicable).
- [ ] Document what was verified in hardware and what remains unverified.
- [ ] Report results in required format:
  - Implementation: change summary -> key decisions -> validation.
  - Review: findings by severity with file/line refs -> open questions/assumptions.
  - Debugging: likely causes -> evidence -> next diagnostic step -> proposed fix.

## Helpful Skill Assets

- Skill definition: `SKILL.md`
- References: `references/` (start with `fw-sbc-wxxx-app-architecture.md` for actuator/mqtt work in this repo)
- Templates: `assets/templates/`
