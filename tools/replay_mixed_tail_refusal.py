"""Verify that a mixed plan with an undersized F00 aperture never arms."""

import json
from pathlib import Path
import re
import sys

if __package__:
    from .replay_routed_paula_alarm import DEVICE, stable
else:
    from replay_routed_paula_alarm import DEVICE, stable


FORECAST = re.compile(r"Paula tracker epoch=(\d+) start=0 stop=3840 paula=4 amigus=1 midi=1 f00=1")
REFUSAL = re.compile(r"Paula tail refusal hz=(\d+) anchor=([0-9a-f]{6}) frame=(\d+)/(\d+) min_span=7 refused=1")
ALARM = re.compile(r"Paula alarm closed=1 chip=[01] timer=[01] calls=0 stray=0")


def replay(text):
    lines = text.splitlines()
    if lines[-2:] != ["Paula windows closed stray=0 rows=0", "Paula trace finished stage=3 rc=0 owned-closed=1"]:
        raise ValueError("Missing deliberate pre-arm refusal or original owned closure")
    forecast = [FORECAST.fullmatch(line) for line in lines if line.startswith("Paula tracker ")]
    refused = [REFUSAL.fullmatch(line) for line in lines if line.startswith("Paula tail refusal ")]
    alarm = [line for line in lines if line.startswith("Paula alarm ")]
    if (len(forecast) != 1 or not forecast[0] or len(refused) != 1 or not refused[0]
            or len(alarm) != 1 or not ALARM.fullmatch(alarm[0])
            or [line for line in lines if line.startswith("Paula reload ")] != ["Paula reload clock callbacks=0"]
            or any(line.startswith(("Paula window ", "Paula wait ", "Mixed IRQ RAM ")) for line in lines)):
        raise ValueError("Original forecast, zero callbacks or zero writes not verified")
    hz, raw, first, end = refused[0].groups()
    hz, first, end = int(hz), int(first), int(end)
    anchor, epoch = stable(raw), int(forecast[0][1])
    reload_tick = anchor + 1
    pre_ticks, post_ticks = anchor - 1 - first, end - reload_tick
    if (hz not in (709379, 715909) or not epoch or first != epoch + (3840 * hz + 47999) // 48000
            or end != epoch + (3841 * hz + 47999) // 48000 or first != anchor - 7
            or not 0 < pre_ticks <= 7 or not 0 < post_ticks <= 7):
        raise ValueError("The original F00 frame lacks an independently checkable short aperture")
    observed = {}
    for line in lines:
        if not line.startswith("Paula trace ") or line.startswith("Paula trace finished "):
            continue
        row = DEVICE.fullmatch(line)
        if not row:
            raise ValueError("Missing original Paula ownership")
        stage, point, dma, request, enabled, writes, register, _ = row.groups()
        key = (int(stage), point)
        if key in observed or int(dma, 16) & 0x20F != 0x200 or int(register, 16) == 0x96:
            raise ValueError("Paula DMA changed in the refused plan")
        observed[key] = (int(request, 16) & 0x780, int(enabled, 16) & 0x780, int(writes))
    initial = observed.get((2, "initial-owned-state"))
    restored = observed.get((7, "interrupt-mask-restored"))
    if (not initial or not restored or initial[1] != restored[1]
            or observed.get((3, "after-request-wait"), (None,))[0] != 0):
        raise ValueError("Missing zero fresh requests or original interrupt mask restoration")
    return {"counter_hz": hz, "f00_frame": 3840, "original_window": [first, end], "reload_tick": reload_tick,
            "available_pre_and_post_ticks": [pre_ticks, post_ticks], "required_bracket_ticks": 7,
            "alarm_callbacks": 0, "owned_dma_stayed_clear": True, "guest_closed": True}


if __name__ == "__main__":
    try:
        result = replay(Path(sys.argv[1]).read_text(encoding="ascii"))
    except ValueError as error:
        print(error, file=sys.stderr)
        sys.exit(1)
    print(json.dumps(result, indent=2))
