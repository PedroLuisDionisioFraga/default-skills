# ESP Registry Rules and Release Automation

## Registry rules

- One repo root = one component: `idf_component.yml` + `idf_component_register()` at root.
- Repo must be **public** for the Registry to serve it.
- A published version is immutable — you can `yank` or `delete` it, but never re-upload the same version number.
- Indexing after upload takes a few minutes.
- Consumers install with `idf.py add-dependency "<namespace>/<name>^X.Y.Z"`.

## One-time setup (user actions — remind, you cannot do these)

1. Create an API token at <https://components.espressif.com/settings/tokens> (scope `write:components`).
2. Add repo secret `IDF_COMPONENT_API_TOKEN` (GitHub → Settings → Secrets and variables → Actions).
3. Make the repository public.

## How templates/workflows/release.yml works (push to main)

1. Reads `version` from `idf_component.yml` (regex, no deps).
2. Queries `https://components.espressif.com/api/components/<ns>/<name>`; if the version is already published → warning, upload skipped (tag/release still ensured).
3. Creates annotated tag `v<version>` + GitHub release with generated notes (skips if the release exists).
4. Uploads via `espressif/upload-components-ci-action@v2` using the secret.

Namespace/component are set once in the workflow's `env:` block — keep them in sync with the manifest `name`.

**Version bump flow:** edit `version` in `idf_component.yml` + move CHANGELOG entries from `[Unreleased]` to the new version → merge to `main` → workflow does the rest.

## CI maintenance

- `build.yml`: matrix `example:` must list every `examples/` dir; `idf_ver` tags come from [espressif/idf docker tags](https://hub.docker.com/r/espressif/idf/tags) (`release-v5.5`, `latest`, ...); keep `target:` a small subset of manifest targets for speed.
- `format.yml`: `clang-format --dry-run --Werror` over `src include examples`; adjust paths if layout differs.
- Examples build against LOCAL code because their manifests use `override_path` — that is what makes these workflows meaningful for PRs.

## compote CLI (manual fallback / cleanup)

```bash
compote registry login
compote component pack   --name <name> --version X.Y.Z
compote component upload --namespace <ns> --name <name>
compote component yank   --namespace <ns> --name <name> --version X.Y.Z   # hide from resolution
compote component delete --namespace <ns> --name <name> --version X.Y.Z   # permanent, not re-uploadable
```
