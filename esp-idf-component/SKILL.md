---
name: esp-idf-component
description: Use when creating a new ESP-IDF component for the ESP Component Registry (components.espressif.com), converting an existing ESP-IDF project into a publishable registry component, or auditing a component repo for publish-readiness — layout, idf_component.yml manifest, examples, release/build/format GitHub workflows, compote/upload-components-ci-action publishing.
---

# ESP-IDF Registry Component

Scaffold, convert, or audit a repository whose **root IS an ESP-IDF component**: `idf_component.yml` + `idf_component_register()` at the root (no `project()` there), sources in `src/` + `include/`, standalone consumer projects under `examples/`, automated release to the Registry. An optional target path may be given (e.g. `/esp-idf-component convert C:\path\to\project`); default is the current repo.

## Modes

| Mode | Trigger | What happens |
|---|---|---|
| `create` | New component from scratch | Apply all templates + source stubs |
| `convert` | Existing conventional ESP-IDF project | Restructure to component layout, old app becomes the example |
| `audit` | "Is this ready to publish?" / repo already has root manifest | **Read-only** report: run `scripts/check_component.py`, walk `QUICK_CHECK.md`, output ✅/❌/⚠️ per item with the fix for each ❌. Change files only if the user then asks. |

## Non-Negotiable Blockers

- Do not write any file before `COMPONENT_NAME` and `NAMESPACE` are confirmed (name rule: lowercase letters/digits/`-`/`_`).
- Never delete or overwrite a git-tracked file without listing it and getting explicit confirmation. Merge existing README/LICENSE/.gitignore by editing, never by replacing.
- Never claim an example builds without running `idf.py build` in it. If IDF is unavailable, say so in the final report.
- Do not declare a component publish-ready until `scripts/check_component.py` exits 0.

## Execute the Task

1. Classify the mode (`create` / `convert` / `audit`). Audit ends at the report — no edits.
2. Gather the values below. Derive what you can; ask the user only for the rest.
3. `convert` only: move real files first — follow `references/conversion-playbook.md`.
4. Run `scripts/apply_templates.py` (it skips existing files, so moves are safe):
   ```bash
   python scripts/apply_templates.py --target <repo> --mode create|convert \
     --set COMPONENT_NAME=my-comp --set DESCRIPTION="..." --set REPO_URL=https://github.com/u/r \
     [--set KEY=VALUE ...] [--with-kconfig] [--dry-run]
   ```
   (Windows: `py` if `python` is missing. Fix anything it reports as unreplaced.)
5. Fill in the judgment gaps: manifest `targets`/`tags`, README TODOs, example code that actually calls the API.
6. Verify: `python scripts/check_component.py <repo>` must pass, then `idf.py build` in each example if available.
7. Report in the Output Format below.

| Placeholder | Meaning | Default / source |
|---|---|---|
| `COMPONENT_NAME` | Registry name, e.g. `cli-api` | ask; from repo name |
| `DESCRIPTION` | Manifest description | ask / infer from code |
| `REPO_URL` | GitHub URL (no `.git`) | `git remote get-url origin` |
| `NAMESPACE` | Registry namespace | `pedroluisdionisiofraga` |
| `VERSION` | Initial semver | `0.1.0` (new) / `1.0.0` (mature) |
| `IDF_MIN_VERSION` | Minimum ESP-IDF | `5.5.0` |
| `AUTHOR` | LICENSE holder | `git config user.name` |

`SNAKE`/`UPPER`/`TITLE` name variants, `YEAR`, `DATE` are derived by the script. In workflow files, `${{ ... }}` is GitHub Actions syntax — never a placeholder.

## Use the References

- `references/manifest-reference.md` — every `idf_component.yml` field, the 3 dependency syntaxes (registry range / `override_path` / git), REQUIRES→manifest mapping.
- `references/conversion-playbook.md` — full convert procedure and edge cases (multiple libs, Kconfig vs Kconfig.projbuild, partition tables, existing workflows, dirty tree).
- `references/registry-and-release.md` — Registry rules, token/secret setup, how release.yml works, version bump flow, CI maintenance, compote commands.
- `QUICK_CHECK.md` — one-page publish-readiness checklist (drives audit mode).

## Output Format

- **create/convert**: what was written/moved (grouped, not file-by-file), key decisions (name, targets, deps mapping), verification results (check script output summary; whether `idf.py build` ran), then **User TODOs**: create `IDF_COMPONENT_API_TOKEN` secret, make repo public, review `targets`/`tags`, bump flow reminder.
- **audit**: ✅/❌/⚠️ list ordered by severity, each ❌ with its concrete fix, ending with the user-only items (secret, visibility).

## Common Mistakes

| Mistake | Fix |
|---|---|
| `project()` left in root CMakeLists | Root registers the component; only examples have `project()` |
| Sources left at repo root instead of `src/` | Follow the flat layout: `src/` + `include/` |
| Manifest missing `targets:` (implies EVERY chip) or `examples:` list | Both required; list only supported targets |
| Example depends on published version only, or `path:`-only dep | `version` + `override_path` — CI tests PR code, consumers get the Registry version |
| Upload workflow triggered by manually-created releases | release.yml template: manifest version bump drives tag + release + upload |
| Old `sdkconfig.defaults`/partition csv deleted during convert | They move into the example, not the trash |
| `build.yml` matrix not matching `examples/` dirs | One `example:` entry per directory |
| IDF built-ins (`driver`, `console`, ...) put in manifest dependencies | They stay in CMake `REQUIRES` only |

## Trigger Examples

- "Create a new ESP-IDF component for the registry called foo"
- "Convert this ESP-IDF project into a publishable component"
- "Is this repo ready to publish on components.espressif.com?"
- "Audit my component / what's missing to upload it to the ESP Registry?"
- "Set up release automation for my ESP-IDF component"
