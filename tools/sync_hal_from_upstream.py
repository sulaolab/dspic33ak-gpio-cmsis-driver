#!/usr/bin/env python3
"""Synchronize the vendored dsPIC33AK GPIO HAL from the upstream HAL repo."""

from __future__ import annotations

import argparse
import re
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path


UPSTREAM_REPO = "https://github.com/sulaolab/dspic33ak-gpio-hal.git"
UPSTREAM_BRANCH = "main"
UPSTREAM_SOURCE_DIR = "src"
DESTINATION_DIR = "src/hal_gpio"

HAL_FILES = (
    "dspic33ak_gpio.c",
    "dspic33ak_gpio.h",
    "dspic33ak_gpio_event.c",
    "dspic33ak_gpio_event.h",
    "dspic33ak_gpio_reg.h",
)


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


def copy_hal_files(upstream_dir: Path, repo_root: Path) -> None:
    source_dir = upstream_dir / UPSTREAM_SOURCE_DIR
    destination_dir = repo_root / DESTINATION_DIR
    destination_dir.mkdir(parents=True, exist_ok=True)

    for file_name in HAL_FILES:
        source_path = source_dir / file_name
        if not source_path.is_file():
            raise SystemExit(f"Upstream file not found: {source_path}")
        shutil.copy2(source_path, destination_dir / file_name)


def update_upstream_md(repo_root: Path, upstream_commit: str, branch: str) -> None:
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

    if branch == "main":
        updated = re.sub(
            r"This first import is synchronized from the upstream\n"
            r"`gpio-event-cmsis-wrapper-migration` branch while the GPIO event layer is being\n"
            r"reviewed\. After that branch is merged, the intended steady-state upstream\n"
            r"branch is `main`\.",
            "This revision is synchronized from the upstream `main` branch.",
            updated,
            count=1,
        )

    upstream_md.write_text(updated, encoding="utf-8", newline="\n")


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Synchronize the vendored dsPIC33AK GPIO HAL from upstream."
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

    with tempfile.TemporaryDirectory(prefix="dspic33ak_gpio_hal_") as temp_dir:
        upstream_dir, upstream_commit = clone_upstream(Path(temp_dir), args.branch)
        copy_hal_files(upstream_dir, repo_root)
        update_upstream_md(repo_root, upstream_commit, args.branch)

    print(
        "Synchronized HAL from "
        f"sulaolab/dspic33ak-gpio-hal {args.branch} @ {upstream_commit}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
