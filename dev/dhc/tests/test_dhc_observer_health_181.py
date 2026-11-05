#!/usr/bin/env python3
"""Regression for 87.181 launcher/ROM/observer health classifications."""
import sys
from pathlib import Path
from tempfile import TemporaryDirectory
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
from ANALYZE_OBSERVER_HEALTH_181 import summarize


def run():
    with TemporaryDirectory() as temp:
        root = Path(temp)
        health = root / "health.log"
        old = root / "native.tsv"
        sub = root / "subframe.tsv"

        health.write_text("RUN_TEST_START date=2026-11-05\n", encoding="utf-8")
        result = summarize(health, old, sub)
        assert result["status"] == "NO_CPS2_FRAME", result

        # Game boot but never reaches match; TSV files must still have headers.
        health.write_text(
            "RUN_TEST_START date=2026-11-05\n"
            "ENGINE_HEALTH_INITIALIZED\t87.181\n"
            "CPS2_FIRST_FRAME\tframe=100\tdriver=xmvsfu\n"
            "XMVSF_FIRST_FRAME\tframe=100\tdriver=xmvsfu\t"
            "oldObserverReady=1\tsubframeObserverReady=1\n"
            "RUN_TEST_EXIT code=0\n",
            encoding="utf-8",
        )
        old.write_text("# XM87.175 OBSERVER SESSION\nframe\tmovie_frame\tstage\n",
                       encoding="utf-8")
        sub.write_text("# XM87.180 SUBFRAME SESSION\nframe\tcycles\tstage\n",
                       encoding="utf-8")
        result = summarize(health, old, sub)
        assert result["status"] == "NO_COMBAT_FRAME", result

        # Enter native combat, but post-CPU actor rows are still absent.
        health.write_text(
            health.read_text(encoding="utf-8").replace(
                "RUN_TEST_EXIT code=0\n",
                "XMVSF_FIRST_COMBAT\tframe=110\tdriver=xmvsfu\t"
                "oldObserverReady=1\tsubframeObserverReady=1\n"
                "RUN_TEST_EXIT code=0\n"),
            encoding="utf-8",
        )
        result = summarize(health, old, sub)
        assert result["status"] == "NO_NATIVE_OBSERVER_ROWS", result
        with old.open("a", encoding="utf-8") as f:
            f.write("111\t1\tPOST_CPU_PRE_DHC\n")
        result = summarize(health, old, sub)
        assert result["status"] == "NATIVE_OBSERVER_CAPTURED", result

        # Another launch with no driver must not inherit the old capture.
        with health.open("a", encoding="utf-8") as f:
            f.write("RUN_TEST_START date=2026-11-06\nRUN_TEST_EXIT code=0\n")
        result = summarize(health, old, sub)
        assert result["status"] == "NO_CPS2_FRAME", result
        print("PASS 87.181: no CPS2, noncombat, empty observer, capture, session reset")


if __name__ == "__main__":
    run()
