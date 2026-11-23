"""Replay the finite native CIA timer observation using its raw triplets."""
import argparse
import json
from pathlib import Path
import re


PAIR_COUNT = 48
COUNTDOWN = 65535
RATE = 48000
HEADER = re.compile(
    r"CIA direct h=(\d+) end_h=(\d+) first=([\da-f]{8}):([\da-f]{8}) "
    r"last=([\da-f]{8}):([\da-f]{8}) chip=([01]) timer=([01]) captured=48 bound=1"
)
PAIR = re.compile(r"CIA pair (\d{2}) ([\da-f]{6}) ([\da-f]{6}) 2452")
SUMMARY = re.compile(r"CIA direct observed verified=(\d+) invalid=(\d+) stray=0 closed=1 rc=0")


def stable_count(text):
    high, low, repeated = bytes.fromhex(text)
    return high * 256 + low if high == repeated else 0


def replay(text):
    lines = text.splitlines()
    if len(lines) != PAIR_COUNT + 2:
        raise ValueError("Incomplete direct clock report")
    header = HEADER.fullmatch(lines[0])
    summary = SUMMARY.fullmatch(lines[-1])
    if not header or not summary:
        raise ValueError("Missing independently bounded and closed observation")
    hz, end_hz = map(int, header.group(1, 2))
    before = int(header.group(3) + header.group(4), 16)
    after = int(header.group(5) + header.group(6), 16)
    elapsed = after - before
    if hz not in (709379, 715909) or hz != end_hz or not 0 < elapsed < COUNTDOWN:
        raise ValueError("Outer E-clock absent, changed or outside the one-shot horizon")

    previous = COUNTDOWN
    intervals = []
    invalid = 0
    for number, line in enumerate(lines[1:-1]):
        row = PAIR.fullmatch(line)
        if not row or int(row.group(1)) != number:
            raise ValueError("Invalid pair order, structure or ordinary-RAM marker")
        first, last = map(stable_count, row.group(2, 3))
        if not first or not last:
            invalid += 1
            continue
        if last > first or first > previous:
            raise ValueError("Stable count moved backwards or wrapped")
        intervals.append(first - last)
        previous = last

    if (len(intervals) < PAIR_COUNT // 2 or (len(intervals), invalid) != tuple(map(int, summary.groups()))
            or COUNTDOWN - previous > elapsed):
        raise ValueError("Raw pairs contradict reported totals or outer elapsed bound")
    floor, remainder = divmod(hz, RATE)
    return {
        "pairs_captured": PAIR_COUNT,
        "stable_pairs": len(intervals),
        "torn_or_zero": invalid,
        "outer_elapsed_ticks": elapsed,
        "counter_hz": hz,
        "chip": int(header.group(7)),
        "timer": int(header.group(8)),
        "min_ticks": min(intervals),
        "max_ticks": max(intervals),
        "frame_ticks": [floor, floor + bool(remainder)],
        "shorter_than_minimum_frame_width": sum(span < floor for span in intervals),
        "at_least_maximum_frame_width": sum(span >= floor + bool(remainder) for span in intervals),
        "closed": True,
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
