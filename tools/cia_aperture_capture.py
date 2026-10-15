"""Bounded saved-output observer for the existing CIA aperture experiment.

This is a diagnostic collection contract, not an expected-failure test. RC20
remains a failed qualification even when the exact report and positive resource
closure permit normal custody/settlement. No transport, target or file operation
is performed here. Ordinary native fixture acceptance remains exact RC0/output.
"""
import re
import zlib

PROFILE = "cia-aperture-v1"
MAX_OUTPUT = 32768
MODEL = ("CIA APERTURE MODEL PASS: original windows, bounded actual reads, exact keys "
         "and irreversible postcommit failure; software diagnostic only")
PASS = ("CIA APERTURE PASS: original-frame actual RAM commit brackets and zero-effect "
        "negative gates; timer/software diagnostic only, no DMA or audio proof")
MARKERS = (MODEL, PASS)
SHADOW = 0x41504552
WIDE = frozenset("first last arm_target arm_observed arm_after entry before after "
                 "last_observed last_read aperture_span dispatch_before dispatch_after "
                 "dispatch_span".split())
PAIRS = {"arm_frequency": 2, "valid": 3, "dispatch_frequency": 2}
FIELDS = frozenset(("index mode phase prepared arm_valid arm_after_valid delivered closed "
                    "result arm_read_valid arm_reads count last_read_frequency last_read_valid "
                    "reads commits shadow aperture_span_valid dispatch_span_valid wake calls").split()) | WIDE | PAIRS.keys()
FLAGS = frozenset("prepared arm_valid arm_after_valid delivered closed arm_read_valid "
                  "last_read_valid aperture_span_valid dispatch_span_valid".split())
CLOSED = frozenset("acquired chip bit vector_removed timer_stopped task_identity priority "
                   "unchanged signals_restored attempted completed positive negative failures "
                   "frequency early_ticks residency_ticks reads_cap".split())


def require(value, message):
    if not value:
        raise ValueError(message)


def decimal(text, maximum=0xffffffff):
    require(re.fullmatch(r"0|[1-9][0-9]{0,9}", text) is not None, "Invalid bounded decimal")
    value = int(text)
    require(value <= maximum, "Decimal exceeds native width")
    return value


def fields(line, prefix, keys):
    require(line.startswith(prefix), "Diagnostic record prefix differs")
    parts = line[len(prefix):].split(" ")
    require(len(parts) == len(keys), "Diagnostic field count differs")
    result = {}
    for part in parts:
        pair = part.split("=")
        require(len(pair) == 2 and pair[0] in keys and pair[0] not in result,
                "Unknown, duplicate or malformed diagnostic field")
        result[pair[0]] = pair[1]
    require(result.keys() == keys, "Diagnostic fields missing")
    return result


def observation(line, index):
    raw = fields(line, "CIA APERTURE OBS ", FIELDS)
    row = {}
    for name, value in raw.items():
        if name in WIDE:
            words = value.split(":")
            require(len(words) == 2, "Invalid native counter words")
            row[name] = (decimal(words[0]) << 32) | decimal(words[1])
        elif name in PAIRS:
            words = value.split("/")
            require(len(words) == PAIRS[name], "Invalid diagnostic observation tuple")
            row[name] = tuple(decimal(word) for word in words)
        else:
            row[name] = decimal(value)
    require(row["index"] == index and row["mode"] == (0 if index < 12 else index - 11),
            "Original diagnostic order/mode differs")
    require(all(row[name] <= 1 for name in FLAGS) and all(x <= 1 for x in row["valid"]),
            "Observation validity is not a flag")
    require(1 <= row["phase"] <= 6 and row["prepared"] == row["closed"] == 1
            and row["result"] <= 11 and row["count"] <= 65535
            and row["arm_reads"] <= 1 and row["reads"] <= 64
            and row["commits"] <= 1 and row["shadow"] in (0, SHADOW),
            "Observation exceeds the fixed experiment policy")
    require(row["shadow"] == (SHADOW if row["commits"] else 0), "Commit/shadow disagree")
    if row["arm_valid"]:
        require(row["arm_read_valid"] == row["arm_reads"] == 1
                and row["arm_target"] >= row["arm_observed"]
                and row["arm_target"] - row["arm_observed"] == row["count"],
                "Original timer arm count differs")
    return row


