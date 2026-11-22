#!/usr/bin/env python3
"""Run the exact routed MIDI fixture in a previously admitted shared 030 guest."""

import argparse
import asyncio
import fcntl
import hashlib
import json
import re
import secrets
import stat
import subprocess
from pathlib import Path


MIDI_PASS = "Routed MIDI: endpoint identity, exact frame, shared note, cancellation, full batch and owned Stop plans"
REPLAY_PASS = "Routed sequence: same Paula/AmiGUS attack frame, MIDI delay after Fxx, OFF, mute, F00 cleanup and replay via lookahead"
TARGET = "amiberry-030"
EXECUTABLE = "PTRoutedMidiWire"
GUEST_LOG = "routed-midi.log"


def digest(data):
    return hashlib.sha256(data).hexdigest()


def read_regular(path, maximum):
    info = path.lstat()
    if not stat.S_ISREG(info.st_mode) or info.st_nlink != 1 or info.st_size > maximum:
        raise RuntimeError(f"Unexpected file identity or size: {path.name}")
    return path.read_bytes()


def check_completion(reply, log):
    if reply.splitlines().count("PT-ROUTED-RC 0") != 1 or log.splitlines() != [REPLAY_PASS, MIDI_PASS]:
        raise RuntimeError("Routed MIDI fixture did not report clean completion")


def devbench_listener_is(pid):
    observed = subprocess.run(
        ["/usr/sbin/lsof", "-nP", "-iTCP:3000", "-sTCP:LISTEN", "-Fp"],
        capture_output=True, text=True, timeout=3,
    )
    return observed.returncode == 0 and not observed.stderr and set(observed.stdout.splitlines()) == {f"p{pid}"}


async def run(args, binary, out):
    # Run with the shared harness's Python and its scripts on PYTHONPATH.
    from bridge_checks import checksum, require_reply, target
    from emulator import R, PROFILES, owns, processes, owned_listener
    from launcher_access import install_client, operation, transport
    from mcp import ClientSession
    from mcp.client.streamable_http import streamable_http_client

    profile = R / "runtime/amiberry/Configurations" / (PROFILES[TARGET] + ".uae")
    share = R / "runtime/Dev/Tests"
    if share.resolve(strict=True) != share or out.is_relative_to(R):
        raise RuntimeError("Expected the canonical share and a separate evidence directory")

    def verify_target():
        active = processes()
        if len(active) != 1 or active[0][0] != args.pid or not owns(active[0][1], TARGET):
            raise RuntimeError("Active 030 identity changed")
        if digest(read_regular(profile, 65536)) != args.profile_sha256:
            raise RuntimeError("Active profile bytes changed")
        if not owned_listener(args.pid, 3):
            raise RuntimeError("Expected the bridge listener on the owned emulator")
        if not devbench_listener_is(args.service_pid):
            raise RuntimeError("DevBench service identity changed")
        target(R, TARGET)

    install_client()
    with (R / "runtime/test.lock").open("rb") as command_lock:
        fcntl.flock(command_lock, fcntl.LOCK_EX | fcntl.LOCK_NB)
        verify_target()
        key = "pt24g-midi-" + secrets.token_hex(8)
        local = share / key
        guest = "Dev:Tests/" + key
        (out / "intent.json").write_text(json.dumps({
            "candidate_sha256": args.sha256, "profile_sha256": args.profile_sha256,
            "target": TARGET, "owned_pid": args.pid, "service_pid": args.service_pid, "guest_path": guest,
        }, indent=2) + "\n")
        with operation(R, TARGET, "Routed MIDI native fixture", may_leave_running=True):
            with transport(R, TARGET):
                verify_target()
                local.mkdir(mode=0o700)
                original_dir = local.stat()
                staged = local / EXECUTABLE
                with staged.open("xb") as file:
                    file.write(binary)
                original_file = staged.lstat()

            async with streamable_http_client("http://127.0.0.1:3000/mcp") as (rd, wr, _):
                async with ClientSession(rd, wr) as session:
                    await session.initialize()

                    async def call(label, tool, params):
                        verify_target()
                        reply = await session.call_tool(tool, params)
                        text = "\n".join(item.text for item in reply.content if hasattr(item, "text"))
                        (out / (label + ".txt")).write_text(text)
                        return require_reply(reply)

                    identity = await call("identity", "amiga_run_script", {"script": "CPU", "timeout": 10})
                    if not re.search(r"(?m)^System: 68030\b", identity):
                        raise RuntimeError("Guest CPU verification failed")
                    crc = await call("checksum", "amiga_checksum", {"path": guest + "/" + EXECUTABLE})
                    checksum(crc, binary)
                    text = await call("execution", "amiga_run_script", {
                        "script": f"Stack 65536\nCD {guest}\n{EXECUTABLE} >{GUEST_LOG}\nEcho PT-ROUTED-RC $RC",
                        "timeout": 30,
                    })

            with transport(R, TARGET):
                verify_target()
                current_dir = local.lstat()
                if not stat.S_ISDIR(current_dir.st_mode) or (current_dir.st_dev, current_dir.st_ino) != (
                    original_dir.st_dev, original_dir.st_ino
                ):
                    raise RuntimeError("Run directory changed")
                if set(p.name for p in local.iterdir()) != {EXECUTABLE, GUEST_LOG}:
                    raise RuntimeError("Run directory contains unexpected files")
                log = read_regular(local / GUEST_LOG, 65536)
                (out / GUEST_LOG).write_bytes(log)
                check_completion(text, log.decode("ascii"))
                if staged.lstat().st_ino != original_file.st_ino or read_regular(staged, 1048576) != binary:
                    raise RuntimeError("Staged executable changed")
                staged.unlink()
                (local / GUEST_LOG).unlink()
                local.rmdir()
            (out / "result.json").write_text(json.dumps({
                "candidate_sha256": args.sha256, "core_assertions_passed": True,
                "guest_exit_code": 0, "run_directory_removed": True,
            }, indent=2) + "\n")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--candidate", type=Path, required=True)
    parser.add_argument("--sha256", required=True)
    parser.add_argument("--profile-sha256", required=True)
    parser.add_argument("--pid", type=int, required=True)
    parser.add_argument("--service-pid", type=int, required=True)
    parser.add_argument("--evidence", type=Path, required=True)
    args = parser.parse_args()
    if args.pid <= 1 or args.service_pid <= 1 or any(not re.fullmatch(r"[0-9a-f]{64}", value)
                            for value in (args.sha256, args.profile_sha256)):
        parser.error("Expected the owned PID and exact lower-case SHA-256 hashes")
    binary = read_regular(args.candidate, 1048576)
    if not binary or digest(binary) != args.sha256:
        parser.error("Candidate hash mismatch")
    out = args.evidence.resolve()
    out.mkdir(mode=0o700)
    asyncio.run(run(args, binary, out))


if __name__ == "__main__":
    main()
