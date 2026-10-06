"""Verify saved public compiler evidence without compiling or running a product.

files.json binds the sanitized public bytes. Calls retain pins of the original
unsanitized logs; those pins are metadata here, not hashes of the public logs.
No source pool, SDK/tool body or HUNK is present for reinspection in this packet.
"""
from pathlib import Path
import hashlib
import json
import math
import posixpath
import re
import shlex
import stat

BASE = Path(__file__).resolve().parent
BASELINE = "c4cbc0419a920e70ed4db1194373a75a6b1a871e"
COMPILER = "$SDK/bin/m68k-amigaos-gcc"
SOURCE_ROOT = "$PRIVATE/outputs/mixed-readers-portable/combined-portable-v3/source/"
PRODUCT = "$PRIVATE/outputs/mixed-readers-portable/combined-portable-v3/native/PTSamplerMixedActivationTest"
FLAGS = ["-std=c99", "-m68000", "-msoft-float", "-mcrt=nix20", "-Os",
         "-Wall", "-Wextra", "-Werror", "-UNDEBUG", "-Isrc/core", "-I.", "-fbbb=-"]
LOOKUP_FLAGS = ["-m68000", "-msoft-float", "-mcrt=nix20"]
ENVIRONMENT = {"PATH": "/usr/bin:/bin", "LC_ALL": "C", "LANG": "C", "TMPDIR": "/private/tmp"}
HELPERS = {
    "cc1": "$SDK/libexec/gcc/m68k-amigaos/6.5.0b/cc1",
    "as": "$SDK/m68k-amigaos/bin/as",
    "ld": "$SDK/m68k-amigaos/bin/ld",
    "collect2": "$SDK/libexec/gcc/m68k-amigaos/6.5.0b/collect2",
}
RUNTIME = {
    "ncrt0.o": "$SDK/m68k-amigaos/libnix/lib/ncrt0.o",
    "libnix20.a": "$SDK/m68k-amigaos/libnix/lib/libnix20.a",
    "libnixmain.a": "$SDK/m68k-amigaos/libnix/lib/libnixmain.a",
    "libnix.a": "$SDK/m68k-amigaos/libnix/lib/libnix.a",
    "libstubs.a": "$SDK/m68k-amigaos/lib/libstubs.a",
    "libamiga.a": "$SDK/m68k-amigaos/lib/libamiga.a",
    "libgcc.a": "$SDK/lib/gcc/m68k-amigaos/6.5.0b/libgcc.a",
}
MARKERS = ["SAMPLER MIXED FACTORY PASS", "SAMPLER MIXED ACTIVATION PASS",
           "MIXED READERS PASS", "amigus reservation lifecycle",
           "WAVETABLE UPLOAD JOB PASS", "AMIGUS WAVETABLE OWNER PASS"]
STDOUT_PIN = {"bytes": 1760, "sha256": "84bebd4a722df63a1a49bbd27a4668b48cfa08f2831c69e7f74178d76989c8c9", "mode": 0o644}
PRODUCT_PIN = {"bytes": 266204, "sha256": "f4c5eeac8cfc6a5ebd4081eceff8ea33691bc2ce3fe5dccfbcb99984f9ca8342", "mode": 0o755}
UNITS = [
    "tests/sampler_mixed_activation_test.c", "src/editor/sampler_mixed_readers.c",
    "src/core/mixed_readers_activation.c", "src/core/mixed_scheduled_readers.c",
    "src/core/elapsed_clock.c", "src/editor/sampler_paula.c", "src/editor/sampler_wavetable.c",
    "src/core/amigus_voice_plan.c", "src/editor/sampler.c", "src/editor/slots.c",
    "src/core/pcm_filtered.c", "src/core/slices.c", "src/core/pattern.c", "src/core/document.c",
    "src/core/pp20.c", "src/core/project.c", "src/core/mod_project.c", "src/core/mod_inspect.c",
    "src/core/channels.c", "src/core/pcm.c", "src/core/wav.c", "src/core/svx.c", "src/core/raw.c",
    "src/core/amigus_reservation.c", "src/core/amigus_wavetable_cache.c",
    "src/core/amigus_sample_ram.c", "src/core/sample_cache.c", "src/core/playback_pcm.c",
]


