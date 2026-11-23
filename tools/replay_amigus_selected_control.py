"""Validate a selected-control diagnostic log without accessing a target."""

import argparse
import json
from pathlib import Path
import re

HEADER = ("WAVETABLE-PROBE scope=Mini-7ea663e7-selected-control writes=NO bank_select=NO "
          "bank_identity=UNKNOWN voices_stopped=NOT_QUALIFIED")
MISSING_LIBRARY = "WAVETABLE result=SKIP reason=library-unavailable rc=5"
CONTROL = re.compile(r"WAVETABLE SELECTED CONTROL offset=0x20 value=0x([0-9a-f]{4})")
RELEASE = "WAVETABLE RELEASE confirmed=1 retained=0 driver=0x00000000"
FINISHED = "WAVETABLE result=PASS reason=observed reads=1 rc=0"


def replay(text, returncode):
    rows = text.splitlines()
    if returncode == 5 and rows == [HEADER, MISSING_LIBRARY]:
        return {"selected_control_observed": False, "outcome": "library-unavailable"}
    if returncode != 0 or len(rows) != 4 or rows[0] != HEADER or rows[-2:] != [RELEASE, FINISHED]:
        raise ValueError("Missing selected-control result or verified original release")
    control = CONTROL.fullmatch(rows[1])
    if not control:
        raise ValueError("Missing readback at the documented selected-control offset")
    value = int(control[1], 16)
    return {"selected_control_observed": True, "control_word": value,
            "playback_control_bit": bool(value & 0x8000), "bank_identity": None,
            "original_release_confirmed": True}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("log", type=Path)
    parser.add_argument("--returncode", required=True, type=int)
    args = parser.parse_args()
    try:
        report = replay(args.log.read_text(encoding="ascii"), args.returncode)
    except ValueError as error:
        parser.exit(1, f"{error}\n")
    print(json.dumps(report, indent=2))
