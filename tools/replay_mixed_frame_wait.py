"""Verify mixed IRQ output from the original alarm while using an Exec wait."""

import argparse
import json
from pathlib import Path
import re

if __package__:
    from .replay_mixed_frame_aperture import observe, replay as replay_mixed
    from .replay_routed_paula_alarm import ALARM
    from .replay_routed_paula_window import ROW, stable
else:
    from replay_mixed_frame_aperture import observe, replay as replay_mixed
    from replay_routed_paula_alarm import ALARM
    from replay_routed_paula_window import ROW, stable


WAIT = re.compile(r"Paula wait (\d) before=([\da-f]{6}) after=([\da-f]{6}) alarm=([01])")


def replay_waits(text, evidence, anchor_ticks=(0, 0), wake_offsets=(0, 0)):
    lines = text.splitlines()
    original_arm = [ALARM.fullmatch(line) for line in lines if re.match(r"Paula alarm \d", line)]
    waits = [WAIT.fullmatch(line) for line in lines if line.startswith("Paula wait ")]
    windows = [ROW.fullmatch(line).groupdict() for line in lines if line.startswith("Paula window ")]
    if len(waits) != 2 or not all(waits):
        raise ValueError("Missing two raw Exec wait observations")
    result = []
    for index, (wait, arm, window) in enumerate(zip(waits, original_arm, windows)):
        entry = stable(wait[2])
        waking = stable(wait[3])
        origin = anchor_ticks[index] + stable(window["anchor"])
        waking_tick = origin + wake_offsets[index] - waking
        if (int(wait[1]) != index or wait[4] != "1" or not int(window["first"]) < entry < stable(arm[2])
                or not waking or waking_tick < origin - stable(window["after"])):
            raise ValueError("The original Exec alarm wait did not span the writes")
        result.append({"frame": evidence["mixed_ram_observations"][index]["frame"],
                       "before_and_after_wait": [origin - entry, waking_tick],
                       "owned_alarm_signal_only": wake_offsets[index] == 0})
    evidence["wait_observations"] = result
    return evidence


def replay(text):
    return replay_waits(text, replay_mixed(text))


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