def controls(raw, stack, prefix):
    """Observe an exact RC0 or RC20 Shell completion without changing that RC."""
    require(isinstance(raw, str) and len(raw) <= 8192, "Control output exceeds bound")
    lines = raw.splitlines()
    fixed = ["[OK]", prefix + "BEFORE", prefix + "STACK-QUERY-BEGIN", None,
             prefix + "STACK-QUERY-END", prefix + "NATIVE-BEGIN", None, prefix + "NATIVE-END"]
    require(len(lines) == len(fixed) and all(want is None or got == want
            for want, got in zip(fixed, lines)), "Incomplete or reordered diagnostic controls")
    require(lines[6] in (prefix + "NATIVE-RC 0", prefix + "NATIVE-RC 20"),
            "Unexpected diagnostic return code")
    numbers = re.findall(r"\b\d+\b", lines[3])
    require(len(numbers) == 1 and int(numbers[0]) == stack, "Bare Stack query differs")
    return {"rawStackQuery": lines[3], "observedStackBytes": stack,
            "returnCode": int(lines[6].rsplit(" ", 1)[1])}


def validate(data, return_code):
    """Keep raw observations; success here establishes report consistency only.

    All absolute windows are checked against one original epoch, never shifted
    to fit observed clocks. A partial RC20 report is retained as FAILED, without
    guessing missing measurements or treating a restored resource as timing.
    """
    require(isinstance(data, bytes) and 0 < len(data) <= MAX_OUTPUT
            and type(return_code) is int and return_code in (0, 20), "Invalid diagnostic capture")
    try:
        text = data.decode("ascii")
    except UnicodeDecodeError as error:
        raise ValueError("Diagnostic capture is not ASCII") from error
    require(text.endswith("\n") and "\r" not in text and "\x00" not in text,
            "Incomplete or noncanonical diagnostic lines")
    lines = text.splitlines()
    require(5 <= len(lines) <= 23 and lines[0] == MODEL, "Missing original model completion")
    memory = re.fullmatch(r"EXEC MEMORY pool_limit=(0|[1-9][0-9]{0,9}) reserve=262144 flags=4", lines[1])
    require(memory is not None and 0 < decimal(memory[1]), "Fast pool declaration differs")
    position, failed_check = 2, None
    if lines[position].startswith("CIA APERTURE FAIL "):
        failure = fields(lines[position], "CIA APERTURE FAIL ", {"line", "completed"})
        failed_check = {name: decimal(value) for name, value in failure.items()}
        require(0 < failed_check["line"] <= 65535 and failed_check["completed"] <= 16,
                "Invalid failed-check observation")
        position += 1
    rows = []
    while position < len(lines) and lines[position].startswith("CIA APERTURE OBS "):
        require(len(rows) < 16, "Too many original attempts")
        rows.append(observation(lines[position], len(rows)))
        position += 1
    require(position < len(lines), "Positive resource closure absent")
    raw_closed = fields(lines[position], "CIA APERTURE CLOSED ", CLOSED)
    closed = {name: (int(value) if name == "priority" and re.fullmatch(r"-?[0-9]{1,3}", value)
                     else decimal(value)) for name, value in raw_closed.items()}
    require(-128 <= closed["priority"] <= 127, "Invalid task priority")
    require(all(closed[name] == 1 for name in ("acquired", "vector_removed", "timer_stopped",
                "task_identity", "unchanged", "signals_restored"))
            and closed["chip"] <= 1 and closed["bit"] <= 1,
            "Original owned timer/vector/task/signal closure unconfirmed")
    require(1 <= closed["attempted"] == len(rows) <= 16 and 0 <= closed["completed"] <= len(rows)
            and closed["positive"] <= 12 and closed["negative"] <= 4
            and closed["failures"] <= 16
            and closed["frequency"] in (709379, 715909)
            and (closed["early_ticks"], closed["residency_ticks"], closed["reads_cap"]) == (128, 256, 64),
            "Original experiment summary/policy differs")
    require(closed["positive"] + closed["negative"] + closed["failures"] <= closed["completed"],
            "Completed-attempt tally exceeds observations")
    require(failed_check is None or failed_check["completed"] == closed["completed"],
            "Failed-check/closure completion counts differ")
    frequency, epoch = closed["frequency"], None
    positive = negative = 0
    for row in rows:
        frame = (row["index"] + 1) * 1536
        offset = (frame * frequency + 47999) // 48000
        current_epoch = row["first"] - offset
        if epoch is None:
            epoch = current_epoch
        require(epoch >= 0 and current_epoch == epoch
                and row["last"] == epoch + ((frame + 1) * frequency + 47999) // 48000,
                "Original frame window/epoch changed")
        require(row["arm_target"] == (row["last"] + 128 if row["mode"] == 3 else row["first"] - 128),
                "Original early/explicit-late arm target changed")
        aperture_valid = (row["valid"][0] == row["last_read_valid"] == 1
            and row["last_read_frequency"] == frequency and row["last_read"] >= row["entry"])
        # entry_frequency is not separately printed; valid entry alone cannot
        # establish its frequency. A zero reported validity retains that gap.
        if row["aperture_span_valid"]:
            require(aperture_valid and row["aperture_span"] == row["last_read"] - row["entry"],
                    "Reported aperture span differs")
        else:
            require(row["aperture_span"] == 0, "Invalid aperture span has a value")
        dispatch_valid = (row["dispatch_frequency"] == (frequency, frequency)
                          and row["dispatch_after"] >= row["dispatch_before"])
        require(bool(row["dispatch_span_valid"]) == dispatch_valid
                and row["dispatch_span"] == (row["dispatch_after"] - row["dispatch_before"] if dispatch_valid else 0),
                "Reported dispatcher span differs")
        common = (row["phase"] == 6 and row["delivered"] == row["arm_valid"] == row["arm_after_valid"] == 1
                  and row["arm_frequency"] == (frequency, frequency) and row["calls"] == row["index"] + 1
                  and row["wake"] != 0 and row["valid"][0] == 1 and 0 < row["reads"] <= 64
                  and dispatch_valid and row["dispatch_span"] <= 256)
        inside = (row["first"] <= row["before"] <= row["after"] < row["last"])
        if row["mode"] == 0:
            positive += bool(common and row["result"] == 1 and row["commits"] == 1
                             and row["valid"] == (1, 1, 1) and row["reads"] >= 2
                             and row["aperture_span_valid"] == 1 and inside)
        else:
            negative += bool(common and row["result"] == (5 if row["mode"] == 3 else 3)
                             and row["commits"] == 0)
    if failed_check is None:
        require((positive, negative, len(rows) - positive - negative) ==
                (closed["positive"], closed["negative"], closed["failures"]),
                "Original outcome tally disagrees with complete observations")
    position += 1
    require(position < len(lines) and lines[position] ==
            "EXEC MEMORY PASS: 2 Fast allocations, zero owned bytes, budget refusal without Chip fallback",
            "Owned Fast allocation closure absent")
    position += 1
    passed = return_code == 0
    if passed:
        require(failed_check is None and closed["attempted"] == closed["completed"] == 16
                and (positive, negative, closed["failures"]) == (12, 4, 0)
                and position < len(lines) and lines[position] == PASS,
                "RC0 lacks the complete original success gates")
        position += 1
    else:
        require(failed_check is not None or (positive, negative) != (12, 4),
                "RC20 lacks a failed original gate")
    require(position == len(lines), "Unexpected, duplicate or missing diagnostic lines")
    return {"diagnosticProfile": PROFILE, "qualification": "PASSED" if passed else "FAILED",
            "returnCode": return_code, "resourceClosureReported": True,
            "failedCheck": failed_check, "summary": closed,
            "observations": [{name: list(value) if isinstance(value, tuple) else value
                              for name, value in row.items()} for row in rows],
            "originalEpoch": epoch, "outputBytes": len(data), "outputCRC32": f"{zlib.crc32(data):08X}",
            "scope": "Saved timer/RAM diagnostic observations only; no actual resource authentication, output, DMA, WCET or listening acceptance"}
