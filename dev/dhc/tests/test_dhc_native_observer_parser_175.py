#!/usr/bin/env python3
"""ROM-free regression for 87.175 observer report host/ROM attribution."""
from pathlib import Path
from tempfile import TemporaryDirectory
import sys
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
from ANALYZE_DHC_NATIVE_OBSERVER_175 import consume

HEADER = ("frame movie_frame stage side slot fighter phase inputSynthetic "
          "injectedDir injectedButtons d6 modeA0 phase08 action script34 "
          "controller256 controller259 a4 c0 c2 c4 c6 c8 ca").split()


def line(frame, stage, action="0000", phase="IDLE"):
    data = {x: "0000" for x in HEADER}
    data.update(frame=str(frame), movie_frame=str(frame), stage=stage,
                side="1", slot="POINT", fighter="0014", phase=phase,
                inputSynthetic="1", injectedDir="02", injectedButtons="03",
                d6="0C", modeA0="2A00", phase08="0000", action=action,
                script34="000D6722", controller256="00", controller259="00",
                a4="00", c0="0000", c2="0000", c4="0003",
                c6="0003", c8="0000", ca="0003")
    return "\t".join(data[h] for h in HEADER)


def main():
    with TemporaryDirectory() as directory:
        p = Path(directory) / "native_observer.tsv"
        p.write_text("\n".join([
            "# XM87.175 OBSERVER SESSION: sample",
            "\t".join(HEADER),
            line(100, "POST_CPU_PRE_DHC", "0000"),
            line(100, "POST_HOST_DHC_CHANGED", "2E00"),
            line(101, "POST_CPU_PRE_DHC", "2E00"),
            line(102, "POST_CPU_PRE_DHC", "0000"),
            line(103, "POST_CPU_PRE_DHC", "2E02"),
            "# XM87.175 OBSERVER SESSION: reset",
            "\t".join(HEADER),
            line(5, "POST_CPU_PRE_DHC", "0000"),
        ]) + "\n", encoding="utf-8")
        result = consume(p)
        counts = result["counts"]
        assert counts["sessions"] == 2, result
        assert counts["post_cpu_2e_echo_of_host_write"] == 1, result
        assert counts["post_cpu_2e_action_transitions"] == 1, result
        assert counts["host_changed_actor_rows"] == 1, result
        assert counts.get("malformed_rows", 0) == 0, result
        assert counts.get("unparsed_rows", 0) == 0, result
        print("PASS: 87.175 observer sessions, host echo vs CPU candidates, native reset")


if __name__ == "__main__":
    main()
