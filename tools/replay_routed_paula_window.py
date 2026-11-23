"""Verify raw CIA brackets for the two owned Paula DMA transitions."""

import json
from pathlib import Path
import re
import sys


ROW = re.compile(
    r"Paula window (?P<index>\d+) stage=(?P<stage>\d+) hz=(?P<hz>\d+) chip=(?P<chip>[01]) "
    r"timer=(?P<timer>[01]) anchor=(?P<anchor>[\da-f]{6}) count=(?P<first>\d+)/(?P<end>\d+) "
    r"attempts=(?P<attempts>\d+) status=(?P<status>\d+) "
    r"rawbefore=(?P<before>[\da-f]{6}) rawafter=(?P<after>[\da-f]{6}) mark=(?P<marker>[\da-f]{4}) "
    r"command=(?P<word>[\da-f]{4}) dma=(?P<dma_before>[\da-f]{4})/(?P<dma_after>[\da-f]{4}) "
    r"in_frame=(?P<in_frame>[01])"
)


def stable(raw):
    high, low, high_again = bytes.fromhex(raw)
    if high != high_again:
        raise ValueError("Torn CIA triplet")
    return high * 256 + low


def replay_windows(text, operations):
    lines = text.splitlines()
    if lines[-2:] != ["Paula windows closed stray=0 rows=2",
                       "Paula trace finished stage=6 rc=0 owned-closed=1"]:
        raise ValueError("Missing both timed cases or verified guest closure")
    observed = [line for line in lines if line.startswith("Paula window ")]
    if len(observed) != 2:
        raise ValueError("Unexpected number of Paula window observations")
    rows = []
    for index, line in enumerate(observed):
        match = ROW.fullmatch(line)
        if not match:
            raise ValueError("Malformed raw Paula observation")
        row = match.groupdict()
        stage, frame, command, dma_before, dma_after = operations[index]
        hz = int(row["hz"])
        if (int(row["index"]) != index or int(row["stage"]) != stage or hz not in (709379, 715909)
                or int(row["word"], 16) != command or int(row["status"]) != 1
                or not 1 <= int(row["attempts"]) <= 64 or int(row["marker"], 16) != 0x4752):
            raise ValueError("Original operation, timer or bounded write evidence changed")
        if (int(row["dma_before"], 16) & 0x20F, int(row["dma_after"], 16) & 0x20F) != (dma_before, dma_after):
            raise ValueError("Actual DMA state does not match the grouped operation")
        anchor = stable(row["anchor"])
        first, excluded = (edge * hz + 47999 for edge in (frame, frame + 1))
        first, excluded = first // 48000, excluded // 48000
        if (int(row["first"]), int(row["end"])) != (anchor - first, anchor - excluded):
            raise ValueError("Countdown does not map to the original local sample frame")
        pre = anchor - stable(row["before"])
        post = anchor - stable(row["after"])
        if not (first <= pre < post < excluded and row["in_frame"] == "1"):
            raise ValueError("DMA write is not bounded by an in-frame CIA bracket")
        rows.append({"stage": stage, "clock_hz": hz, "chip": int(row["chip"]), "timer": int(row["timer"]),
                     "command": command, "dma_before": dma_before, "dma_after": dma_after,
                     "frame_ticks": [first, excluded], "observed_ticks": [pre, post]})
    return {"writes": rows, "guest_closed": True}


def replay(text):
    return replay_windows(text, ((3, 512, 0x800F, 0x200, 0x20F), (6, 512, 0x000A, 0x20A, 0x200)))


if __name__ == "__main__":
    try:
        result = replay(Path(sys.argv[1]).read_text(encoding="ascii"))
    except ValueError as error:
        print(error, file=sys.stderr)
        sys.exit(1)
    print(json.dumps(result, indent=2))
