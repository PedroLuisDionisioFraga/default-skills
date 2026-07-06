# idf_component.yml Reference

The manifest at the repo root defines the component on the ESP Component Registry. Official schema: `compote manifest schema`.

## Fields

| Field | Rules |
|---|---|
| `name` | Lowercase letters, digits, `-`/`_`. Must match the Registry component name. Consumers install `<namespace>/<name>`. |
| `version` | Semver `MAJOR.MINOR.PATCH` (pre-release suffixes allowed, e.g. `1.2.0-rc1`). Single source of truth: the release workflow reads it, tags `v<version>`, uploads. Registry rejects re-uploading an existing version. |
| `description` | One paragraph shown in search results. No TODO stubs. |
| `url` / `repository` / `documentation` / `issues` | Links shown on the component page. `repository` without trailing `.git`. |
| `license` | SPDX id (e.g. `MIT`). Must match the LICENSE file. |
| `targets` | List of supported chips (`esp32`, `esp32s2`, `esp32s3`, `esp32c3`, `esp32c5`, `esp32c6`, `esp32h2`, `esp32p4`). **Omitting it means "all targets"** — list only what you support/test. |
| `tags` | Search keywords on components.espressif.com. |
| `examples` | `- path: examples/basic` entries. Registry packs each as a named example usable with `idf.py create-project-from-example "<ns>/<name>:basic"`. Paths must exist. |
| `dependencies` | See below. `idf: ">=X.Y.Z"` pins the minimum ESP-IDF. |

## Dependency syntaxes (when to use each)

```yaml
dependencies:
  idf: ">=5.5.0"

  # 1. Registry range — normal consumption of published components
  espressif/led_strip: "^2.5.0"

  # 2. version + override_path — ONLY inside this repo's examples:
  #    consumers copying the example resolve the published version,
  #    while local/CI builds use the repo root code (tests PR code).
  pedroluisdionisiofraga/my-comp:
    version: "^1.0.0"
    override_path: "../../../"

  # 3. git / path — unpublished dependencies; avoid in published
  #    components (Registry consumers can't resolve local paths).
  some_dep:
    git: https://github.com/user/repo.git
    version: v1.0.0
```

Version ranges: `"^1.2.0"` (same major), `"~1.2.0"` (same minor), `">=1.2.0,<2.0.0"`, `"*"` (any — avoid).

## Mapping CMake REQUIRES → manifest dependencies

IDF built-in components (`driver`, `esp_timer`, `console`, `nvs_flash`, `fatfs`, `esp_driver_gpio`, `esp_driver_uart`, ...) stay ONLY in `idf_component_register(REQUIRES ...)` — they ship with ESP-IDF and never go in the manifest. Only external/managed components (things you'd fetch from the Registry or git) become manifest dependencies.
