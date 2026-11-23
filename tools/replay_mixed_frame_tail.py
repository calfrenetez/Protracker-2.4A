"""Verify a mixed tracker F00 whose original sample frame spans a CIA reload."""

import json
from pathlib import Path
import re
import sys

if __package__:
    from .replay_mixed_frame_reload import add_mixed_ram
    from .replay_mixed_frame_wait import replay_waits
    from .replay_routed_paula_alarm import ROW, replay_alarm_pair, stable
else:
    from replay_mixed_frame_reload import add_mixed_ram
    from replay_mixed_frame_wait import replay_waits
    from replay_routed_paula_alarm import ROW, replay_alarm_pair, stable


START, STOP = 0, 3840


def replay(text):
    lines = text.splitlines()
    headers = [line for line in lines if line.startswith("Paula tracker ")]
    clock = [ROW.fullmatch(line) for line in lines if line.startswith("Paula window ")]
    forecast = re.fullmatch(r"Paula tracker epoch=(\d+) start=0 stop=3840 paula=4 amigus=1 midi=1 f00=1",
                            headers[0]) if len(headers) == 1 else None
    if not forecast or len(clock) != 2 or not all(clock):
        raise ValueError("Missing original tracker ordinals or exact countdown origin")
    anchor, hz = stable(clock[0]["anchor"]), int(clock[0]["hz"])
    epoch = int(forecast[1])
    first = epoch + (STOP * hz + 47999) // 48000
    end = epoch + ((STOP + 1) * hz + 47999) // 48000
    reload_tick, cutoff = anchor + 1, anchor - 1
    if (not epoch or first != anchor - 13 or not first < reload_tick < end
            or (int(clock[1]["first"]), int(clock[1]["end"])) != (13, 1)
            or stable(clock[1]["before"]) <= 8):
        raise ValueError("F00 did not use a bounded aperture in its original crossing frame")
    reload = [line for line in lines if line.startswith("Paula reload ")]
    if reload != ["Paula reload 0 tick=0 planned=0 issued=0",
                  "Paula reload 1 tick=0 planned=0 issued=0",
                  "Paula reload clock callbacks=1"]:
        raise ValueError("The original generation and later owner signal disagree")
    sampled = [re.fullmatch(r"Paula tail samples (\d) before=([1-4]) after=([1-4])", line)
               for line in lines if line.startswith("Paula tail samples ")]
    if len(sampled) != 2 or not all(sampled) or [int(row[1]) for row in sampled] != [0, 1]:
        raise ValueError("Missing bounded coherent task samples around the wait")
    result = replay_alarm_pair(text, START, STOP, epoch, cutoffs=(None, cutoff))
    result["task_sample_attempts"] = [[int(row[2]), int(row[3])] for row in sampled]
    add_mixed_ram(text, result, (START, STOP))
    result["clock_reload"] = {"reload_tick": reload_tick, "stop_aperture": [first, cutoff]}
    return replay_waits(text, result, wake_offsets=(0, 65536))


if __name__ == "__main__":
    try:
        result = replay(Path(sys.argv[1]).read_text(encoding="ascii"))
    except ValueError as error:
        print(error, file=sys.stderr)
        sys.exit(1)
    print(json.dumps(result, indent=2))
