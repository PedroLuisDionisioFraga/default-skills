---
name: esp-idf-component
description: Use when creating a new ESP-IDF component for the ESP Component Registry (components.espressif.com) or converting an existing ESP-IDF project into a publishable registry component — sets up layout, idf_component.yml manifest, examples, and release/build/format GitHub workflows.
---

# ESP-IDF Registry Component

## Overview

Scaffold or convert a repository so its **root IS an ESP-IDF component** publishable to the ESP Component Registry: `idf_component.yml` + `idf_component_register()` at the root (NO `project()` boilerplate there), sources in `src/` + `include/`, consumer projects under `examples/`, and automated release to the Registry.

An optional target path may be given (e.g. `/esp-idf-component convert C:\path\to\project`); default is the current repo.

## Target layout

```
<repo>/
├── idf_component.yml        # registry manifest (name, version, targets, examples)
├── CMakeLists.txt           # idf_component_register(SRCS "src/..." INCLUDE_DIRS "include")
├── Kconfig                  # only if the component has config options
├── include/<name>.h         # public headers
├── src/<name>.c             # implementation
├── examples/basic/          # standalone IDF project consuming the component
├── docs/publish_to_esp_registry.md
├── .github/workflows/       # release.yml, build.yml, format.yml
├── .clang-format  .gitignore  LICENSE  CHANGELOG.md  README.md
```

## Step 1 — Gather info BEFORE touching files

Derive what you can; ask the user only for the rest.

| Placeholder | Meaning | Default / source |
|---|---|---|
| `{{COMPONENT_NAME}}` | Registry name, lowercase kebab-case (e.g. `cli-api`) | ask; from repo name |
| `{{COMPONENT_NAME_SNAKE}}` | C identifier prefix (e.g. `cli_api`) | derived |
| `{{COMPONENT_NAME_UPPER}}` | Kconfig prefix (e.g. `CLI_API`) | derived |
| `{{COMPONENT_TITLE}}` | Human title (e.g. `CLI API`) | derived |
| `{{DESCRIPTION}}` | One-paragraph manifest description | ask / infer from code |
| `{{NAMESPACE}}` | Registry namespace | `pedroluisdionisiofraga` |
| `{{REPO_URL}}` | GitHub URL, no trailing `/` or `.git` | `git remote get-url origin` |
| `{{VERSION}}` | Initial semver | `0.1.0` (new) / `1.0.0` (mature) |
| `{{IDF_MIN_VERSION}}` | Minimum ESP-IDF | `5.5.0` |
| `{{YEAR}}` / `{{AUTHOR}}` / `{{DATE}}` | LICENSE + CHANGELOG | `git config user.name`, today |

Also confirm: supported `targets` and `tags` (edit the defaults inside the copied manifest).

## Step 2 — Apply templates

Copy from `templates/` in this skill directory, replace ALL placeholders:

| Template | Destination |
|---|---|
| `idf_component.yml` | `idf_component.yml` |
| `CMakeLists.txt` | `CMakeLists.txt` |
| `component.h` / `component.c` | `include/{{COMPONENT_NAME}}.h` / `src/{{COMPONENT_NAME}}.c` (create mode only) |
| `Kconfig` | `Kconfig` (only if the component needs options) |
| `clang-format` / `gitignore` | `.clang-format` / `.gitignore` |
| `LICENSE` / `CHANGELOG.md` / `README.md` | same names at root |
| `publish_to_esp_registry.md` | `docs/publish_to_esp_registry.md` |
| `workflows/*.yml` | `.github/workflows/*.yml` |
| `example-basic/**` | `examples/basic/**` |

`${{ ... }}` in the workflow files is GitHub Actions syntax — leave it untouched; only replace `{{UPPER_CASE}}` placeholders.

### Create mode
All of the above, then fill `src/`/`include/` stubs and make `examples/basic/main/main.c` actually call the API.

### Convert mode (existing project)
1. **Classify code**: library code (the component) vs application code (`main/`, test apps).
2. Move library sources → `src/`, public headers → `include/`. If the lib lives in `components/<x>/`, hoist it to the root. Update `#include`s.
3. **Replace** the root `CMakeLists.txt` (`cmake_minimum_required`/`project()` boilerplate) with the component-register template. Map old `REQUIRES`/`PRIV_REQUIRES` into it; map IDF-managed deps into `idf_component.yml` `dependencies`.
4. Move the old application (`main/`) → `examples/basic/` as a consumer project (example gets the `project()` boilerplate). Project-level `sdkconfig.defaults`, partition tables, `Kconfig.projbuild` MOVE (`git mv`) to the example that needs them — do not delete them; a component-level `Kconfig` stays at root.
5. Apply remaining templates. Merge, don't blindly overwrite, an existing README/LICENSE/.gitignore.
6. **Never delete without confirming**: list leftovers (`sdkconfig`, `build/`, `managed_components/`, `dependencies.lock`) and let `.gitignore` cover them; ask before removing anything tracked by git.

## Registry rules (quick reference)

- Component name: lowercase, digits, `-`/`_` only.
- `version` in the manifest is the single source of truth — the release workflow reads it, tags `v<version>`, creates the GitHub release, uploads to the Registry (skips upload if already published).
- Every dir under `examples/` must build standalone; example manifests use `override_path: "../../../"` so CI builds the **local** code, not the published version.
- Repo must be **public**; secret `IDF_COMPONENT_API_TOKEN` (token from <https://components.espressif.com/settings/tokens>) must exist — remind the user, you cannot do this for them.

**Version bump flow**: bump `version` in `idf_component.yml` + move CHANGELOG entries from `[Unreleased]` → merge to `main` → workflow does the rest.

## Final checks

```bash
grep -rnE '\{\{[A-Z_]+\}\}' .          # no leftover placeholders (GH ${{ ... }} won't match)
python -c "import yaml,glob; [yaml.safe_load(open(f)) for f in glob.glob('**/idf_component.yml', recursive=True)]"
idf.py build                            # inside each examples/<dir>, if IDF is available
```

If `idf.py` is unavailable, say so explicitly — do not claim the example builds.

## Common mistakes

| Mistake | Fix |
|---|---|
| `project()` left in root CMakeLists | Root registers the component; only examples have `project()` |
| Sources left at repo root instead of `src/` | Follow the target layout: `src/` + `include/` |
| Manifest missing `targets:` (implies EVERY chip) or `examples:` list | Both are required; list only targets you support |
| Example depends on published version only, or `path:`-only dep | Use `version` + `override_path` — CI tests PR code, consumers still get the Registry version |
| Upload workflow triggered by manually-created releases | Use the `release.yml` template: bumping `version` in the manifest drives tag + release + upload |
| Old `sdkconfig.defaults`/partition csv deleted during convert | They move into the example, not the trash |
| `build.yml` matrix not matching `examples/` dirs | One `example:` entry per directory |
| Publishing with placeholders left in files | Run the grep final check |
