#!/usr/bin/env python3
"""Synchronize the vendored NORA GPIO HAL from the upstream HAL repo.

The upstream repository is nora-hal-dspic33ak-gpio (formerly dspic33ak-hal-gpio).
Its public API is nora_gpio_*; the _dspic33ak tag appears only on the backend
implementation files.
"""

from __future__ import annotations

import argparse
import re
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path


UPSTREAM_REPO = "https://github.com/sulaolab/nora-hal-dspic33ak-gpio.git"
UPSTREAM_BRANCH = "main"
UPSTREAM_SOURCE_DIR = "src"
DESTINATION_DIR = "src/hal_gpio"

HAL_FILES = (
    "nora_gpio.h",
    "nora_gpio_event.h",
    "nora_gpio_dspic33ak.c",
    "nora_gpio_event_dspic33ak.c",
    "nora_gpio_dspic33ak_reg.h",
)

# Files upstream ships that this repo deliberately does not vendor, each with the
# reason. Anything upstream adds that is in neither this mapping nor HAL_FILES makes
# the sync fail loudly -- see check_upstream_coverage(). A literal file list that
# silently skips new upstream files is how a vendored HAL goes quietly stale, so the
# omission has to be a decision on record rather than an oversight.
INTENTIONALLY_NOT_VENDORED = {
    "nora_pps.h": "PPS is out of scope: this wrapper is a GPIO-only validation "
                  "layer and does not own peripheral pin select or IRQ routing.",
    "nora_pps_dspic33ak.c": "see nora_pps.h",
    "nora_gpio_table.h": "the optional declarative pin table is a whole-board "
                         "bring-up convenience layered on top of nora_gpio.h. The "
                         "CMSIS-Driver GPIO API configures one pin per call, so the "
                         "wrapper neither calls it nor needs it to compile.",
    "nora_gpio_table_dspic33ak.c": "see nora_gpio_table.h",
}


def run(command: list[str], cwd: Path | None = None) -> str:
    result = subprocess.run(
        command,
        cwd=cwd,
        check=False,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    )
    if result.returncode != 0:
        if result.stdout:
            print(result.stdout, end="", file=sys.stdout)
        if result.stderr:
            print(result.stderr, end="", file=sys.stderr)
        raise SystemExit(result.returncode)
    return result.stdout.strip()


def require_repo_root(repo_root: Path) -> None:
    required_paths = (
        repo_root / "README.md",
        repo_root / DESTINATION_DIR / "UPSTREAM.md",
    )
    missing = [str(path.relative_to(repo_root)) for path in required_paths if not path.exists()]
    if missing:
        joined = ", ".join(missing)
        raise SystemExit(f"Run this script from the repository root; missing: {joined}")


def clone_upstream(work_dir: Path, branch: str) -> tuple[Path, str]:
    upstream_dir = work_dir / "upstream"
    run(
        [
            "git",
            "clone",
            "--depth",
            "1",
            "--branch",
            branch,
            UPSTREAM_REPO,
            str(upstream_dir),
        ]
    )
    upstream_commit = run(["git", "rev-parse", "HEAD"], cwd=upstream_dir)
    return upstream_dir, upstream_commit


def check_upstream_coverage(upstream_dir: Path) -> None:
    """Fail if upstream ships a source file this repo neither vendors nor excludes.

    HAL_FILES is a literal list, so on its own it copies what it names and says
    nothing about what it missed. When upstream adds a file the wrapper needs, the
    silent outcome is a vendored HAL that no longer builds -- or worse, one that
    builds against a stale header. Turning that into a hard error means every
    omission is either listed in INTENTIONALLY_NOT_VENDORED with a reason, or it
    stops the sync.
    """
    source_dir = upstream_dir / UPSTREAM_SOURCE_DIR
    shipped = {
        path.name
        for path in source_dir.iterdir()
        if path.is_file() and path.suffix in (".c", ".h")
    }
    unaccounted = sorted(shipped - set(HAL_FILES) - set(INTENTIONALLY_NOT_VENDORED))
    if unaccounted:
        listed = "\n".join(f"  {name}" for name in unaccounted)
        raise SystemExit(
            "Upstream ships source files this repo neither vendors nor excludes:\n"
            f"{listed}\n"
            "Add each to HAL_FILES, or to INTENTIONALLY_NOT_VENDORED with the reason."
        )

    missing = sorted(set(HAL_FILES) - shipped)
    if missing:
        listed = "\n".join(f"  {name}" for name in missing)
        raise SystemExit(f"HAL_FILES names files upstream no longer ships:\n{listed}")


def copy_hal_files(upstream_dir: Path, repo_root: Path) -> None:
    source_dir = upstream_dir / UPSTREAM_SOURCE_DIR
    destination_dir = repo_root / DESTINATION_DIR
    destination_dir.mkdir(parents=True, exist_ok=True)

    for file_name in HAL_FILES:
        source_path = source_dir / file_name
        if not source_path.is_file():
            raise SystemExit(f"Upstream file not found: {source_path}")
        shutil.copy2(source_path, destination_dir / file_name)


def update_upstream_md(repo_root: Path, upstream_commit: str) -> None:
    upstream_md = repo_root / DESTINATION_DIR / "UPSTREAM.md"
    text = upstream_md.read_text(encoding="utf-8")
    updated, replacements = re.subn(
        r"- Upstream commit: [0-9a-fA-F]+",
        f"- Upstream commit: {upstream_commit}",
        text,
        count=1,
    )
    if replacements != 1:
        raise SystemExit("Could not update upstream commit line in src/hal_gpio/UPSTREAM.md")

    # A branch-specific paragraph rewrite used to live here, aimed at the wording of
    # the very first import. That wording was replaced long ago, so the pattern could
    # no longer match anything -- and re.sub is silent about matching nothing, so it
    # looked like an active rule while doing exactly nothing. Removed rather than
    # retargeted: UPSTREAM.md's prose is maintained by hand.

    upstream_md.write_text(updated, encoding="utf-8", newline="\n")


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Synchronize the vendored NORA GPIO HAL from upstream."
    )
    parser.add_argument(
        "--branch",
        default=UPSTREAM_BRANCH,
        help=f"upstream branch or tag to clone (default: {UPSTREAM_BRANCH})",
    )
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    repo_root = Path.cwd().resolve()
    require_repo_root(repo_root)

    with tempfile.TemporaryDirectory(prefix="nora_gpio_hal_") as temp_dir:
        upstream_dir, upstream_commit = clone_upstream(Path(temp_dir), args.branch)
        check_upstream_coverage(upstream_dir)
        copy_hal_files(upstream_dir, repo_root)
        update_upstream_md(repo_root, upstream_commit)

    print(
        "Synchronized HAL from "
        f"sulaolab/nora-hal-dspic33ak-gpio {args.branch} @ {upstream_commit}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
