"""Read the private raw DMA and audio request observations."""
import json
from pathlib import Path
import re
import sys


ROW = re.compile(
    r"Paula trace (\d{2}) stage=(\d+) point=([a-z-]+) dma=([\da-f]{4}) req=([\da-f]{4}) "
    r"ena=([\da-f]{4}) adk=([\da-f]{4}) owned=([01]) writes=(\d+) reg=([\da-f]{4}) value=([\da-f]{4})"
)
FINISH = re.compile(r"Paula trace finished stage=([1-6]) rc=(0|20) owned-closed=1")


def replay(text):
    lines = text.splitlines()
    if len(lines) < 2 or len(lines) > 65 or not FINISH.fullmatch(lines[-1]):
        raise ValueError("Missing complete private audio closure or observations")
    rows = []
    for index, line in enumerate(lines[:-1]):
        m = ROW.fullmatch(line)
        if not m or int(m.group(1)) != index or m.group(8) != "1":
            raise ValueError("Malformed observation or loss of original ownership")
        row = {
            "stage": int(m.group(2)), "point": m.group(3),
            **{name: int(m.group(slot), 16) for name, slot in
               (("dma", 4), ("request", 5), ("enable", 6), ("adk", 7), ("register", 10), ("value", 11))},
            "writes": int(m.group(9)),
        }
        if row["stage"] > 7 or rows and rows[-1]["stage"] > row["stage"]:
            raise ValueError("Trace stages moved backwards")
        rows.append(row)
    index = {(row["stage"], row["point"]): row for row in rows}
    if len(index) != len(rows):
        raise ValueError("Duplicated observation")
    initial = index.get((2, "initial-owned-state"))
    final = index.get((7, "interrupt-mask-restored"))
    if not initial or not final or final["dma"] & 15 or final["request"] & 0x780:
        raise ValueError("Owned device closure lacks its final observations")
    after_enable = index.get((3, "after-registers"))
    if not after_enable or (after_enable["writes"], after_enable["register"], after_enable["value"]) != (1, 0x96, 0x800f):
        raise ValueError("Original grouped DMA command was not observed")
    finish = FINISH.fullmatch(lines[-1])
    if finish.group(2) == "0":
        expected_masks = {2: 0, 3: 15, 4: 15, 5: 10, 6: 0}
        for stage, mask in expected_masks.items():
            row = index.get((stage, "after-request-wait"))
            if not row or (row["dma"] & 15) != mask or (row["request"] & 0x780) != (mask << 7):
                raise ValueError("Successful receipt contradicts the exact device decision sample")
        if (final["enable"] & 0x780) != (initial["enable"] & 0x780):
            raise ValueError("Original owned interrupt mask did not match at closure")
    return {
        "closed": True, "exit": int(finish.group(2)), "final_case_stage": int(finish.group(1)),
        "stages_replayed": sorted({row["stage"] for row in rows}),
        "original_grouped_dma_readback": after_enable["dma"],
        "original_grouped_dma_matches": (after_enable["dma"] & 0x20f) == 0x20f,
        "start_request_sequence": {
            point: index[(3, point)]["request"] & 0x780
            for point in ("before-request-clear", "after-request-clear", "after-request-wait")
            if (3, point) in index
        },
        "initial_state": initial, "final_state": final,
        "rows": rows,
    }


if __name__ == "__main__":
    try:
        result = replay(Path(sys.argv[1]).read_text(encoding="ascii"))
    except ValueError as error:
        print(error, file=sys.stderr)
        sys.exit(1)
    print(json.dumps(result, indent=2))