def require(condition, message):
    if not condition:
        raise ValueError(message)


def read_json(relative):
    return json.loads((BASE / relative).read_text())


def pin(path):
    data = path.read_bytes()
    return {"bytes": len(data), "sha256": hashlib.sha256(data).hexdigest()}


def check_metadata_pin(value, label):
    require(set(value) == {"bytes", "sha256", "mode"}, label + " pin schema")
    require(type(value["bytes"]) is int and value["bytes"] >= 0, label + " bytes")
    require(isinstance(value["sha256"], str) and re.fullmatch(r"[0-9a-f]{64}", value["sha256"]), label + " sha256")
    require(type(value["mode"]) is int and 0 <= value["mode"] <= 0o777, label + " mode")


def normalized_path(value):
    require(isinstance(value, str) and value and "\x00" not in value, "invalid saved path")
    result = posixpath.normpath(value)
    require(result.startswith("$SDK/") or result.startswith(SOURCE_ROOT), "path outside recorded roots: " + value)
    return result


def lookup_stdout(label):
    data = (BASE / "logs" / (label + ".stdout")).read_bytes()
    require(data.endswith(b"\n") and data.count(b"\n") == 1, label + " stdout is not one path line")
    value = data[:-1].decode("utf-8")
    require(value.startswith("$SDK/"), label + " stdout is not a sanitized SDK path")
    return normalized_path(value)


