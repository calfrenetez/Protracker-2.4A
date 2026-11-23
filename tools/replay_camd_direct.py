"""Check separately bounded CIA brackets around CAMD's local PutMidi call."""

import json
from pathlib import Path
import re
import sys

if __package__:
    from .replay_routed_paula_window import stable
else:
    from replay_routed_paula_window import stable


GROUP = re.compile(
    r"CAMD direct group (\d+) hz=(\d+) timer=([01])/([01]) start=([0-9a-f]{8}):([0-9a-f]{8}) "
    r"end=([0-9a-f]{8}):([0-9a-f]{8}) attempted=4 coherent=(\d+)"
)
ROW = re.compile(
    r"CAMD direct row (\d+) baseline=([0-9a-f]{6})/([0-9a-f]{6}) put=([0-9a-f]{6})/([0-9a-f]{6}) "
    r"message=([0-9a-f]{6}) stamp=([0-9a-f]{8}) received=1"
)
GROUPS, PAIRS_PER_GROUP = 8, 4


def replay(text):
    lines = text.splitlines()
    if len(lines) != GROUPS * (PAIRS_PER_GROUP + 1) + 1 or (
            lines[-1] != "CAMD direct closed: original clock, library, node and links released rc=0"):
        raise ValueError("Missing complete CAMD observation or resource closure")
    reports, baseline, puts = [], [], []
    previous_end = 0
    hz = None
    for index in range(GROUPS):
        start = index * (PAIRS_PER_GROUP + 1)
        clock = GROUP.fullmatch(lines[start])
        if not clock or int(clock[1]) != index:
            raise ValueError("Missing original CIA and E-clock group")
        rate, chip, timer = map(int, clock.groups()[1:4])
        if rate not in (709379, 715909) or hz not in (None, rate):
            raise ValueError("Inconsistent original E-clock rate")
        hz = rate
        first, last = (int(clock[i] + clock[i + 1], 16) for i in (5, 7))
        if first < previous_end or not 0 < last - first < 60000:
            raise ValueError("Independent E-clock did not bound one complete countdown")
        previous_end = last
        previous_count = 65536
        coherent = 0
        for offset in range(PAIRS_PER_GROUP):
            row = ROW.fullmatch(lines[start + offset + 1])
            ordinal = index * PAIRS_PER_GROUP + offset
            expected = 0x863C00 if ordinal & 1 else 0x963C64
            if (not row or int(row[1]) != ordinal or int(row[6], 16) != expected
                    or int(row[7], 16) != (0xCAD10000 | ordinal)):
                raise ValueError("Wrong local receive value or receive timestamp")
            samples = []
            for raw in row.groups()[1:5]:
                if raw[:2] != raw[4:]:
                    samples.append(None)
                    continue
                value = stable(raw)
                if not 0 < value < previous_count or 65535 - value > last - first:
                    raise ValueError("Direct CIA observations stopped, regressed or exceeded the independent bound")
                previous_count = value
                samples.append(value)
            if None not in samples:
                baseline.append(samples[0] - samples[1])
                puts.append(samples[2] - samples[3])
                coherent += 1
        if coherent != int(clock[9]):
            raise ValueError("Reported coherent count differs from the group's retained raw observations")
        reports.append({"clock_chip": chip, "clock_timer": timer, "independent_ticks": last - first,
                        "coherent": coherent})
    if not 20 <= len(puts) <= GROUPS * PAIRS_PER_GROUP:
        raise ValueError("The minimum number of coherent observations was not met")
    return {"counter_hz": hz, "local_receipts": GROUPS * PAIRS_PER_GROUP, "groups": reports,
            "coherent_pairs": len(puts), "baseline_ticks": [min(baseline), max(baseline)],
            "put_bracket_ticks": [min(puts), max(puts)],
            "put_brackets_narrower_than_maximum_frame": sum(width < (hz + 47999) // 48000 for width in puts),
            "closed": True}


if __name__ == "__main__":
    try:
        result = replay(Path(sys.argv[1]).read_text(encoding="ascii"))
    except ValueError as error:
        print(error, file=sys.stderr)
        sys.exit(1)
    print(json.dumps(result, indent=2))
