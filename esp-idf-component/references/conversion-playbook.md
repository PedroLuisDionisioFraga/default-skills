# Conversion Playbook: Existing ESP-IDF Project → Registry Component

Detailed steps and edge cases for `convert` mode. Summary lives in SKILL.md; this is the full procedure.

## Order of operations (important)

Move real project files FIRST, run `scripts/apply_templates.py --mode convert` SECOND — the script skips existing destinations, so moved files are never clobbered.

## Step-by-step

1. **Classify the code.** Library code = the reusable part (usually `components/<x>/` or a lib dir). Application code = `main/` and anything that calls the library. If unsure which part is "the component", ask the user — this defines the public API.
2. **Hoist the library** (`git mv`, never copy+delete):
   - implementation `.c/.cpp` → `src/`
   - public headers → `include/` (headers used only internally stay in `src/`)
   - update `#include` paths in moved files if they referenced old relative locations.
3. **Replace the root CMakeLists.txt.** The old one has `cmake_minimum_required` + `project()` boilerplate; the new one is a single `idf_component_register()`:
   - `SRCS`: the files now in `src/` (or `SRC_DIRS "src"`).
   - Carry over `REQUIRES`/`PRIV_REQUIRES` from the old library CMakeLists — IDF built-ins stay here, external components also go to the manifest (see references/manifest-reference.md).
4. **Turn the old app into the example.** `git mv main examples/basic/main`, give `examples/basic/` the `project()` boilerplate (template `example-basic/CMakeLists.txt`). In `examples/basic/main/CMakeLists.txt` drop any `REQUIRES <component>` — the dependency comes from `main/idf_component.yml` (template with `override_path`).
5. **Relocate project config:**
   - `sdkconfig.defaults`, partition `.csv`, `Kconfig.projbuild` → `git mv` into the example that needs them (update `CONFIG_PARTITION_TABLE_CUSTOM_FILENAME` paths if any).
   - Component-level `Kconfig` (options of the library itself) stays at repo root.
   - pytest files (`pytest_*.py`) → move next to the example they exercise.
6. **Run the script** for everything else: manifest, workflows, docs, hygiene files. It reports `skipped` for files you already placed.
7. **Merge, don't overwrite,** existing README/LICENSE/.gitignore: keep the user's content, add the missing pieces (badge + install section in README; template entries into .gitignore). The script never overwrites them; do the merge by editing.
8. **Leftovers.** `sdkconfig`, `sdkconfig.old`, `build/`, `managed_components/`, `dependencies.lock` are covered by `.gitignore`. **Never `git rm` a tracked file without listing it and getting confirmation.**

## Edge cases

| Situation | Handling |
|---|---|
| Multiple libs under `components/` | Ask which ONE is the component. Others become manifest dependencies (if published), move into the example (if app-support code), or the user splits repos. One repo = one Registry component. |
| Library already at root (`src/`-less, files loose) | Same steps; just `git mv` into `src/`/`include/`. |
| C++ sources | Fine: list `.cpp` in `SRCS`; keep `extern "C"` guards in public headers (templates already have them). |
| Old root CMakeLists had `EXTRA_COMPONENT_DIRS` | Those dirs are app-side; they move with the example. |
| App had multiple build configs (`sdkconfig.ci`, etc.) | Move all next to the example; CI uses defaults unless told otherwise. |
| Existing GitHub workflows | Don't delete; add the three templates alongside and point out overlaps to the user. |
| No git repo / dirty tree | Stop and ask: conversion relies on `git mv` history and a clean tree to be reviewable. |
