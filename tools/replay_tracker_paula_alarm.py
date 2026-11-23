"""Check actual Paula writes at the prepared tracker's Start and F00 frames."""

import json
from pathlib import Path
import sys

if __package__:
    from .replay_routed_paula_alarm import replay_alarm_pair
else:
    from replay_routed_paula_alarm import replay_alarm_pair


def replay(text):
    expected = "Paula tracker epoch=8192 start=0 stop=1920 paula=4 amigus=1 midi=1 f00=1"
    headers = [line for line in text.splitlines() if line.startswith("Paula tracker ")]
    if headers != [expected]:
        raise ValueError("Missing the original mixed forecast or Start/F00 frames")
    result = replay_alarm_pair(text, 0, 1920, epoch=8192)
    result["song_epoch_ticks"] = 8192
    result["forecast_start_frame"] = 0
    result["forecast_f00_frame"] = 1920
    result["forecast_actions"] = {"paula": 4, "amigus": 1, "midi": 1}
    return result


if __name__ == "__main__":
    try:
        result = replay(Path(sys.argv[1]).read_text(encoding="ascii"))
    except ValueError as error:
        print(error, file=sys.stderr)
        sys.exit(1)
    print(json.dumps(result, indent=2))
