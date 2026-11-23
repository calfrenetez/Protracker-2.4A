"""Replay the tracker Start and F00 with Paula DMA and the serial loopback."""

import argparse
import json
from pathlib import Path
import re

if __package__:
    from .replay_mixed_frame_aperture import EXPECTED, replay as replay_mixed
    from .replay_mixed_frame_reload import replay as replay_reload
    from .replay_mixed_frame_wait import replay_waits
    from .replay_routed_paula_window import ROW, stable
else:
    from replay_mixed_frame_aperture import EXPECTED, replay as replay_mixed
    from replay_mixed_frame_reload import replay as replay_reload
    from replay_mixed_frame_wait import replay_waits
    from replay_routed_paula_window import ROW, stable

ISSUE_MARGIN_TICKS = 8
READBACK_ISSUE_MARGIN_TICKS = 9
READBACK_SLACK_TICKS = 5000
BYTE_RECEIPT = re.compile(
    r"Mixed UART ([01]) tx=([\da-f]{3})/([\da-f]{3})/([\da-f]{3}) rx=([\da-f]{6}) received=3 idle=1"
)
READBACK_CLOCK = re.compile(r"Mixed UART readback ([01]) samples=([\da-f]{6})/([\da-f]{6})/([\da-f]{6})")
CLEANUP = re.compile(r"Mixed UART cleanup restored=1 request=(000[01])/\1 uart=(3[\da-f]{3}) period=(\d+)")


def replay_readback_clock(lines, windows, result):
    rows = [READBACK_CLOCK.fullmatch(line) for line in lines if line.startswith("Mixed UART readback ")]
    if len(rows) != 2 or not all(rows):
        raise ValueError("Missing coherent clock samples for both serial receipts")
    rebased_tick = result.get("clock_reload", {}).get("origin_sample_tick", 0)
    for i, (row, window) in enumerate(zip(rows, windows)):
        anchor = stable(window["anchor"]) + (rebased_tick if i else 0)
        ticks = [anchor - stable(raw) for raw in row.group(2, 3, 4)]
        wake = result["wait_observations"][i]["before_and_after_wait"][1]
        timing = result["mixed_wire_observations"][i]
        if (int(row[1]) != i or not wake <= ticks[0] < ticks[1] < ticks[2]
                or ticks[2] > timing["window"][1] + READBACK_SLACK_TICKS):
            raise ValueError("Serial readback samples do not follow the owned wake and receive deadline")
        receipt = timing["serial_loopback"]
        receipt["byte_readback_ticks"] = ticks
        receipt["completion_ticks_after_frame_start"] = ticks[-1] - timing["window"][0]


def replay(text, *, require_byte_clock=False, reload=False):
    if reload:
        result = replay_reload(text, wire=True)
    else:
        result = replay_waits(text, replay_mixed(text, wire=True), observations="mixed_wire_observations")
    lines = text.splitlines()
    rows = [BYTE_RECEIPT.fullmatch(line) for line in lines if re.match(r"Mixed UART \d", line)]
    cleanup = [CLEANUP.fullmatch(line) for line in lines if line.startswith("Mixed UART cleanup")]
    with_clock = require_byte_clock or any(line.startswith("Mixed UART readback ") for line in lines)
    margin = READBACK_ISSUE_MARGIN_TICKS if with_clock else ISSUE_MARGIN_TICKS
    windows = [ROW.fullmatch(line) for line in lines if line.startswith("Paula window ")]
    period = (int(windows[0]["hz"]) * 5 + 15625) // 31250 - 1
    if len(cleanup) != 1 or not cleanup[0] or int(cleanup[0][3]) != period or len(rows) != 2 or not all(rows):
        raise ValueError("Missing MIDI byte receipts or original serial interrupt restoration")
    for i, row in enumerate(rows):
        message = (EXPECTED[i][2] & 0xffffff).to_bytes(3, "big")
        words = [int(value, 16) for value in row.group(2, 3, 4)]
        if int(row[1]) != i or words != [0x100 | byte for byte in message] or bytes.fromhex(row[5]) != message:
            raise ValueError("UART receipt differs from the frozen tracker route")
        timing = result["mixed_wire_observations"][i]
        if timing["window"][1] - timing["observed"][0] <= margin:
            raise ValueError("Mixed IRQ began inside the protected end margin")
        timing["serial_loopback"] = {"first_word": words[0], "receive": row[5]}
    if with_clock:
        replay_readback_clock(lines, windows, result)
    result["serial_request_restored"] = True
    return result


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("log", type=Path)
    parser.add_argument("--require-byte-clock", action="store_true")
    parser.add_argument("--reload", action="store_true")
    args = parser.parse_args()
    try:
        result = replay(args.log.read_text(encoding="ascii"), require_byte_clock=args.require_byte_clock, reload=args.reload)
        print(json.dumps(result, indent=2))
    except ValueError as error:
        parser.exit(1, f"{error}\n")
