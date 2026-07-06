#!/usr/bin/env python3
"""Validate a repository against the ESP Component Registry architecture.

Read-only. Prints one line per check ([PASS]/[FAIL]/[WARN]) and exits 1 if
any check fails. Used by the esp-idf-component skill for the audit mode and
as the final gate before declaring a component publish-ready.

Usage:
  python check_component.py [REPO_DIR]   (default: current directory)
"""

import re
import sys
from pathlib import Path

SKIP_DIRS = {".git", "build", "managed_components", ".claude", "node_modules", ".venv", "dist"}
PLACEHOLDER_RE = re.compile(r"\{\{[A-Z_]+\}\}")
SEMVER_RE = re.compile(r"^\d+\.\d+\.\d+(-[0-9A-Za-z.-]+)?$")
NAME_RE = re.compile(r"^[a-z][a-z0-9_-]*$")

results = []


def check(ok, label, fix=""):
    status = "PASS" if ok else "FAIL"
    line = f"[{status}] {label}"
    if not ok and fix:
        line += f" -> fix: {fix}"
    results.append((status, line))


def warn(label):
    results.append(("WARN", f"[WARN] {label}"))


def load_yaml(path: Path):
    try:
        import yaml  # type: ignore
    except ImportError:
        return None
    try:
        return yaml.safe_load(path.read_text(encoding="utf-8"))
    except Exception as exc:  # parse error is a finding, not a crash
        return exc


def main() -> int:
    root = Path(sys.argv[1] if len(sys.argv) > 1 else ".").resolve()
    if not root.is_dir():
        print(f"error: not a directory: {root}", file=sys.stderr)
        return 2
    print(f"Auditing: {root}\n")

    # --- Manifest ---
    manifest_path = root / "idf_component.yml"
    check(manifest_path.is_file(), "root idf_component.yml exists",
          "create it from the skill's templates/idf_component.yml")
    manifest = {}
    if manifest_path.is_file():
        loaded = load_yaml(manifest_path)
        if loaded is None:
            warn("PyYAML not installed — manifest field checks limited to regex")
            text = manifest_path.read_text(encoding="utf-8")
            m_name = re.search(r"^name:\s*[\"']?([^\"'\s]+)", text, re.M)
            m_ver = re.search(r"^version:\s*[\"']?([^\"'\s]+)", text, re.M)
            manifest = {"name": m_name and m_name.group(1), "version": m_ver and m_ver.group(1)}
            manifest["targets"] = ["?"] if re.search(r"^targets:", text, re.M) else None
            manifest["examples"] = ["?"] if re.search(r"^examples:", text, re.M) else None
        elif isinstance(loaded, Exception):
            check(False, "idf_component.yml parses as YAML", f"YAML error: {loaded}")
        else:
            manifest = loaded or {}
            check(True, "idf_component.yml parses as YAML")

        name = manifest.get("name")
        check(bool(name) and NAME_RE.match(str(name)) is not None,
              f"component name is lowercase/registry-safe ({name!r})",
              "use lowercase letters, digits, - or _")
        version = manifest.get("version")
        check(bool(version) and SEMVER_RE.match(str(version)) is not None,
              f"version is semver ({version!r})", "use MAJOR.MINOR.PATCH")
        check(bool(manifest.get("targets")), "manifest lists targets:",
              "omitting targets implies EVERY chip is supported")
        examples = manifest.get("examples")
        check(bool(examples), "manifest lists examples:",
              "add `examples: [- path: examples/basic]` so the Registry packs them")
        if isinstance(examples, list):
            for entry in examples:
                path = entry.get("path") if isinstance(entry, dict) else None
                if path and path != "?":
                    check((root / path).is_dir(), f"manifest example path exists: {path}")

    # --- Layout ---
    cmake = root / "CMakeLists.txt"
    check(cmake.is_file(), "root CMakeLists.txt exists")
    if cmake.is_file():
        text = cmake.read_text(encoding="utf-8")
        check("idf_component_register" in text, "root CMakeLists registers the component",
              "root must call idf_component_register(), it IS the component")
        check(re.search(r"^\s*project\s*\(", text, re.M) is None,
              "root CMakeLists has no project() boilerplate",
              "project() belongs only in examples/*/CMakeLists.txt")
    check((root / "src").is_dir(), "src/ directory exists (flat Registry layout)",
          "move implementation files to src/")
    check((root / "include").is_dir(), "include/ directory exists",
          "move public headers to include/")

    # --- Examples ---
    examples_dir = root / "examples"
    example_dirs = sorted(d for d in examples_dir.iterdir() if d.is_dir()) if examples_dir.is_dir() else []
    check(bool(example_dirs), "examples/ contains at least one example project",
          "scaffold one from templates/example-basic")
    for ex in example_dirs:
        check((ex / "CMakeLists.txt").is_file(), f"{ex.name}: has project CMakeLists.txt")
        ex_manifest = ex / "main" / "idf_component.yml"
        check(ex_manifest.is_file(), f"{ex.name}: has main/idf_component.yml",
              "declare the component dependency there")
        if ex_manifest.is_file():
            check("override_path" in ex_manifest.read_text(encoding="utf-8"),
                  f"{ex.name}: dependency uses override_path",
                  'add `override_path: "../../../"` so CI builds local code, not the published version')

    # --- Workflows and repo hygiene ---
    for wf in ("release.yml", "build.yml", "format.yml"):
        check((root / ".github" / "workflows" / wf).is_file(), f".github/workflows/{wf} exists",
              f"copy from the skill's templates/workflows/{wf}")
    for fname, why in (
        (".clang-format", "consistent formatting + format.yml CI"),
        (".gitignore", "keep build/, sdkconfig, managed_components/ out of git"),
        ("LICENSE", "the Registry displays the license"),
        ("CHANGELOG.md", "consumers need release history"),
        ("README.md", "the Registry renders it as the component page"),
    ):
        check((root / fname).is_file(), f"{fname} exists", why)
    readme = root / "README.md"
    if readme.is_file() and "components.espressif.com/components" not in readme.read_text(encoding="utf-8", errors="replace"):
        warn("README has no ESP Registry badge/link (components.espressif.com/components/...)")

    # --- Leftover placeholders ---
    leftovers = []
    for path in root.rglob("*"):
        if any(part in SKIP_DIRS for part in path.parts):
            continue
        if not path.is_file() or path.stat().st_size > 512 * 1024:
            continue
        try:
            if PLACEHOLDER_RE.search(path.read_text(encoding="utf-8", errors="strict")):
                leftovers.append(path.relative_to(root))
        except (UnicodeDecodeError, OSError):
            continue
    check(not leftovers, "no leftover {{PLACEHOLDER}} markers",
          f"found in: {', '.join(map(str, leftovers[:10]))}")

    # --- Report ---
    print("\n".join(line for _, line in results))
    fails = sum(1 for status, _ in results if status == "FAIL")
    warns = sum(1 for status, _ in results if status == "WARN")
    passes = sum(1 for status, _ in results if status == "PASS")
    print(f"\n{passes} passed, {fails} failed, {warns} warnings")
    if fails == 0:
        print("Structure is publish-ready. Still required (cannot be checked here): "
              "each example builds with idf.py, repo is public, IDF_COMPONENT_API_TOKEN secret set.")
    return 1 if fails else 0


if __name__ == "__main__":
    sys.exit(main())
