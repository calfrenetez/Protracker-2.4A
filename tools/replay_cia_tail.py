"""Verify a RAM diagnostic aperture inside a frame that spans one CIA reload."""

import json
from pathlib import Path
import re
import sys

if __package__:
    from .replay_routed_paula_window import stable
else:
    from replay_routed_paula_window import stable


COUNTER = re.compile(
    r"CIA tail hz=(\d+) clock=([01])/([01]) alarm=([01])/([01]) original=([0-9a-f]{6}) "
    r"after=([0-9a-f]{6}) after_tick=(\d+) reloads=1"
)
PLAN = re.compile(
    r"CIA tail plan frame=(\d+)/(\d+) aperture=(\d+)/(\d+) count=(\d+)/(\d+) "
    r"too_short=(\d+)/(\d+) refused=1"
)
ISSUE = re.compile(
    r"CIA tail issue generation=0 status=1 arm=([0-9a-f]{6}) lead=(\d+) delay=(\d+) attempts=(\d+) "
    r"pre=([0-9a-f]{6}) post=([0-9a-f]{6}) mark=4752 ticks=(\d+)/(\d+)"
)


def replay(text):
    lines = text.splitlines()
    if len(lines) != 4 or lines[-1] != "CIA tail finished callbacks=1 stray=0 closed=1 rc=0":
        raise ValueError("Missing complete assertions and original owner closure")
    clock, plan, issue = (pattern.fullmatch(line) for pattern, line in zip((COUNTER, PLAN, ISSUE), lines))
    if not clock or not plan or not issue:
        raise ValueError("Missing the original generation, frame, or issue evidence")
    hz = int(clock[1])
    anchor, later = stable(clock[6]), stable(clock[7])
    reload_tick = anchor + 1
    rebased_tick = 65536 + anchor - later
    if (hz not in (709379, 715909) or clock.groups()[1:3] == clock.groups()[3:5]
            or not 13 < anchor or not later or int(clock[8]) != rebased_tick):
        raise ValueError("Counter reload observation disagrees with separate alarm identity")
    first, end, start, stop, count, excluded, small_first, small_end = map(int, plan.groups())
    width = (hz + 47999) // 48000
    if (first != anchor - 13 or end != first + width or not first < reload_tick < end
            or (start, stop, count, excluded) != (first, anchor - 1, 13, 1)
            or (small_first, small_end) != (anchor - 5, anchor - 5 + width)
            or not small_first < reload_tick < small_end or stop - small_first > 6
            or stop - start <= 6):
        raise ValueError("The aperture or the undersized refusal changes the original frame")
    arm, lead, delay = stable(issue[1]), int(issue[2]), int(issue[3])
    pre, post = stable(issue[5]), stable(issue[6])
    if (lead != count + 128 or not lead < arm <= anchor or delay != arm - lead
            or not 1 <= int(issue[4]) <= 64 or not excluded < post < pre <= count or pre <= excluded + 6):
        raise ValueError("IRQ arming or direct countdown bounds disagree")
    observed = [anchor - pre, anchor - post]
    if observed != [int(issue[7]), int(issue[8])] or not start <= observed[0] < observed[1] < stop:
        raise ValueError("The observed store exceeded the selected part of its original window")
    return {"counter_hz": hz, "original_frame": [first, end], "reload_tick": reload_tick,
            "later_observed_tick": rebased_tick, "aperture": [start, stop], "observed_ticks": observed,
            "insufficient_aperture_refused": [small_first, small_end], "closed": True}


if __name__ == "__main__":
    try:
        result = replay(Path(sys.argv[1]).read_text(encoding="ascii"))
    except ValueError as error:
        print(error, file=sys.stderr)
        sys.exit(1)
    print(json.dumps(result, indent=2))
