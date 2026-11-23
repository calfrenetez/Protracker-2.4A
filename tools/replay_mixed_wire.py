"""Replay the tracker Start and F00 with Paula DMA and the serial loopback."""

import argparse
import json
from pathlib import Path
import re

if __package__:
    from .replay_mixed_frame_aperture import EXPECTED, replay as replay_mixed
    from .replay_mixed_frame_wait import replay_waits
    from .replay_routed_paula_window import ROW
else:
    from replay_mixed_frame_aperture import EXPECTED, replay as replay_mixed
    from replay_mixed_frame_wait import replay_waits
    from replay_routed_paula_window import ROW

BYTE_RECEIPT = re.compile(
    r"Mixed UART ([01]) tx=([\da-f]{3})/([\da-f]{3})/([\da-f]{3}) rx=([\da-f]{6}) received=3 idle=1"
)
CLEANUP = re.compile(r"Mixed UART cleanup restored=1 request=(000[01])/\1 uart=(3[\da-f]{3}) period=(\d+)")


def replay(text):
    result = replay_waits(text, replay_mixed(text, wire=True), observations="mixed_wire_observations")
    rows = [BYTE_RECEIPT.fullmatch(line) for line in text.splitlines() if re.match(r"Mixed UART \d", line)]
    cleanup = [CLEANUP.fullmatch(line) for line in text.splitlines() if line.startswith("Mixed UART cleanup")]
    window = next(ROW.fullmatch(line) for line in text.splitlines() if line.startswith("Paula window "))
    period = (int(window["hz"]) * 5 + 15625) // 31250 - 1
    if len(cleanup) != 1 or not cleanup[0] or int(cleanup[0][3]) != period or len(rows) != 2 or not all(rows):
        raise ValueError("Missing MIDI byte receipts or original serial interrupt restoration")
    for i, row in enumerate(rows):
        message = (EXPECTED[i][2] & 0xffffff).to_bytes(3, "big")
        words = [int(value, 16) for value in row.group(2, 3, 4)]
        if int(row[1]) != i or words != [0x100 | byte for byte in message] or bytes.fromhex(row[5]) != message:
            raise ValueError("UART receipt differs from the frozen tracker route")
        result["mixed_wire_observations"][i]["serial_loopback"] = {"first_word": words[0], "receive": row[5]}
    result["serial_request_restored"] = True
    return result


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("log", type=Path)
    args = parser.parse_args()
    try:
        print(json.dumps(replay(args.log.read_text(encoding="ascii")), indent=2))
    except ValueError as error:
        parser.exit(1, f"{error}\n")
