"""Verify original mixed frame timing across one observed continuous CIA reload."""

import json
from pathlib import Path
import re
import sys

if __package__:
    from .replay_mixed_frame_aperture import EXPECTED, RAM, WIRE
    from .replay_mixed_frame_wait import replay_waits
    from .replay_routed_paula_alarm import replay_alarm_pair
    from .replay_routed_paula_window import ROW, stable
else:
    from replay_mixed_frame_aperture import EXPECTED, RAM, WIRE
    from replay_mixed_frame_wait import replay_waits
    from replay_routed_paula_alarm import replay_alarm_pair
    from replay_routed_paula_window import ROW, stable


RELOAD = re.compile(r"Paula reload (\d) tick=(\d+) planned=(\d+) issued=(\d+)")
EPOCH = 8192
FRAMES = (0, 5760)
MARGIN = 4096


def add_mixed_ram(text, result, frames, *, wire=False):
    pattern = WIRE if wire else RAM
    heading = "Mixed IRQ wire " if wire else "Mixed IRQ RAM "
    ram = [pattern.fullmatch(line) for line in text.splitlines() if line.startswith(heading)]
    if len(ram) != 2 or not all(ram):
        raise ValueError("Incomplete mixed RAM readback")
    observations = []
    result["mixed_wire_observations" if wire else "mixed_ram_observations"] = observations
    for index, (row, timing) in enumerate(zip(ram, result["writes"])):
        prepared = tuple(int(row[col], 16) for col in (2, 4, 6))
        observed = tuple(int(row[col], 16) for col in (3, 5, 7))
        if int(row[1]) != index or prepared != EXPECTED[index] or observed != prepared:
            raise ValueError("Original mixed values and RAM readback disagree")
        observations.append({
            "frame": frames[index], "window": timing["frame_ticks"], "observed": timing["observed_ticks"],
            ("card_ram_and_midi_receive" if wire else "card_bank_control_and_midi_ram"): list(observed),
            "verdict": "issued_in_frame",
        })
    return result


def replay(text, *, wire=False):
    lines = text.splitlines()
    forecast = [line for line in lines if line.startswith("Paula tracker ")]
    if forecast != ["Paula tracker epoch=8192 start=0 stop=5760 paula=4 amigus=1 midi=1 f00=1"]:
        raise ValueError("Original tracker Start and F00 forecast is missing")
    reloads = [RELOAD.fullmatch(line) for line in lines if re.match(r"Paula reload \d", line)]
    windows = [ROW.fullmatch(line) for line in lines if line.startswith("Paula window ")]
    if any(len(rows) != 2 or not all(rows) for rows in (reloads, windows)):
        raise ValueError("Incomplete generation, raw clock or mixed RAM evidence")
    if [line for line in lines if line.startswith("Paula reload clock ")] != ["Paula reload clock callbacks=1"]:
        raise ValueError("Expected one original countdown reload before closure")
    start, later = (stable(row["anchor"]) for row in windows)
    rebased_tick = 65536 + start - later
    offsets = (0, rebased_tick)
    for index, (reload, window) in enumerate(zip(reloads, windows)):
        if tuple(map(int, reload.groups())) != (index, offsets[index], index, index):
            raise ValueError("Original generation and clock sample disagree")
        if not MARGIN < int(window["end"]) < int(window["first"]) < 65535 - MARGIN:
            raise ValueError("The write window reaches the reload guard margin")
    result = replay_alarm_pair(text, *FRAMES, EPOCH, offsets)
    add_mixed_ram(text, result, FRAMES, wire=wire)
    result["clock_reload"] = {"origin_sample_tick": rebased_tick, "original_callbacks": 1}
    return replay_waits(text, result, offsets,
                        observations="mixed_wire_observations" if wire else "mixed_ram_observations")


if __name__ == "__main__":
    try:
        result = replay(Path(sys.argv[1]).read_text(encoding="ascii"))
    except ValueError as error:
        print(error, file=sys.stderr)
        sys.exit(1)
    print(json.dumps(result, indent=2))
