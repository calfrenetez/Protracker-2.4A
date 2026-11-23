"""Validate a selected-voice diagnostic log without accessing a target."""

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
VOICE_HEADER = ("WAVETABLE-PROBE scope=Mini-7ea663e7-selected-voice writes=NO bank_select=NO "
                "bank_identity=UNKNOWN sample_ram_quiet=NOT_QUALIFIED")
VOICE_WORD = re.compile(r"WAVETABLE VOICE offset=0x([0-9a-f]{2}) value=0x([0-9a-f]{4})")
VOICE_OFFSETS = range(0x20, 0x3e, 2)
VOICE_FINISHED = "WAVETABLE result=PASS reason=observed reads=15 rc=0"


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


def replay_selected_voice(text, returncode):
    rows = text.splitlines()
    if returncode == 5 and rows == [VOICE_HEADER, MISSING_LIBRARY]:
        return {"selected_voice_observed": False, "outcome": "library-unavailable"}
    if (returncode != 0 or len(rows) != len(VOICE_OFFSETS) + 3 or rows[0] != VOICE_HEADER
            or rows[-2:] != [RELEASE, VOICE_FINISHED]):
        raise ValueError("Missing complete selected-voice snapshot or verified original release")

    words = {}
    for offset, row in zip(VOICE_OFFSETS, rows[1:-2]):
        match = VOICE_WORD.fullmatch(row)
        if not match or int(match[1], 16) != offset:
            raise ValueError(f"Missing or out-of-order selected-voice offset 0x{offset:02x}")
        words[f"0x{offset:02x}"] = int(match[2], 16)
    return {"selected_voice_observed": True, "register_words": words,
            "playback_control_bit": bool(words["0x20"] & 0x8000), "bank_identity": None,
            "original_release_confirmed": True, "sample_ram_quiet_qualified": False}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("log", type=Path)
    parser.add_argument("--returncode", required=True, type=int)
    parser.add_argument("--selected-voice", action="store_true")
    args = parser.parse_args()
    try:
        verify = replay_selected_voice if args.selected_voice else replay
        report = verify(args.log.read_text(encoding="ascii"), args.returncode)
    except ValueError as error:
        parser.exit(1, f"{error}\n")
    print(json.dumps(report, indent=2))