def main():
    inventory = read_json("files.json")
    actual = {}
    for path in sorted(BASE.rglob("*")):
        require(not path.is_symlink(), "symlink in public packet")
        if path.is_file() and path.name != "files.json":
            require(stat.S_ISREG(path.stat().st_mode), "nonregular public file")
            actual[str(path.relative_to(BASE))] = pin(path)
    require(actual == inventory, "public packet byte inventory changed")
    require(not any(name.endswith((".o", ".a", ".c", ".h")) or "native/" in name for name in actual),
            "public packet contains a source pool, SDK body or product")

    receipt = read_json("receipt.json")
    require(receipt["status"] == "PASS_SAVED_COMPILER_LINK_ONLY_NEVER_EXECUTED", "receipt status")
    require(receipt["baseline"] == BASELINE and receipt["profile"] == "combined", "baseline/profile")
    require([receipt[k] for k in ("source_count", "unit_count", "dependency_count", "compiler_call_count")] == [933, 28, 99, 41], "receipt counts")
    require(receipt["flags"] == FLAGS and receipt["ordered_units"] == UNITS, "flags or ordered units")
    require(receipt["bounds"] == {"overall_seconds": 600, "each_compiler_call_seconds": 120,
            "each_git_readback_seconds": 20, "first_failure": "RETAINED_NO_RETRY"}, "fixed recorded bounds")
    overall = receipt["overall_elapsed_seconds"]
    require(isinstance(overall, (int, float)) and math.isfinite(overall) and 0 < overall <= 600, "overall elapsed bound")
    require(receipt["product"] == dict(PRODUCT_PIN, format="HUNK", execution="NEVER_EXECUTED"), "recorded product pin/status")
    require(receipt["expected_stdout"] == STDOUT_PIN and receipt["markers"] == MARKERS, "expected stdout metadata")
    require(all(receipt[k] == "NOT_RUN" for k in ("native_execution", "emulator_execution", "physical_execution")), "execution tier")
    require(receipt["helper_selection_evidence"] == "DRIVER_LOOKUP_ONLY_NOT_HELPER_INVOCATION", "helper evidence tier")
    require(receipt["compiler_process_policy"] == {
        "compiler_session": "OWNED_NEW_SESSION_PER_DRIVER_CALL",
        "failure_cleanup": "TERMINATE_REAP_OWNED_COMPILER_PROCESS_GROUP_ONLY",
        "helper_selection": "DRIVER_LOOKUP_ONLY_NOT_HELPER_INVOCATION",
        "cleanup_reserve_seconds": 5}, "recorded compiler process policy")
    require(set(receipt["tools"]) == {"compiler", *HELPERS}, "five tool metadata pins")
    require(set(receipt["runtime_inputs"]) == set(RUNTIME), "seven runtime metadata pins")
    for label, value in list(receipt["tools"].items()) + list(receipt["runtime_inputs"].items()):
        check_metadata_pin(value, label)

    sources = read_json("source-inputs.json")
    dependencies = read_json("dependencies.json")
    require(len(sources) == 933 and len(dependencies) == 99, "source/dependency inventory count")
    for name, value in sources.items():
        require(posixpath.normpath(name) == name and not name.startswith(("/", "../", "$")), "source metadata path")
        check_metadata_pin(value, name)
    require(all(unit in sources for unit in UNITS), "unit absent from source inventory")
    for name, value in dependencies.items():
        require(normalized_path(name) == name, "dependency metadata path")
        check_metadata_pin(value, name)
        if name.startswith(SOURCE_ROOT):
            relative = name[len(SOURCE_ROOT):]
            require(relative in sources and value == sources[relative], "dependency/source metadata mismatch")

    labels = (["compiler-version"] + ["helper-" + name for name in HELPERS] +
              ["runtime-" + name.replace(".", "-") for name in RUNTIME] +
              ["dependency-%02d" % number for number in range(28)] + ["compile-link"])
    calls = read_json("calls.json")
    require(len(calls) == 41 and [call["label"] for call in calls] == labels, "exact ordered compiler call labels")
    expected_logs = {"logs/" + label + suffix for label in labels for suffix in (".stdout", ".stderr")}
    require({name for name in actual if name.startswith("logs/")} == expected_logs, "exact recorded log set")
    elapsed_sum = 0.0
    for call in calls:
        label = call["label"]
        require(call["status"] == "PASS" and call["returncode"] == 0, label + " status/RC")
        require(call["timeout_seconds"] == 115 and call["call_budget_seconds"] == 120, label + " bounds")
        elapsed = call["elapsed_seconds"]
        require(isinstance(elapsed, (int, float)) and math.isfinite(elapsed) and 0 <= elapsed <= 115, label + " elapsed bound")
        elapsed_sum += elapsed
        require(call["execution_environment"] == ENVIRONMENT, label + " recorded environment")
        require(call["process_session"] == "OWNED_NEW_SESSION" and call["driver_reaped"] is True and
                call["compiler_process_group_quiescent"] is True, label + " recorded quiescence")
        require(call["product_execution"] == "NEVER_EXECUTED", label + " product execution tier")
        for stream in ("stdout", "stderr"):
            check_metadata_pin(call[stream], label + " original " + stream)
        stderr = BASE / "logs" / (label + ".stderr")
        require(stderr.read_bytes() == b"" and call["stderr"] == dict(pin(stderr), mode=0o644), label + " stderr")
        if label == "compiler-version":
            argv = [COMPILER, "--version"]
        elif label.startswith("helper-"):
            argv = [COMPILER] + LOOKUP_FLAGS + ["-print-prog-name=" + label[7:]]
        elif label.startswith("runtime-"):
            runtime_name = next(name for name in RUNTIME if "runtime-" + name.replace(".", "-") == label)
            argv = [COMPILER] + LOOKUP_FLAGS + ["-print-file-name=" + runtime_name]
        elif label.startswith("dependency-"):
            argv = [COMPILER] + FLAGS + ["-M", UNITS[int(label[11:])]]
        else:
            argv = [COMPILER] + FLAGS + UNITS + ["-o", PRODUCT]
        require(call["argv"] == argv, label + " exact saved argv")
        stdout_path = BASE / "logs" / (label + ".stdout")
        if label in ("compiler-version", "compile-link"):
            require(call["stdout"] == dict(pin(stdout_path), mode=0o644), label + " unchanged stdout pin")
    require(elapsed_sum <= overall, "sum of recorded call times exceeds overall time")
    require((BASE / "logs/compiler-version.stdout").read_bytes().startswith(b"m68k-amigaos-gcc (GCC) 6.5.0b "), "saved compiler version")
    require((BASE / "logs/compile-link.stdout").read_bytes() == b"", "link stdout")

    for name, expected in HELPERS.items():
        require(lookup_stdout("helper-" + name) == expected, "parsed helper lookup " + name)
    for name, expected in RUNTIME.items():
        require(lookup_stdout("runtime-" + name.replace(".", "-")) == expected, "parsed runtime lookup " + name)

    dependency_union = set()
    for number, unit in enumerate(UNITS):
        text = (BASE / "logs" / ("dependency-%02d.stdout" % number)).read_text()
        joined = text.replace("\\\n", " ")
        require(joined.count(":") == 1, "dependency target separator")
        target, body = joined.split(":", 1)
        require(target.strip() == posixpath.basename(unit)[:-2] + ".o", "dependency object target")
        tokens = shlex.split(body, posix=True)
        require(tokens and posixpath.normpath(tokens[0]) == unit, "dependency leading unit")
        for token in tokens:
            value = token if token.startswith("$SDK/") else SOURCE_ROOT + token
            dependency_union.add(normalized_path(value))
    require(dependency_union == set(dependencies), "parsed dependency union differs from metadata")

    stdout = (BASE / "expected-native.stdout").read_bytes()
    require(dict(pin(BASE / "expected-native.stdout"), mode=(BASE / "expected-native.stdout").stat().st_mode & 0o777) == STDOUT_PIN,
            "complete expected native stdout pin")
    lines = stdout.decode("utf-8").splitlines()
    require(len(lines) == 6 and stdout.endswith(b"\n"), "six complete expected marker lines")
    require(all(line.startswith(marker + ":") for line, marker in zip(lines, MARKERS)), "complete marker prefixes")
    require(all("SOFTWARE_ONLY" in line for line in lines[:3]) and
            "fake library, no hardware" in lines[3] and "injected only" in lines[4] and
            "fake library/bus only" in lines[5], "expected software evidence scopes")

    saved = read_json("saved-byte-verification.json")
    require(saved["status"] == "PASS_INDEPENDENT_OFFLINE_SAVED_BYTES_PORTABLE_NEVER_EXECUTED", "saved offline verification status")
    require([saved[k] for k in ("source_count", "unit_count", "dependency_count", "observed_compiler_calls")] == [933, 28, 99, 41], "saved verification counts")
    require(saved["product"] == PRODUCT_PIN, "saved verification product metadata")
    failure = read_json("first-failure/receipt.json")
    require(failure["status"] == "FIRST_FAILURE_RETAINED" and failure["compiler_calls"] == 0 and
            failure["readback_RC"] == 0 and failure["product"] == "NONE" and failure["target_operations"] == 0,
            "retained actual Git refusal tier")
    require(failure["failure"] == "RuntimeError('Git readback failed: head-before')", "retained failure")
    require((BASE / "first-failure/head-before.stdout").read_bytes() == (BASELINE + "\n").encode(), "retained Git HEAD stdout")
    require((BASE / "first-failure/head-before.stderr").read_bytes() ==
            b"git: warning: confstr() failed with code 5: couldn't get path of DARWIN_USER_TEMP_DIR; using /tmp instead\n", "retained macOS Git warning")
    print(json.dumps({
        "status": "PASS_SAVED_PUBLIC_PORTABLE_PACKET", "files": len(actual),
        "recorded_compiler_calls": 41, "source_metadata": 933, "ordered_units": 28,
        "parsed_dependency_union": len(dependency_union), "helper_lookup_paths": 4,
        "runtime_lookup_paths": 7, "expected_stdout": STDOUT_PIN,
        "recorded_product_pin": PRODUCT_PIN, "binary_reinspection": "NOT_AVAILABLE_IN_PUBLIC_PACKET",
        "retained_first_failure": "ACTUAL_GIT_READ_RC0_STDERR_REFUSAL_ZERO_COMPILER_CALLS",
        "product_or_target_execution": "NONE",
        "limits": "Public sanitized byte custody and saved compiler metadata only. Original log pins are metadata; SDK/tool/source pool and product bodies are absent. No live Git, target ownership, native, physical, timing or listening claim."
    }, sort_keys=True))


if __name__ == "__main__":
    main()
