"""Check the owned continuous CIA counter against independent E-clock bounds."""

import json
from pathlib import Path
import re
import sys

if __package__:
    from .replay_routed_paula_window import stable
else:
    from replay_routed_paula_window import stable


PROBE = re.compile(r"CIA reload probe ([01]) before=([\da-f]{8}):([\da-f]{8}) "
                   r"after=([\da-f]{8}):([\da-f]{8}) calls=(\d+)/(\d+) count=([\da-f]{6})")
FINISHED = re.compile(r"CIA reload finished hz=(\d+) chip=([01]) timer=([01]) wakes=(\d+) calls=(\d+) "
                      r"bounds=(\d+)/(\d+) ticks=(\d+) closed=1 rc=(0|20)")
PERIOD = 65536
MIN_RELOADS = 1024
MAX_RELOADS = 1200


def joined(high, low):
    return (int(high, 16) << 32) + int(low, 16)


def replay(text):
    lines = text.splitlines()
    if len(lines) != 3:
        raise ValueError("Missing both original counter observations and guest closure")
    finished = FINISHED.fullmatch(lines[-1])
    if not finished:
        raise ValueError("No independently checkable counter summary")
    hz, chip, timer, wakes, calls, reported_min, reported_max, reported_ticks, rc = map(int, finished.groups())
    if hz not in (709379, 715909) or not 0 < wakes <= calls or not MIN_RELOADS <= calls <= MAX_RELOADS:
        raise ValueError("Unexpected E-clock rate or missing original underflow notifications")
    points = []
    for index, (line, reloads) in enumerate(zip(lines[:2], (0, calls))):
        match = PROBE.fullmatch(line)
        if not match or int(match[1]) != index or (int(match[6]), int(match[7])) != (reloads, reloads):
            raise ValueError("Counter generation changed across the sample")
        lo, hi, count = joined(match[2], match[3]), joined(match[4], match[5]), stable(match[8])
        if lo >= hi or count <= 4096:
            raise ValueError("Invalid or near-underflow raw probe")
        points.append((lo, hi, count))
    first, final = points
    lower, upper = final[0] - first[1], final[1] - first[0]
    delta = calls * PERIOD + first[2] - final[2]
    if lower <= 0 or (lower, upper, delta) != (reported_min, reported_max, reported_ticks):
        raise ValueError("Reported elapsed ticks disagree with the raw samples")
    if not delta - calls < lower <= delta <= upper < delta + calls:
        raise ValueError("Reference interval does not distinguish the assumed period")
    if rc:
        raise ValueError("Guest retained a nonzero diagnostic result")
    return {"hz": hz, "clock_chip": chip, "clock_timer": timer, "reloads": calls, "wake_observations": wakes,
            "selected_period_ticks": PERIOD, "independent_elapsed_bounds": [lower, upper],
            "reconstructed_elapsed_ticks": delta, "closed": True}


if __name__ == "__main__":
    try:
        result = replay(Path(sys.argv[1]).read_text(encoding="ascii"))
    except ValueError as error:
        print(error, file=sys.stderr)
        sys.exit(1)
    print(json.dumps(result, indent=2))
