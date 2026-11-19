#!/usr/bin/env python3
"""Early, read-only policy check for recomp Python patches and build versions."""
from __future__ import annotations

import ast
from pathlib import Path
import re

from version_guard import require_matching_versions, source_version

ROOT = Path(__file__).resolve().parents[2]
PORT = ROOT / "recomp_port"
TESTS = PORT / "tests"
PINNED = re.compile(r"\bTEST[ ]+[0-9]+\.[0-9]+")


def sample(major: int, minor: int) -> str:
    return f'const char *version = "TEST {major}.{minor}";'


def expect_assertion(callback) -> None:
    try:
        callback()
    except AssertionError:
        return
    raise AssertionError("version guard failed to reject invalid input")


def main() -> None:
    home = (PORT / "xmvsf_home.h").read_text(encoding="utf-8")
    renderer = (PORT / "xmvsf_menu_textures.h").read_text(encoding="utf-8")
    current = require_matching_versions(home, renderer, minimum=(87, 455))
    assert current >= (87, 455)

    # A future build number must not require editing controller/menu tests.
    for version in ((87, 455), (87, 456), (87, 999), (88, 0)):
        assert require_matching_versions(
            sample(*version), sample(*version), minimum=(87, 455)
        ) == version
    expect_assertion(lambda: source_version("no version", "missing"))
    expect_assertion(lambda: source_version(
        sample(87, 455) + sample(87, 456), "ambiguous"
    ))
    expect_assertion(lambda: require_matching_versions(
        sample(87, 455), sample(87, 456), minimum=(87, 455)
    ))
    expect_assertion(lambda: require_matching_versions(
        sample(87, 454), sample(87, 454), minimum=(87, 455)
    ))

    # AST inspection, not a fragile substring test: ban literal TEST N.N
    # constants anywhere in test Python code. Feature tests must use
    # require_matching_versions / require_minimum_version instead.
    # Compile Python sources without emitting files or generating caches.
    python_files = sorted(PORT.glob("*.py")) + sorted(TESTS.glob("*.py"))
    for path in python_files:
        source = path.read_text(encoding="utf-8")
        compile(source, str(path), "exec")
    for path in sorted(TESTS.glob("test_*.py")):
        tree = ast.parse(path.read_text(encoding="utf-8"), filename=str(path))
        for node in ast.walk(tree):
            if isinstance(node, ast.Constant) and isinstance(node.value, str):
                if PINNED.search(node.value):
                    raise AssertionError(
                        f"{path.name}:{getattr(node, 'lineno', '?')}: "
                        "hardcoded build version in test; use version_guard.py"
                    )
    print(
        f"XMVSF Python preflight PASS: menu/renderer version {current[0]}.{current[1]}, "
        f"{len(python_files)} scripts parsed, no pinned test versions"
    )


if __name__ == "__main__":
    main()
