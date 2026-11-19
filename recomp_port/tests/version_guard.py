#!/usr/bin/env python3
"""Future-proof XMVSF recomp version checks for Python preflight tests.

Only the two real C++ menu version labels count. Each source must contain
one unambiguous quoted TEST N.N literal. Compare versions numerically and
apply a minimum required by the feature being tested; NEVER pin tests to
the latest version or maintain a list of all previous versions.
"""
from __future__ import annotations

import re

_VERSION = re.compile(r'"TEST\s+([0-9]+)\.([0-9]+)"')


def source_version(source: str, label: str) -> tuple[int, int]:
    versions = {(int(major), int(minor)) for major, minor in _VERSION.findall(source)}
    if len(versions) != 1:
        raise AssertionError(
            f"{label}: expected one unambiguous quoted TEST version, got {sorted(versions)}"
        )
    return next(iter(versions))


def require_minimum_version(
    source: str, minimum: tuple[int, int], label: str
) -> tuple[int, int]:
    version = source_version(source, label)
    if version < minimum:
        raise AssertionError(
            f"{label}: TEST {version[0]}.{version[1]} is older than required "
            f"TEST {minimum[0]}.{minimum[1]}"
        )
    return version


def require_matching_versions(
    home: str, renderer: str, minimum: tuple[int, int] = (87, 0)
) -> tuple[int, int]:
    home_version = require_minimum_version(home, minimum, "menu source")
    render_version = require_minimum_version(renderer, minimum, "texture renderer")
    if home_version != render_version:
        raise AssertionError(
            f"menu version {home_version} differs from renderer version {render_version}"
        )
    return home_version
