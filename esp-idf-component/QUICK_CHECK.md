# ESP Registry Component: One-Page Publish-Readiness Check

Run through this before declaring a component ready to publish (and as the checklist for `audit` mode). Mechanical items are covered by `scripts/check_component.py`; the judgment items below need eyes.

## 1) Identity and Manifest

- [ ] `scripts/check_component.py` passes (manifest parses, name lowercase, semver version, targets, examples).
- [ ] `description` actually describes the component (not a stub/TODO).
- [ ] `targets:` lists ONLY chips that are supported/tested — no aspirational entries.
- [ ] `tags:` are searchable terms a consumer would type on components.espressif.com.
- [ ] `dependencies: idf:` minimum matches the oldest IDF actually tested.

## 2) Layout

- [ ] Root `CMakeLists.txt` calls `idf_component_register()` — no `project()` at root.
- [ ] Sources in `src/`, public headers in `include/` (flat Registry layout).
- [ ] `Kconfig` exists only if the component really has options (delete the stub otherwise).

## 3) Examples

- [ ] Every `examples/<dir>` is a standalone IDF project (own `CMakeLists.txt` + `main/`).
- [ ] Each example's `main/idf_component.yml` uses `version` **and** `override_path: "../../../"`.
- [ ] Example code actually calls the component's API (not just prints).
- [ ] Every example dir is listed under `examples:` in the root manifest.

## 4) Workflows / CI

- [ ] `build.yml` matrix has one `example:` entry per `examples/` directory.
- [ ] `build.yml` targets are a subset of the manifest `targets:`.
- [ ] `release.yml` `env: NAMESPACE`/`COMPONENT` match the manifest name and registry namespace.
- [ ] `format.yml` paths cover where the code actually lives.

## 5) Docs

- [ ] README has the Registry badge, install command, usage snippet, examples table.
- [ ] `CHANGELOG.md` has an entry for the version in `idf_component.yml`.
- [ ] `docs/publish_to_esp_registry.md` present.
- [ ] LICENSE year/author correct.

## 6) GitHub Setup (user must confirm — cannot be checked from files)

- [ ] Repository is **public**.
- [ ] Secret `IDF_COMPONENT_API_TOKEN` created (token from <https://components.espressif.com/settings/tokens>).

## 7) Final Commands

```bash
python scripts/check_component.py <repo>       # from the skill dir
grep -rnE '\{\{[A-Z_]+\}\}' <repo>             # no leftovers ( ${{ ... }} won't match )
idf.py build                                   # inside EACH examples/<dir>, if IDF available
```

- [ ] If `idf.py` was not run, the final report says so explicitly — never claim examples build.
