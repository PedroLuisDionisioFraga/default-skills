#!/usr/bin/env python3
"""Apply the esp-idf-component skill templates to a target repository.

Copies templates/ files to their destinations, replacing {{PLACEHOLDER}} keys.
Never overwrites existing files unless --force is given, so in convert mode
you can move real project files into place FIRST and the script fills the gaps.

Usage:
  python apply_templates.py --target DIR --mode create|convert \
      --set COMPONENT_NAME=my-comp --set DESCRIPTION="..." [--set KEY=VALUE ...] \
      [--with-kconfig] [--force] [--dry-run]

Required keys: COMPONENT_NAME, DESCRIPTION, REPO_URL.
Defaulted keys: NAMESPACE, VERSION, IDF_MIN_VERSION, AUTHOR (git user.name).
Derived keys:   COMPONENT_NAME_SNAKE/_UPPER, COMPONENT_TITLE, YEAR, DATE.
"""

import argparse
import datetime
import re
import subprocess
import sys
from pathlib import Path

SKILL_DIR = Path(__file__).resolve().parent.parent
TEMPLATES = SKILL_DIR / "templates"

DEFAULTS = {
    "NAMESPACE": "pedroluisdionisiofraga",
    "VERSION": "0.1.0",
    "IDF_MIN_VERSION": "5.5.0",
}

# (template relative path, destination relative path, modes it applies to)
MAPPING = [
    ("idf_component.yml", "idf_component.yml", {"create", "convert"}),
    ("CMakeLists.txt", "CMakeLists.txt", {"create", "convert"}),
    ("component.h", "include/{{COMPONENT_NAME}}.h", {"create"}),
    ("component.c", "src/{{COMPONENT_NAME}}.c", {"create"}),
    ("clang-format", ".clang-format", {"create", "convert"}),
    ("gitignore", ".gitignore", {"create", "convert"}),
    ("LICENSE", "LICENSE", {"create", "convert"}),
    ("CHANGELOG.md", "CHANGELOG.md", {"create", "convert"}),
    ("README.md", "README.md", {"create", "convert"}),
    ("publish_to_esp_registry.md", "docs/publish_to_esp_registry.md", {"create", "convert"}),
    ("workflows/release.yml", ".github/workflows/release.yml", {"create", "convert"}),
    ("workflows/build.yml", ".github/workflows/build.yml", {"create", "convert"}),
    ("workflows/format.yml", ".github/workflows/format.yml", {"create", "convert"}),
    ("example-basic/CMakeLists.txt", "examples/basic/CMakeLists.txt", {"create", "convert"}),
    ("example-basic/sdkconfig.defaults", "examples/basic/sdkconfig.defaults", {"create", "convert"}),
    ("example-basic/README.md", "examples/basic/README.md", {"create", "convert"}),
    ("example-basic/main/CMakeLists.txt", "examples/basic/main/CMakeLists.txt", {"create", "convert"}),
    ("example-basic/main/idf_component.yml", "examples/basic/main/idf_component.yml", {"create", "convert"}),
    # main.c only in create mode: in convert mode the old app's main.c is moved in by hand.
    ("example-basic/main/main.c", "examples/basic/main/main.c", {"create"}),
]
KCONFIG_ENTRY = ("Kconfig", "Kconfig", {"create", "convert"})

NAME_RE = re.compile(r"^[a-z][a-z0-9_-]*$")
PLACEHOLDER_RE = re.compile(r"\{\{([A-Z_]+)\}\}")


def git_user_name(target: Path) -> str:
    try:
        out = subprocess.run(
            ["git", "config", "user.name"], cwd=target, capture_output=True, text=True, check=False
        )
        return out.stdout.strip()
    except OSError:
        return ""


def derive(values: dict) -> dict:
    name = values["COMPONENT_NAME"]
    snake = name.replace("-", "_")
    now = datetime.date.today()
    derived = {
        "COMPONENT_NAME_SNAKE": snake,
        "COMPONENT_NAME_UPPER": snake.upper(),
        "COMPONENT_TITLE": " ".join(w.upper() if len(w) <= 3 else w.capitalize() for w in snake.split("_")),
        "YEAR": str(now.year),
        "DATE": now.isoformat(),
    }
    for key, val in derived.items():
        values.setdefault(key, val)
    return values


def substitute(text: str, values: dict, missing: set) -> str:
    def repl(match):
        key = match.group(1)
        if key in values:
            return values[key]
        missing.add(key)
        return match.group(0)

    return PLACEHOLDER_RE.sub(repl, text)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--target", required=True, help="Component repository root")
    parser.add_argument("--mode", required=True, choices=["create", "convert"])
    parser.add_argument("--set", dest="pairs", action="append", default=[], metavar="KEY=VALUE")
    parser.add_argument("--with-kconfig", action="store_true", help="Also scaffold a component Kconfig")
    parser.add_argument("--force", action="store_true", help="Overwrite existing destination files")
    parser.add_argument("--dry-run", action="store_true", help="Print actions without writing")
    args = parser.parse_args()

    target = Path(args.target).resolve()
    if not target.is_dir():
        print(f"error: target directory not found: {target}", file=sys.stderr)
        return 2

    values = dict(DEFAULTS)
    for pair in args.pairs:
        key, sep, val = pair.partition("=")
        if not sep:
            print(f"error: --set expects KEY=VALUE, got: {pair}", file=sys.stderr)
            return 2
        values[key.strip()] = val

    required = ["COMPONENT_NAME", "DESCRIPTION", "REPO_URL"]
    absent = [k for k in required if not values.get(k)]
    if absent:
        print(f"error: missing required --set keys: {', '.join(absent)}", file=sys.stderr)
        return 2
    if not NAME_RE.match(values["COMPONENT_NAME"]):
        print(
            f"error: COMPONENT_NAME '{values['COMPONENT_NAME']}' must be lowercase "
            "letters/digits/-/_ (ESP Registry rule)",
            file=sys.stderr,
        )
        return 2
    values["REPO_URL"] = values["REPO_URL"].rstrip("/").removesuffix(".git")
    if not values.get("AUTHOR"):
        values["AUTHOR"] = git_user_name(target) or "TODO-AUTHOR"
    derive(values)

    entries = list(MAPPING)
    if args.with_kconfig:
        entries.append(KCONFIG_ENTRY)

    written, skipped = [], []
    missing_keys: set = set()
    for tpl_rel, dest_rel, modes in entries:
        if args.mode not in modes:
            continue
        src = TEMPLATES / tpl_rel
        dest = target / substitute(dest_rel, values, missing_keys)
        if dest.exists() and not args.force:
            skipped.append(dest)
            continue
        content = substitute(src.read_text(encoding="utf-8"), values, missing_keys)
        if not args.dry_run:
            dest.parent.mkdir(parents=True, exist_ok=True)
            dest.write_text(content, encoding="utf-8", newline="\n")
        written.append(dest)

    for path in written:
        print(f"wrote   {path.relative_to(target)}")
    for path in skipped:
        print(f"skipped {path.relative_to(target)} (exists; use --force to overwrite)")
    if missing_keys:
        print(f"WARNING: unreplaced placeholders remain for: {', '.join(sorted(missing_keys))}", file=sys.stderr)
        return 1
    print(f"done: {len(written)} written, {len(skipped)} skipped ({args.mode} mode)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
