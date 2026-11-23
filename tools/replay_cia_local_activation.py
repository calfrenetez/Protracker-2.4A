"""Check the original local CIA frame bounds against raw native observations."""
import argparse
import json
from pathlib import Path
import re


FRAMES = tuple(512 + i * 128 for i in range(12)) + (3900, 1)
RATE = 48000
MAX_COUNT = 65535
HEADER = re.compile(
    r"CIA local hz=(\d+) end_hz=(\d+) first=([\da-f]{8}):([\da-f]{8}) "
    r"last=([\da-f]{8}):([\da-f]{8}) chip=([01]) timer=([01]) anchor=([\da-f]{6}) "
    r"captured=14 complete=1"
)
ROW = re.compile(
    r"CIA frame (\d{2}) frame=(\d+) bounds=(\d+)/(\d+) count=(\d+)/(\d+) "
    r"attempts=(\d+) status=([012]) pre=([\da-f]{6}) post=([\da-f]{6}) mark=([\da-f]{4})"
)
SUMMARY = re.compile(r"CIA local finished on=(\d+) outside=(\d+) missed=(\d+) stray=0 closed=1 rc=0")


def sample(text):
    high, low, high_again = bytes.fromhex(text)
    return high * 256 + low if high == high_again else None


def replay(text):
    lines = text.splitlines()
    if len(lines) != len(FRAMES) + 2:
        raise ValueError("Incomplete local activation report")
    header = HEADER.fullmatch(lines[0])
    summary = SUMMARY.fullmatch(lines[-1])
    if not header or not summary:
        raise ValueError("Missing original timer scope or closing observation")

    hz, end_hz = map(int, header.group(1, 2))
    elapsed = int(header.group(5) + header.group(6), 16) - int(header.group(3) + header.group(4), 16)
    anchor = sample(header.group(9))
    if hz not in (709379, 715909) or hz != end_hz or not 0 < elapsed < MAX_COUNT or not anchor:
        raise ValueError("Invalid timer domain, countdown anchor or outer duration")

    previous = anchor
    verified, outside, missed = 0, 0, 0
    rows = []
    for index, (frame, line) in enumerate(zip(FRAMES, lines[1:-1])):
        match = ROW.fullmatch(line)
        if not match:
            raise ValueError("Malformed frame observation")
        ordinal, actual_frame, first, end, first_count, end_count, attempts, status = map(int, match.groups()[:8])
        pre, post = map(sample, match.group(9, 10))
        marker = int(match.group(11), 16)

        expected = ((frame * hz + RATE - 1) // RATE, ((frame + 1) * hz + RATE - 1) // RATE)
        if (ordinal != index or actual_frame != frame or (first, end) != expected
                or (first_count, end_count) != (anchor - first, anchor - end)
                or not 0 < end_count < first_count < anchor or not 1 <= attempts <= 64):
            raise ValueError("Original frame, window or attempt count changed")
        if pre is not None:
            if not 0 < pre <= previous:
                raise ValueError("Original countdown increased or expired")
            previous = pre

        bracket = None
        if status == 1:
            if marker != 0x4752 or pre is None or not end_count < pre <= first_count:
                raise ValueError("Marker has no coherent in-window admission")
            if post is not None:
                if not 0 < post <= pre:
                    raise ValueError("Countdown reversed around a real write")
                previous = post
                bracket = [anchor - pre, anchor - post]
            inside = bracket is not None and bracket[1] < end
            verdict = "inside" if inside else "issued_outside_or_torn"
            verified += inside
            outside += not inside
        else:
            if marker or match.group(10) != "000000":
                raise ValueError("Refusal carries a write or post-write sample")
            if status == 0 and (attempts != 64 or pre is not None and pre <= first_count):
                raise ValueError("Exhausted attempt did not remain early or torn")
            if status == 2 and (pre is None or pre > end_count):
                raise ValueError("Expired attempt has no expired sample")
            verdict = "exhausted" if status == 0 else "expired"
            missed += index < 12

        if index >= 12 and status != (0 if index == 12 else 2):
            raise ValueError("Original deliberate refusal failed")
        rows.append({"frame": frame, "first": first, "excluded": end,
                     "attempts": attempts, "marker": marker, "bracket": bracket, "verdict": verdict})

    if MAX_COUNT - previous > elapsed or tuple(map(int, summary.groups())) != (verified, outside, missed):
        raise ValueError("Raw samples contradict outer duration or native totals")
    return {
        "counter_hz": hz,
        "outer_elapsed_ticks": elapsed,
        "clock_anchor_count": anchor,
        "timed_frames": 12,
        "inside": verified,
        "issued_outside_or_torn": outside,
        "missed_before_write": missed,
        "deliberate_no_write_refusals": 2,
        "rows": rows,
    }


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("log", type=Path)
    args = parser.parse_args()
    try:
        result = replay(args.log.read_text(encoding="ascii"))
    except ValueError as error:
        parser.exit(1, str(error) + "\n")
    print(json.dumps(result, indent=2))
