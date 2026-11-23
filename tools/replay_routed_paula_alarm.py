"""Check both original Paula IRQ windows against a single measured countdown."""

import json
from pathlib import Path
import re
import sys

if __package__:
    from .replay_routed_paula_window import ROW, replay_windows, stable
else:
    from replay_routed_paula_window import ROW, replay_windows, stable


ALARM = re.compile(r"Paula alarm (\d+) sample=([\da-f]{6}) lead=(\d+) delay=(\d+) calls=(\d+)")
CLOSED = re.compile(r"Paula alarm closed=1 chip=([01]) timer=([01]) calls=2 stray=0")
DEVICE = re.compile(
    r"Paula trace \d+ stage=(\d+) point=([a-z-]+) dma=([\da-f]{4}) req=([\da-f]{4}) "
    r"ena=([\da-f]{4}) adk=[\da-f]{4} owned=1 writes=(\d+) reg=([\da-f]{4}) value=([\da-f]{4})"
)


def replay_device(lines):
    expected = {(2, "initial-owned-state"): (0, 0), (2, "after-request-wait"): (0, 0),
                (3, "after-registers"): (15, None), (3, "after-start-fetches"): (15, 0x780),
                (6, "after-registers"): (0, None), (6, "after-request-wait"): (0, 0),
                (7, "interrupt-mask-restored"): (0, 0)}
    trace = {}
    for line in lines:
        if not line.startswith("Paula trace ") or line.startswith("Paula trace finished "):
            continue
        match = DEVICE.fullmatch(line)
        if not match:
            raise ValueError("Device trace lacks an original owned observation")
        stage, point, dma, request, enabled, writes, register, value = match.groups()
        key = (int(stage), point)
        if key in trace:
            raise ValueError("Device trace repeats an observation")
        trace[key] = (int(dma, 16) & 15, int(request, 16) & 0x780, int(enabled, 16) & 0x780,
                      int(writes), int(register, 16), int(value, 16))
    for key, (expected_dma, expected_request) in expected.items():
        row = trace.get(key)
        if not row or row[0] != expected_dma or (expected_request is not None and row[1] != expected_request):
            raise ValueError("Original DMA or fresh audio request was not observed")
    if (trace[3, "after-registers"][3:] != (1, 0x96, 0x800F) or
            trace[6, "after-registers"][3:] != (1, 0x96, 0x000F) or
            trace[2, "initial-owned-state"][2] != trace[7, "interrupt-mask-restored"][2]):
        raise ValueError("Owned device command or original interrupt mask changed")


def replay_alarm_pair(text, start_frame, stop_frame, epoch=0, anchor_ticks=(0, 0)):
    evidence = replay_windows(text, ((3, start_frame, 0x800F, 0x200, 0x20F),
                                     (6, stop_frame, 0x000F, 0x20F, 0x200)), epoch, anchor_ticks)
    lines = text.splitlines()
    windows = [ROW.fullmatch(line).groupdict() for line in lines if line.startswith("Paula window ")]
    clocks = {(row["hz"], row["chip"], row["timer"]) for row in windows}
    if len(clocks) != 1 or (anchor_ticks[1] == 0 and windows[1]["anchor"] != windows[0]["anchor"]):
        raise ValueError("Start and Stop do not share the original clock")
    _, chip, timer = clocks.pop()
    anchor_raw = windows[0]["anchor"]
    closed = [line for line in lines if line.startswith("Paula alarm closed")]
    alarm = CLOSED.fullmatch(closed[0]) if len(closed) == 1 else None
    if not alarm or alarm.groups() == (chip, timer):
        raise ValueError("Separate IRQ timer ownership and closure are missing")
    arms = [line for line in lines if re.match(r"Paula alarm \d", line)]
    if len(arms) != 2:
        raise ValueError("Missing original IRQ arming observations")
    previous = stable(anchor_raw)
    for index, line in enumerate(arms):
        match = ALARM.fullmatch(line)
        if not match:
            raise ValueError("Malformed arm sample")
        i, raw, lead, delay, calls = match.groups()
        count = stable(raw)
        if index and anchor_ticks[index] != anchor_ticks[index - 1]:
            previous = stable(windows[index]["anchor"])
        if (int(i) != index or int(calls) != index + 1 or
                int(lead) != int(windows[index]["first"]) + 128 or
                not int(lead) < count < previous or int(delay) != count - int(lead)):
            raise ValueError("Wrong original threshold, descending count or IRQ receipt")
        previous = stable(windows[index]["after"])
    replay_device(lines)
    evidence["clock_anchor_count"] = stable(anchor_raw)
    evidence["alarm"] = {"chip": int(alarm[1]), "timer": int(alarm[2]), "callbacks": 2, "closed": True}
    evidence["device_guest_closed"] = True
    return evidence


def replay(text):
    return replay_alarm_pair(text, 512, 1280)

if __name__ == "__main__":
    try:
        result = replay(Path(sys.argv[1]).read_text(encoding="ascii"))
    except ValueError as error:
        print(error, file=sys.stderr)
        sys.exit(1)
    print(json.dumps(result, indent=2))
