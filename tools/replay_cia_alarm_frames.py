"""Replay the owned CIA wake records with each retained local frame window."""
import argparse
import json
from pathlib import Path
import re

from replay_cia_local_activation import FRAMES, HEADER, ROW, replay as replay_frame, sample


ARM = re.compile(
    r"CIA alarm (\d{2}) probe=(\d+) raw=([\da-f]{6}) wake_at=(\d+) delta=(\d+) wake=([01]) phase=([0-5]) calls=(\d+)"
)
CLOSED = re.compile(r"CIA alarm closed chip=([01]) timer=([01]) calls=(\d+) stray=0 attempted=14")


def replay(text):
    lines = text.splitlines()
    count = len(FRAMES)
    if len(lines) != 2 * count + 3:
        raise ValueError("Incomplete alarm or frame report")
    frames = lines[:count + 2]
    facts = replay_frame("\n".join(frames), allow_before_arm_refusal=True)
    clock = HEADER.fullmatch(lines[0]).group(7, 8)
    closure = CLOSED.fullmatch(lines[-1])
    if not closure or closure.group(1, 2) == clock:
        raise ValueError("Distinct alarm source or its closure is missing")

    anchor = facts["clock_anchor_count"]
    callbacks = 0
    prior_probe = anchor
    arms = []
    for number, line in enumerate(lines[count + 2:-1]):
        match = ARM.fullmatch(line)
        if not match:
            raise ValueError("Missing original IRQ admission record")
        ordinal, probe = map(int, match.group(1, 2))
        raw = match.group(3)
        threshold, delta, wake, phase, calls = map(int, match.group(4, 5, 6, 7, 8))
        point = facts["rows"][number]
        row = ROW.fullmatch(frames[number + 1])
        pre = sample(row.group(9))
        expected_threshold = (anchor - point["first"] + 128 if number < 12
                              else max(0, probe - (1024 if number == 12 else 128)))
        if ordinal != number or threshold != expected_threshold or (sample(raw) or 0) != probe or probe > prior_probe:
            raise ValueError("Alarm count differs from its raw sample or original threshold")
        if probe:
            prior_probe = probe

        if point["verdict"] == "refused_before_arm":
            if (delta or wake or calls != callbacks or row.group(9) != raw or
                    (phase == 1 and probe) or (phase == 2 and not 0 < probe <= threshold) or phase not in (1, 2)):
                raise ValueError("Pre-arm refusal has an IRQ or lacks its original reason")
            verdict = "torn_probe" if phase == 1 else "wake_deadline_too_close"
        else:
            callbacks += 1
            if (phase != 5 or not wake or calls != callbacks or
                    not 0 < threshold < probe or probe - threshold != delta or
                    pre is not None and pre > threshold):
                raise ValueError("IRQ receipt conflicts with its requested early wake")
            verdict = "irq_observed"
        arms.append({"frame": point["frame"], "probe": probe, "wake_at": threshold,
                     "delta": delta, "callback_count": calls, "verdict": verdict})

    if int(closure.group(3)) != callbacks:
        raise ValueError("Alarm closure disagrees with the original callbacks")
    facts["alarm_source"] = {"chip": int(closure.group(1)), "timer": int(closure.group(2)),
                             "callbacks": callbacks, "closed": True, "original_clock_distinct": True}
    facts["arming"] = arms
    return facts


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("log", type=Path)
    params = parser.parse_args()
    try:
        result = replay(params.log.read_text(encoding="ascii"))
    except ValueError as error:
        parser.exit(1, str(error) + "\n")
    print(json.dumps(result, indent=2))
