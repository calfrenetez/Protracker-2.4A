"""Check scheduled first-byte writes and complete MIDI UART loopback receipts."""

import json
from pathlib import Path
import re
import sys

if __package__:
    from .replay_routed_paula_window import stable
else:
    from replay_routed_paula_window import stable


CLOCK = re.compile(r"MIDI wire clock hz=(\d+) timer=([01]/[01]) alarm=([01]/[01]) "
                   r"anchor=([0-9a-f]{6}) epoch=8192 period=(\d+)")
ROW = re.compile(
    r"MIDI wire (?P<index>\d+) frame=(?P<frame>\d+) window=(?P<start>\d+)/(?P<end>\d+) "
    r"bound=(?P<first>\d+)/(?P<excluded>\d+) arm=(?P<arm>[0-9a-f]{6}) delay=(?P<delay>\d+) "
    r"calls=(?P<calls>\d+) status=(?P<status>\d+) attempts=(?P<attempts>\d+) "
    r"pre=(?P<pre>[0-9a-f]{6}) post=(?P<post>[0-9a-f]{6}) mark=(?P<mark>[0-9a-f]{4}) "
    r"tx=(?P<tx>[0-9a-f]{3}/[0-9a-f]{3}/[0-9a-f]{3}) rx=(?P<rx>[0-9a-f]{6}) "
    r"received=(?P<received>[0-3]) idle=(?P<idle>[01])"
)


def replay(text):
    lines = text.splitlines()
    if len(lines) != 5 or lines[-1] != "MIDI wire finished callbacks=3 stray=0/0 closed=1 rc=0":
        raise ValueError("Missing all three native cases and verified closure")
    clock = CLOCK.fullmatch(lines[0])
    if not clock:
        raise ValueError("Missing the original CIA clock")
    hz = int(clock[1])
    anchor = stable(clock[4])
    if hz not in (709379, 715909) or clock[2] == clock[3] or int(clock[5]) != (hz * 5 + 15625) // 31250 - 1:
        raise ValueError("Changed UART period, rate or timer identity")

    notes = []
    for index, (frame, tx) in enumerate(((0, "195/13c/15a"), (1920, "185/13c/100"), (3000, "195/13c/15a"))):
        row = ROW.fullmatch(lines[index + 1])
        if not row:
            raise ValueError("Missing original IRQ or receive observations")
        first, end = (8192 + (edge * hz + 47999) // 48000 for edge in (frame, frame + 1))
        low, high = anchor - first, anchor - end
        if ((int(row["index"]), int(row["frame"]), int(row["start"]), int(row["end"])) != (index, frame, first, end)
                or (int(row["first"]), int(row["excluded"])) != (low, high) or row["tx"] != tx
                or row["idle"] != "1" or int(row["calls"]) != index + 1 or not 1 <= int(row["attempts"]) <= 64):
            raise ValueError("Changed frame, serial words or callback count")
        threshold = high - 128 if index == 2 else low + 128
        arming = stable(row["arm"])
        if not threshold < arming <= anchor or int(row["delay"]) != arming - threshold:
            raise ValueError("Changed original hardware alarm arming")
        before, after = stable(row["pre"]), stable(row["post"])
        if index == 2:
            if not 0 < before <= high or after or (row["status"], row["mark"], row["rx"], row["received"]) != (
                    "2", "0000", "000000", "0"):
                raise ValueError("Expired window did not leave the UART quiet")
        else:
            rx = ("953c5a", "853c00")[index]
            if not high < after < before <= low or (row["status"], row["mark"], row["rx"], row["received"]) != (
                    "1", "4752", rx, "3"):
                raise ValueError("Unqualified first-byte issue or complete MIDI receipt")
            notes.append({"frame": frame, "window": [first, end],
                          "first_write_ticks": [anchor - before, anchor - after], "loopback": rx})
    return {"counter_hz": hz, "uart_period": int(clock[5]), "notes": notes,
            "expired_frame": 3000, "expired_without_send": True, "closed": True}


if __name__ == "__main__":
    try:
        print(json.dumps(replay(Path(sys.argv[1]).read_text(encoding="ascii")), indent=2))
    except ValueError as error:
        print(error, file=sys.stderr)
        sys.exit(1)
