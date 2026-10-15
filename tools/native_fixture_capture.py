"""Pure capture contract for a finite AmigaDOS native fixture.

Shell and native stdout may buffer independently. Keep native output in an
exclusive run-owned RAM file and validate it separately from Shell controls.
This module performs no target, process, filesystem or ownership operation.
"""
import re
import zlib

MAX_OUTPUT = 8192


def require(ok, message):
    if not ok:
        raise ValueError(message)


def run_script(remote, output, stack, prefix, *, control=None):
    """Build one same-Shell invocation, refusing an existing output file."""
    path = r"RAM:[A-Za-z0-9_-]+/[A-Za-z0-9_.-]+"
    require(isinstance(remote, str) and re.fullmatch(path, remote), "Invalid executable path")
    require(isinstance(output, str) and re.fullmatch(path, output), "Invalid capture path")
    require(remote.rsplit("/", 1)[0] == output.rsplit("/", 1)[0]
            and remote.lower() != output.lower(), "Capture must be distinct in the owned RAM directory")
    require(type(stack) is int and 0 < stack <= 0x7fffffff, "Invalid Stack size")
    require(isinstance(prefix, str) and re.fullmatch(r"[A-Za-z0-9_-]+-", prefix), "Invalid marker prefix")
    script = (f"If EXISTS {output}\n Echo {prefix}CAPTURE-EXISTS\n Quit 20\nEndIf\n"
            f"Echo {prefix}BEFORE\nStack {stack}\nIf WARN\n Echo {prefix}STACK-SET-WARN\n Quit 20\nEndIf\n"
            f"Echo {prefix}STACK-QUERY-BEGIN\nStack\nIf WARN\n Echo {prefix}STACK-QUERY-WARN\n Quit 20\nEndIf\n"
            f"Echo {prefix}STACK-QUERY-END\nEcho {prefix}NATIVE-BEGIN\n{remote} >{output}\n"
            f"Echo {prefix}NATIVE-RC $RC\nEcho {prefix}NATIVE-END\n")
    if control is None:
        return script
    require(isinstance(control, str) and re.fullmatch(path, control)
            and control.rsplit('/', 1)[0] == remote.rsplit('/', 1)[0]
            and control.lower() not in (remote.lower(), output.lower()),
            "Distinct control capture in the owned RAM directory required")
    # Execute's inherited output may differ from the invoking command's handle.
    # Redirect each control-producing command itself. Each short-lived command
    # closes its file before the script can publish the terminal flag.
    lines = []
    for text in script.splitlines():
        bare = text.strip()
        if bare.startswith('Echo '):
            first = bare in (f'Echo {prefix}BEFORE', f'Echo {prefix}CAPTURE-EXISTS')
            text += (' >' if first else ' >>') + control
        elif bare == 'Stack':
            text += ' >>' + control
        lines.append(text)
    return f"If EXISTS {control}\n Quit 20\nEndIf\n" + '\n'.join(lines) + '\n'


def parse_control(raw, stack, prefix):
    """Accept only complete Shell controls, exact RC0 and one bare Stack query."""
    require(isinstance(raw, str) and len(raw) <= MAX_OUTPUT, "Control output exceeds bound")
    lines = raw.splitlines()
    fixed = ["[OK]", prefix + "BEFORE", prefix + "STACK-QUERY-BEGIN",
             None, prefix + "STACK-QUERY-END", prefix + "NATIVE-BEGIN",
             prefix + "NATIVE-RC 0", prefix + "NATIVE-END"]
    require(len(lines) == len(fixed), "Unexpected or incomplete Shell controls")
    require(all(want is None or got == want for want, got in zip(fixed, lines)),
            "Shell controls are missing, duplicated or out of order")
    numbers = re.findall(r"\b\d+\b", lines[3])
    require(len(numbers) == 1 and int(numbers[0]) == stack, "Bare Stack query differs")
    return {"rawStackQuery": lines[3], "observedStackBytes": stack, "returnCode": 0}


def parse_checksum(raw, path):
    """Bind a bounded nonempty capture to the exact requested guest pathname."""
    require(isinstance(raw, str) and len(raw) <= 4096, "Checksum response exceeds bound")
    lines = raw.splitlines()
    require(len(lines) == 3 and lines[0] == "File: " + path, "Checksum path or fields differ")
    crc = re.fullmatch(r"CRC32: ([0-9A-Fa-f]{8})", lines[1])
    size = re.fullmatch(r"Size: ([0-9]+) bytes", lines[2])
    require(crc and size, "Malformed checksum response")
    length = int(size[1])
    require(0 < length <= MAX_OUTPUT, "Native output is empty or exceeds bound")
    return {"crc32": int(crc[1], 16), "size": length}


def decode_read(raw, offset, count):
    """Decode the installed READFILE hex-dump format, including its ASCII column."""
    require(type(offset) is int and type(count) is int and 0 <= offset < MAX_OUTPUT
            and 0 < count <= 4096 and offset + count <= MAX_OUTPUT, "Invalid bounded read")
    require(isinstance(raw, str) and len(raw) <= 24000, "Read response exceeds bound")
    lines = raw.splitlines()
    require(len(lines) == (count + 15) // 16, "Read response is incomplete or oversized")
    decoded = bytearray()
    for line in lines:
        width = min(16, count - len(decoded))
        require(len(line) == 60 + width and line[:10] == f"{offset + len(decoded):08x}  "
                and line[58:60] == "  ", "Read address or columns differ")
        field = line[10:58]
        tokens = field.rstrip().split(" ")
        require(len(tokens) == width and all(re.fullmatch(r"[0-9a-fA-F]{2}", x) for x in tokens),
                "Malformed read bytes")
        data = bytes(int(x, 16) for x in tokens)
        require(field == " ".join(tokens).ljust(48), "Read byte spacing differs")
        require(line[60:] == "".join(chr(x) if 32 <= x < 127 else "." for x in data),
                "Read ASCII column differs")
        decoded.extend(data)
    return bytes(decoded)


def validate_native(data, checksum, markers):
    require(isinstance(data, bytes) and 0 < len(data) <= MAX_OUTPUT, "Native output exceeds bound")
    require(len(data) == checksum["size"] and zlib.crc32(data) == checksum["crc32"],
            "Captured byte count or CRC differs")
    require(markers and len(set(markers)) == len(markers)
            and all(isinstance(x, str) and x and "\n" not in x and "\r" not in x for x in markers),
            "Expected markers are not unique lines")
    try:
        text = data.decode("ascii")
    except UnicodeDecodeError as exc:
        raise ValueError("Native capture is not ASCII") from exc
    require(text.splitlines() == list(markers), "Native markers are missing, duplicated, reordered or unexpected")
    return {"nativeMarkers": list(markers), "outputBytes": len(data), "outputCRC32": f"{zlib.crc32(data):08X}"}
