"""Replay a tracker-timed Paula write with measured card/MIDI RAM stores."""

import argparse
import json
from pathlib import Path
import re

if __package__:
    from .replay_routed_paula_window import ROW, stable
    from .replay_tracker_paula_alarm import replay as replay_tracker
else:
    from replay_routed_paula_window import ROW, stable
    from replay_tracker_paula_alarm import replay as replay_tracker


RAM = re.compile(
    r"Mixed IRQ RAM (\d) bank=([\da-f]{4})/([\da-f]{4}) control=([\da-f]{4})/([\da-f]{4}) "
    r"midi=([\da-f]{8})/([\da-f]{8})"
)
EXPECTED = ((0, 0x8000, 0x01953C5A), (0, 0, 0x01853C00))
UNWRITTEN = (0xFFFF, 0xFFFF, 0xFFFFFFFF)


def observe(text):
    lines = text.splitlines()
    ram = [RAM.fullmatch(line) for line in lines if line.startswith("Mixed IRQ RAM ")]
    windows = [ROW.fullmatch(line) for line in lines if line.startswith("Paula window ")]
    if not 1 <= len(ram) <= 2 or len(ram) != len(windows) or not all(ram + windows):
        raise ValueError("Missing paired raw RAM and clock observations")
    if "Paula tracker epoch=8192 start=0 stop=1920 paula=4 amigus=1 midi=1 f00=1" not in lines:
        raise ValueError("Original tracker Start and Stop forecast is missing")
    observations = []
    for index, (memory, timing) in enumerate(zip(ram, windows)):
        row = timing.groupdict()
        if int(memory[1]) != index or int(row["index"]) != index or int(row["stage"]) != (3, 6)[index]:
            raise ValueError("Changed issue sequence")
        prepared = tuple(int(memory[column], 16) for column in (2, 4, 6))
        readback = tuple(int(memory[column], 16) for column in (3, 5, 7))
        if prepared != EXPECTED[index]:
            raise ValueError("Original bank, voice control or MIDI bytes differ")
        hz = int(row["hz"])
        frame = (0, 1920)[index]
        if hz not in (709379, 715909):
            raise ValueError("Unrecognized counter rate")
        first, excluded = (8192 + (edge * hz + 47999) // 48000 for edge in (frame, frame + 1))
        anchor = stable(row["anchor"])
        if (anchor - first, anchor - excluded) != (int(row["first"]), int(row["end"])):
            raise ValueError("Changed original frame bounds")
        observed = None
        if row["status"] == "1" and row["marker"] == "4752" and readback == prepared:
            verdict = "issued_unbracketed"
            try:
                observed = [anchor - stable(row["before"]), anchor - stable(row["after"])]
            except ValueError:
                pass
            if observed:
                if not first <= observed[0] < excluded or observed[1] < observed[0]:
                    raise ValueError("Observed write began outside its exact window")
                verdict = "issued_in_frame" if observed[1] < excluded else "issued_outside"
        elif row["marker"] == "0000" and readback == UNWRITTEN:
            verdict = "no_ram_stores_observed"
        else:
            verdict = "uncertain"
        if row["in_frame"] == "1" and verdict != "issued_in_frame":
            raise ValueError("Claimed timing contradicts raw results")
        observations.append({"frame": frame, "window": [first, excluded], "observed": observed,
                             "card_bank_control_and_midi_ram": list(readback), "verdict": verdict})
    return observations


def replay(text):
    observed = observe(text)
    result = replay_tracker(text)
    if len(observed) != 2 or any(row["verdict"] != "issued_in_frame" for row in observed):
        raise ValueError("Mixed RAM effects exceed the original activation window")
    result["mixed_ram_observations"] = observed
    return result


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("log", type=Path)
    parser.add_argument("--partial", action="store_true")
    args = parser.parse_args()
    try:
        text = args.log.read_text(encoding="ascii")
        result = observe(text) if args.partial else replay(text)
    except ValueError as error:
        parser.exit(1, f"{error}\n")
    print(json.dumps(result, indent=2))
